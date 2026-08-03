#pragma once

#include "simbus/symmulcha.h"

#include <cstdlib>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace simbus::test {

struct TestCase {
    const char* name;
    std::function<void()> fn;
};

#define CHECK_TRUE(expr)                                                                  \
    do {                                                                                  \
        if (!(expr)) {                                                                     \
            throw std::runtime_error(std::string("CHECK_TRUE failed: ") + #expr +        \
                                     " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        }                                                                                 \
    } while (0)

#define CHECK_FALSE(expr) CHECK_TRUE(!(expr))
#define CHECK_EQ(a, b) CHECK_TRUE((a) == (b))
#define CHECK_NE(a, b) CHECK_TRUE((a) != (b))
#define CHECK_GT(a, b) CHECK_TRUE((a) > (b))
#define CHECK_GE(a, b) CHECK_TRUE((a) >= (b))
#define CHECK_LT(a, b) CHECK_TRUE((a) < (b))
#define CHECK_LE(a, b) CHECK_TRUE((a) <= (b))

inline std::vector<uint8_t> bytes(uint32_t n, uint8_t seed = 0x10) {
    std::vector<uint8_t> out(n);
    for (uint32_t i = 0; i < n; ++i) {
        out[i] = static_cast<uint8_t>(seed + i * 13);
    }
    return out;
}

inline void tick_until_recv(SymmetricMultiChannelBus& bus, BusPortT port, ChannelT cha, uint32_t max_ticks) {
    for (uint32_t i = 0; i < max_ticks && !bus.can_recv(port, cha); ++i) {
        bus.apply_next_tick();
    }
    CHECK_TRUE(bus.can_recv(port, cha));
}

inline BusRouteTable line3_route() {
    BusRouteTable route;
    route_insert(0, 0, 0, route);
    route_insert(0, 1, 1, route);
    route_insert(0, 2, 1, route);

    route_insert(1, 0, 0, route);
    route_insert(1, 1, 1, route);
    route_insert(1, 2, 2, route);

    route_insert(2, 0, 1, route);
    route_insert(2, 1, 1, route);
    route_insert(2, 2, 2, route);
    return route;
}

inline std::string payload_key(const std::vector<uint8_t>& v) {
    return std::string(reinterpret_cast<const char*>(v.data()), v.size());
}

inline void drain_one_message(SymmetricMultiChannelBus& bus,
                              BusPortT port,
                              ChannelT cha,
                              std::vector<uint8_t>& recv,
                              uint32_t max_ticks = 2000) {
    tick_until_recv(bus, port, cha, max_ticks);
    CHECK_TRUE(bus.recv(port, cha, recv));
}

inline NocConfig base_config(uint32_t route_latency = 1, uint32_t link_width = 8) {
    NocConfig cfg;
    cfg.route_latency = route_latency;
    cfg.link_width_byte = link_width;
    cfg.congestion.enabled = true;
    cfg.congestion.node_buffer_limit = 64;
    cfg.congestion.high_watermark = 48;
    cfg.congestion.low_watermark = 16;
    return cfg;
}

}  // namespace simbus::test
