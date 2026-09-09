#include "test_common.h"

namespace simbus::test {

namespace {

void test_same_node_transfer() {
    BusRouteTable route;
    route_insert(0, 0, 0, route);
    NocConfig cfg;
    cfg.route_latency = 0;
    cfg.congestion.enabled = false;
    SymmetricMultiChannelBus bus({10, 11}, {0, 0}, {8}, route, cfg, "same-node");

    auto payload = bytes(13);
    CHECK_TRUE(bus.send(10, 11, 0, payload));
    CHECK_FALSE(bus.can_send(10, 0));
    std::vector<uint8_t> recv;
    drain_one_message(bus, 11, 0, recv, 20);
    CHECK_EQ(recv, payload);
}

void test_cross_node_transfer() {
    std::vector<BusNodeT> nodes{0, 1, 2, 3};
    BusRouteTable route;
    genroute_single_ring(nodes, route);

    NocConfig cfg;
    cfg.route_latency = 1;
    cfg.link_width_byte = 8;
    cfg.congestion.enabled = false;
    SymmetricMultiChannelBus bus({0, 1, 2, 3}, nodes, {8}, route, cfg, "ring");

    auto payload = bytes(19, 0x33);
    CHECK_TRUE(bus.send(0, 2, 0, payload));

    std::vector<uint8_t> recv;
    drain_one_message(bus, 2, 0, recv, 100);
    CHECK_EQ(recv, payload);
}

void test_multi_channel_independent() {
    BusRouteTable route = line3_route();
    NocConfig cfg;
    cfg.route_latency = 1;
    cfg.link_width_byte = 16;
    cfg.congestion.enabled = false;
    SymmetricMultiChannelBus bus({0, 1, 2}, {0, 1, 2}, {4, 8}, route, cfg, "line3");

    auto a = bytes(15, 0x01);
    auto b = bytes(22, 0x90);
    CHECK_TRUE(bus.send(0, 2, 0, a));
    CHECK_TRUE(bus.send(1, 0, 1, b));

    std::vector<uint8_t> ra;
    std::vector<uint8_t> rb;
    drain_one_message(bus, 2, 0, ra, 200);
    drain_one_message(bus, 0, 1, rb, 200);

    CHECK_EQ(ra, a);
    CHECK_EQ(rb, b);
}

void test_zero_length_message() {
    BusRouteTable route = line3_route();
    NocConfig cfg;
    cfg.route_latency = 1;
    cfg.congestion.enabled = false;
    SymmetricMultiChannelBus bus({0, 1, 2}, {0, 1, 2}, {8}, route, cfg, "zero-length");

    std::vector<uint8_t> payload;
    CHECK_TRUE(bus.send(0, 2, 0, payload));

    std::vector<uint8_t> recv;
    drain_one_message(bus, 2, 0, recv, 100);
    CHECK_TRUE(recv.empty());
}

void test_payload_padding_is_removed() {
    BusRouteTable route = line3_route();
    NocConfig cfg;
    cfg.route_latency = 1;
    cfg.congestion.enabled = false;
    SymmetricMultiChannelBus bus({0, 1, 2}, {0, 1, 2}, {16}, route, cfg, "padding");

    auto payload = bytes(17, 0x44); // becomes two 16-byte packets internally
    CHECK_TRUE(bus.send(0, 2, 0, payload));

    std::vector<uint8_t> recv;
    drain_one_message(bus, 2, 0, recv, 100);
    CHECK_EQ(recv.size(), 17u);
    CHECK_EQ(recv, payload);
}

}  // namespace

void register_basic_transfer_tests(std::vector<TestCase>& tests) {
    tests.push_back({"same_node_transfer", test_same_node_transfer});
    tests.push_back({"cross_node_transfer", test_cross_node_transfer});
    tests.push_back({"multi_channel_independent", test_multi_channel_independent});
    tests.push_back({"zero_length_message", test_zero_length_message});
    tests.push_back({"payload_padding_is_removed", test_payload_padding_is_removed});
}

}  // namespace simbus::test
