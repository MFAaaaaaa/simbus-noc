#include "test_common.h"

namespace simbus::test {

namespace {

void test_route_single_ring() {
    std::vector<BusNodeT> nodes{0, 1, 2, 3};
    BusRouteTable route;
    genroute_single_ring(nodes, route);
    assert_route_table_valid(nodes, route);
    CHECK_EQ(route[0][1], 1u);
    CHECK_EQ(route[0][3], 1u);
}

void test_route_double_ring() {
    std::vector<BusNodeT> nodes{0, 1, 2, 3};
    BusRouteTable route;
    genroute_double_ring(nodes, route);
    assert_route_table_valid(nodes, route);
    CHECK_EQ(route[0][3], 3u);
}

void test_route_mesh2d_xy() {
    std::vector<BusNodeT> nodes;
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 4; ++x) {
            nodes.push_back(x + y * 4);
        }
    }
    BusRouteTable route;
    genroute_mesh2d_xy(nodes, 4, 4, true, route);
    assert_route_table_valid(nodes, route);
    CHECK_EQ(route[0][3], 3u);   // shortest wrap-around left on x axis
    CHECK_EQ(route[0][12], 12u); // shortest wrap-around up on y axis
}

void test_route_duplicate_node_rejected() {
    std::vector<BusNodeT> nodes{0, 1, 1, 2};
    BusRouteTable route;

    bool threw = false;
    try {
        genroute_single_ring(nodes, route);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK_TRUE(threw);
}

}  // namespace

void register_routetable_tests(std::vector<TestCase>& tests) {
    tests.push_back({"route_single_ring", test_route_single_ring});
    tests.push_back({"route_double_ring", test_route_double_ring});
    tests.push_back({"route_mesh2d_xy", test_route_mesh2d_xy});
    tests.push_back({"route_duplicate_node_rejected", test_route_duplicate_node_rejected});
}

}  // namespace simbus::test
