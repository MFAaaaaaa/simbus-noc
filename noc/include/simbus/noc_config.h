#pragma once

#include "simbus/common.h"

namespace simbus {

struct CongestionConfig {
    bool enabled = true;

    // Router-local buffered packet count limit. This bounds packets already
    // admitted into a router, including forwarding pipeline packets and packets
    // waiting for reassembly at the destination node.
    uint32_t node_buffer_limit = 512;

    // Hysteresis watermarks. A node becomes congested at or above the high
    // watermark and recovers at or below the low watermark.
    uint32_t high_watermark = 480;
    uint32_t low_watermark = 384;
};

struct NocConfig {
    // Max bytes an edge can move from input registers to output registers in one tick.
    uint32_t link_width_byte = 64;

    // Number of ticks consumed inside a router before a packet can be sent to the next hop.
    uint32_t route_latency = 3;

    CongestionConfig congestion;

    bool enable_log = false;
};

struct NocStatistics {
    uint64_t tick = 0;
    uint64_t transmitted_packets = 0;
    uint64_t received_messages = 0;
    uint64_t packet_latency_sum = 0;
    uint64_t blocked_cycles = 0;
    uint64_t congested_cycles = 0;
    uint64_t edge_busy_cycles = 0;

    double average_packet_latency() const {
        if (transmitted_packets == 0) {
            return 0.0;
        }
        return static_cast<double>(packet_latency_sum) / static_cast<double>(transmitted_packets);
    }
};

}  // namespace simbus
