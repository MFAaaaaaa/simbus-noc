#include "test_common.h"

#include <array>

namespace simbus::test {

namespace {

void test_random_stress_no_loss() {
    std::vector<BusNodeT> nodes{0, 1, 2, 3};
    BusRouteTable route;
    genroute_double_ring(nodes, route);

    NocConfig cfg;
    cfg.route_latency = 2;
    cfg.link_width_byte = 8;
    cfg.congestion.node_buffer_limit = 16;
    cfg.congestion.high_watermark = 12;
    cfg.congestion.low_watermark = 4;
    SymmetricMultiChannelBus bus({0, 1, 2, 3}, nodes, {4}, route, cfg, "stress");

    std::mt19937 rng(12345);
    std::array<std::vector<std::pair<BusPortT, std::vector<uint8_t>>>, 4> expected_at_port;

    for (uint32_t round = 0; round < 80; ++round) {
        const BusPortT src = rng() % 4;
        BusPortT dst = rng() % 4;
        if (dst == src) dst = (dst + 1) % 4;
        auto payload = bytes(1 + (rng() % 24), static_cast<uint8_t>(round));
        if (bus.can_send(src, 0) && bus.send(src, dst, 0, payload)) {
            expected_at_port[dst].push_back({src, payload});
        }
        bus.apply_next_tick();
    }

    for (uint32_t i = 0; i < 600; ++i) {
        bus.apply_next_tick();
    }

    for (BusPortT p = 0; p < 4; ++p) {
        std::multiset<std::string> expected;
        for (const auto& item : expected_at_port[p]) {
            expected.insert(payload_key(item.second));
        }

        std::multiset<std::string> actual;
        while (bus.can_recv(p, 0)) {
            std::vector<uint8_t> recv;
            CHECK_TRUE(bus.recv(p, 0, recv));
            actual.insert(payload_key(recv));
        }

        CHECK_EQ(actual, expected);
    }
}

void test_random_stress_with_small_buffers() {
    std::vector<BusNodeT> nodes{0, 1, 2, 3};
    BusRouteTable route;
    genroute_double_ring(nodes, route);

    NocConfig cfg;
    cfg.route_latency = 5;
    cfg.link_width_byte = 4;
    cfg.congestion.enabled = true;
    cfg.congestion.node_buffer_limit = 3;
    cfg.congestion.high_watermark = 3;
    cfg.congestion.low_watermark = 1;
    SymmetricMultiChannelBus bus({0, 1, 2, 3}, nodes, {4}, route, cfg, "small-buffer-stress");

    std::mt19937 rng(67890);
    std::array<std::vector<std::vector<uint8_t>>, 4> expected_at_port;

    for (uint32_t round = 0; round < 120; ++round) {
        const BusPortT src = rng() % 4;
        BusPortT dst = rng() % 4;
        if (dst == src) dst = (dst + 1) % 4;
        auto payload = bytes(4, static_cast<uint8_t>(round));

        if (bus.can_send(src, 0) && bus.send(src, dst, 0, payload)) {
            expected_at_port[dst].push_back(payload);
        }
        bus.apply_next_tick();
    }

    for (uint32_t i = 0; i < 2000; ++i) {
        bus.apply_next_tick();
    }

    const auto s = bus.statistics();
    CHECK_GT(s.blocked_cycles, 0u);
    CHECK_GT(s.congested_cycles, 0u);

    for (BusPortT p = 0; p < 4; ++p) {
        std::multiset<std::string> expected;
        for (const auto& payload : expected_at_port[p]) {
            expected.insert(payload_key(payload));
        }

        std::multiset<std::string> actual;
        while (bus.can_recv(p, 0)) {
            std::vector<uint8_t> recv;
            CHECK_TRUE(bus.recv(p, 0, recv));
            actual.insert(payload_key(recv));
        }

        CHECK_EQ(actual, expected);
    }
}

}  // namespace

void register_stress_tests(std::vector<TestCase>& tests) {
    tests.push_back({"random_stress_no_loss", test_random_stress_no_loss});
    tests.push_back({"random_stress_with_small_buffers", test_random_stress_with_small_buffers});
}

}  // namespace simbus::test
