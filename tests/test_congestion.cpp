#include "test_common.h"

namespace simbus::test {

namespace {

struct CompletionResult {
    uint64_t ticks_to_receive = 0;
    double average_packet_latency = 0.0;
    uint64_t blocked_cycles = 0;
    uint64_t congested_cycles = 0;
};

CompletionResult run_single_large_message(NocConfig cfg) {
    BusRouteTable route = line3_route();
    SymmetricMultiChannelBus bus({0, 1, 2}, {0, 1, 2}, {4}, route, cfg, "latency-line");

    auto payload = bytes(256, 0x31); // 64 packets when channel width is 4
    CHECK_TRUE(bus.send(0, 2, 0, payload));

    std::vector<uint8_t> recv;
    drain_one_message(bus, 2, 0, recv, 5000);
    CHECK_EQ(recv, payload);

    const auto s = bus.statistics();
    return CompletionResult{bus.current_tick(), s.average_packet_latency(), s.blocked_cycles, s.congested_cycles};
}

uint32_t run_stream_for_fixed_window(NocConfig cfg, uint32_t ticks) {
    BusRouteTable route = line3_route();
    // Ports 0 and 10 share source node 0. Both inject toward port 2 through node 1.
    SymmetricMultiChannelBus bus({0, 10, 1, 2}, {0, 0, 1, 2}, {4}, route, cfg, "stream-line");

    uint32_t next_seed = 0;
    uint32_t completed = 0;

    for (uint32_t t = 0; t < ticks; ++t) {
        const BusPortT src = (t % 2 == 0) ? 0 : 10;
        if (bus.can_send(src, 0)) {
            auto payload = bytes(4, static_cast<uint8_t>(next_seed++));
            CHECK_TRUE(bus.send(src, 2, 0, payload));
        }

        while (bus.can_recv(2, 0)) {
            std::vector<uint8_t> recv;
            CHECK_TRUE(bus.recv(2, 0, recv));
            ++completed;
        }

        bus.apply_next_tick();
    }

    while (bus.can_recv(2, 0)) {
        std::vector<uint8_t> recv;
        CHECK_TRUE(bus.recv(2, 0, recv));
        ++completed;
    }

    return completed;
}

void test_congestion_backpressure_and_recovery() {
    BusRouteTable route = line3_route();
    NocConfig cfg;
    cfg.route_latency = 12;
    cfg.link_width_byte = 4;
    cfg.congestion.enabled = true;
    cfg.congestion.node_buffer_limit = 2;
    cfg.congestion.high_watermark = 2;
    cfg.congestion.low_watermark = 0;

    SymmetricMultiChannelBus bus({0, 1, 2}, {0, 1, 2}, {4}, route, cfg, "congested-line");
    auto payload = bytes(96, 0x55); // 24 packets
    CHECK_TRUE(bus.send(0, 2, 0, payload));

    bool saw_congested_mid = false;
    for (uint32_t i = 0; i < 80; ++i) {
        bus.apply_next_tick();
        saw_congested_mid = saw_congested_mid || bus.node_congested(1);
    }
    auto mid = bus.statistics();
    CHECK_TRUE(saw_congested_mid);
    CHECK_GT(mid.blocked_cycles, 0u);
    CHECK_GT(mid.congested_cycles, 0u);

    std::vector<uint8_t> recv;
    drain_one_message(bus, 2, 0, recv, 1000);
    CHECK_EQ(recv, payload);

    for (uint32_t i = 0; i < 50; ++i) {
        bus.apply_next_tick();
    }
    CHECK_FALSE(bus.node_congested(1));
    CHECK_TRUE(bus.can_send(0, 0));
}

void test_congestion_increases_latency() {
    NocConfig free_cfg;
    free_cfg.route_latency = 8;
    free_cfg.link_width_byte = 4;
    free_cfg.congestion.enabled = false;

    NocConfig congested_cfg = free_cfg;
    congested_cfg.congestion.enabled = true;
    congested_cfg.congestion.node_buffer_limit = 2;
    congested_cfg.congestion.high_watermark = 2;
    congested_cfg.congestion.low_watermark = 0;

    const auto free_result = run_single_large_message(free_cfg);
    const auto congested_result = run_single_large_message(congested_cfg);

    CHECK_GT(congested_result.blocked_cycles, 0u);
    CHECK_GT(congested_result.congested_cycles, 0u);
    CHECK_GT(congested_result.ticks_to_receive, free_result.ticks_to_receive);
    CHECK_GT(congested_result.average_packet_latency, free_result.average_packet_latency);
}

void test_congestion_limits_throughput() {
    NocConfig free_cfg;
    free_cfg.route_latency = 8;
    free_cfg.link_width_byte = 4;
    free_cfg.congestion.enabled = false;

    NocConfig congested_cfg = free_cfg;
    congested_cfg.congestion.enabled = true;
    congested_cfg.congestion.node_buffer_limit = 2;
    congested_cfg.congestion.high_watermark = 2;
    congested_cfg.congestion.low_watermark = 0;

    const uint32_t uncongested_completed = run_stream_for_fixed_window(free_cfg, 240);
    const uint32_t congested_completed = run_stream_for_fixed_window(congested_cfg, 240);

    CHECK_GT(uncongested_completed, congested_completed);
    CHECK_GT(congested_completed, 0u);
}

void test_can_send_blocked_when_source_congested() {
    BusRouteTable route = line3_route();
    NocConfig cfg;
    cfg.route_latency = 16;
    cfg.link_width_byte = 4;
    cfg.congestion.enabled = true;
    cfg.congestion.node_buffer_limit = 2;
    cfg.congestion.high_watermark = 2;
    cfg.congestion.low_watermark = 0;

    // Ports 0 and 10 share node 0. Port 0 creates congestion in node 0;
    // port 10 has an empty send buffer, so can_send(10) isolates the node-level gate.
    SymmetricMultiChannelBus bus({0, 10, 1, 2}, {0, 0, 1, 2}, {4}, route, cfg, "source-gate");
    auto payload = bytes(128, 0x77);
    CHECK_TRUE(bus.send(0, 2, 0, payload));

    bool saw_source_congested = false;
    bool saw_can_send_blocked = false;
    for (uint32_t i = 0; i < 80; ++i) {
        bus.apply_next_tick();
        saw_source_congested = saw_source_congested || bus.node_congested(0);
        if (bus.node_congested(0) && !bus.can_send(10, 0)) {
            saw_can_send_blocked = true;
        }
    }

    CHECK_TRUE(saw_source_congested);
    CHECK_TRUE(saw_can_send_blocked);

    std::vector<uint8_t> recv;
    drain_one_message(bus, 2, 0, recv, 2000);
    CHECK_EQ(recv, payload);

    for (uint32_t i = 0; i < 50; ++i) {
        bus.apply_next_tick();
    }
    CHECK_FALSE(bus.node_congested(0));
    CHECK_TRUE(bus.can_send(10, 0));
}

}  // namespace

void register_congestion_tests(std::vector<TestCase>& tests) {
    tests.push_back({"congestion_backpressure_and_recovery", test_congestion_backpressure_and_recovery});
    tests.push_back({"congestion_increases_latency", test_congestion_increases_latency});
    tests.push_back({"congestion_limits_throughput", test_congestion_limits_throughput});
    tests.push_back({"can_send_blocked_when_source_congested", test_can_send_blocked_when_source_congested});
}

}  // namespace simbus::test
