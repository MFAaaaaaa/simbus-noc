#include "simbus/routetable.h"
#include "simbus/symmulcha.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

using namespace simbus;

struct Options {
    std::string topology = "mesh";     // mesh, ring, double_ring
    std::string pattern = "uniform";   // uniform, hotspot
    uint32_t nodes = 16;
    uint32_t mesh_x = 4;
    uint32_t mesh_y = 4;
    uint32_t ticks = 10000;
    uint32_t drain_ticks = 10000;
    uint32_t payload_bytes = 32;
    uint32_t channel_width = 8;
    uint32_t link_width = 16;
    uint32_t route_latency = 2;
    double offered_load = 0.50;
    uint32_t hotspot_node = 0;
    double hotspot_prob = 0.80;
    bool congestion_enabled = true;
    uint32_t buffer_limit = 16;
    uint32_t high_watermark = 12;
    uint32_t low_watermark = 4;
    uint32_t seed = 1;
    bool csv_header = true;
};

struct Metrics {
    uint64_t offered_messages = 0;
    uint64_t injected_messages = 0;
    uint64_t source_blocked_messages = 0;
    uint64_t delivered_messages = 0;
    uint64_t delivered_bytes = 0;
    uint64_t drain_ticks_used = 0;

    uint64_t undelivered_messages() const {
        return injected_messages >= delivered_messages ? injected_messages - delivered_messages : 0;
    }

    bool drain_timeout(uint32_t drain_tick_budget) const {
        return undelivered_messages() > 0 && drain_ticks_used >= drain_tick_budget;
    }
};

void usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --topology mesh|ring|double_ring   default: mesh\n"
        << "  --pattern uniform|hotspot          default: uniform\n"
        << "  --nodes N                          default: 16\n"
        << "  --mesh-x N                         default: 4\n"
        << "  --mesh-y N                         default: 4\n"
        << "  --ticks N                          default: 10000\n"
        << "  --drain-ticks N                    default: 10000\n"
        << "  --payload-bytes N                  default: 32\n"
        << "  --channel-width N                  default: 8\n"
        << "  --link-width N                     default: 16\n"
        << "  --route-latency N                  default: 2\n"
        << "  --offered-load X                   default: 0.50, probability per source per tick\n"
        << "  --hotspot-node N                   default: 0\n"
        << "  --hotspot-prob X                   default: 0.80\n"
        << "  --congestion 0|1                   default: 1\n"
        << "  --buffer-limit N                   default: 16\n"
        << "  --high-watermark N                 default: 12\n"
        << "  --low-watermark N                  default: 4\n"
        << "  --seed N                           default: 1\n"
        << "  --no-header                        omit CSV header\n"
        << "  --help                             show this message\n";
}

uint32_t parse_u32(const char* s, const char* name) {
    char* end = nullptr;
    unsigned long v = std::strtoul(s, &end, 10);
    if (end == s || *end != '\0') {
        throw std::runtime_error(std::string("invalid integer for ") + name + ": " + s);
    }
    return static_cast<uint32_t>(v);
}

double parse_double(const char* s, const char* name) {
    char* end = nullptr;
    double v = std::strtod(s, &end);
    if (end == s || *end != '\0') {
        throw std::runtime_error(std::string("invalid floating point value for ") + name + ": " + s);
    }
    return v;
}

Options parse_args(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto need_value = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("missing value for ") + name);
            }
            return argv[++i];
        };

        if (arg == "--help") {
            usage(argv[0]);
            std::exit(0);
        } else if (arg == "--topology") {
            opt.topology = need_value("--topology");
        } else if (arg == "--pattern") {
            opt.pattern = need_value("--pattern");
        } else if (arg == "--nodes") {
            opt.nodes = parse_u32(need_value("--nodes"), "--nodes");
        } else if (arg == "--mesh-x") {
            opt.mesh_x = parse_u32(need_value("--mesh-x"), "--mesh-x");
        } else if (arg == "--mesh-y") {
            opt.mesh_y = parse_u32(need_value("--mesh-y"), "--mesh-y");
        } else if (arg == "--ticks") {
            opt.ticks = parse_u32(need_value("--ticks"), "--ticks");
        } else if (arg == "--drain-ticks") {
            opt.drain_ticks = parse_u32(need_value("--drain-ticks"), "--drain-ticks");
        } else if (arg == "--payload-bytes") {
            opt.payload_bytes = parse_u32(need_value("--payload-bytes"), "--payload-bytes");
        } else if (arg == "--channel-width") {
            opt.channel_width = parse_u32(need_value("--channel-width"), "--channel-width");
        } else if (arg == "--link-width") {
            opt.link_width = parse_u32(need_value("--link-width"), "--link-width");
        } else if (arg == "--route-latency") {
            opt.route_latency = parse_u32(need_value("--route-latency"), "--route-latency");
        } else if (arg == "--offered-load") {
            opt.offered_load = parse_double(need_value("--offered-load"), "--offered-load");
        } else if (arg == "--hotspot-node") {
            opt.hotspot_node = parse_u32(need_value("--hotspot-node"), "--hotspot-node");
        } else if (arg == "--hotspot-prob") {
            opt.hotspot_prob = parse_double(need_value("--hotspot-prob"), "--hotspot-prob");
        } else if (arg == "--congestion") {
            opt.congestion_enabled = parse_u32(need_value("--congestion"), "--congestion") != 0;
        } else if (arg == "--buffer-limit") {
            opt.buffer_limit = parse_u32(need_value("--buffer-limit"), "--buffer-limit");
        } else if (arg == "--high-watermark") {
            opt.high_watermark = parse_u32(need_value("--high-watermark"), "--high-watermark");
        } else if (arg == "--low-watermark") {
            opt.low_watermark = parse_u32(need_value("--low-watermark"), "--low-watermark");
        } else if (arg == "--seed") {
            opt.seed = parse_u32(need_value("--seed"), "--seed");
        } else if (arg == "--no-header") {
            opt.csv_header = false;
        } else {
            throw std::runtime_error("unknown option: " + arg);
        }
    }

    if (opt.topology != "mesh" && opt.topology != "ring" && opt.topology != "double_ring") {
        throw std::runtime_error("--topology must be mesh, ring, or double_ring");
    }
    if (opt.pattern != "uniform" && opt.pattern != "hotspot") {
        throw std::runtime_error("--pattern must be uniform or hotspot");
    }
    if (opt.nodes < 2) {
        throw std::runtime_error("--nodes must be >= 2");
    }
    if (opt.topology == "mesh") {
        if (opt.mesh_x == 0 || opt.mesh_y == 0 || opt.mesh_x * opt.mesh_y != opt.nodes) {
            throw std::runtime_error("for mesh, --mesh-x * --mesh-y must equal --nodes");
        }
    }
    if (opt.offered_load < 0.0 || opt.offered_load > 1.0) {
        throw std::runtime_error("--offered-load must be in [0, 1]");
    }
    if (opt.hotspot_prob < 0.0 || opt.hotspot_prob > 1.0) {
        throw std::runtime_error("--hotspot-prob must be in [0, 1]");
    }
    if (opt.hotspot_node >= opt.nodes) {
        throw std::runtime_error("--hotspot-node must be less than --nodes");
    }
    if (opt.payload_bytes == 0) {
        throw std::runtime_error("--payload-bytes must be non-zero");
    }
    if (opt.channel_width == 0 || opt.link_width == 0) {
        throw std::runtime_error("--channel-width and --link-width must be non-zero");
    }
    if (opt.congestion_enabled) {
        if (opt.buffer_limit == 0) {
            throw std::runtime_error("--buffer-limit must be non-zero when congestion is enabled");
        }
        if (opt.high_watermark > opt.buffer_limit || opt.low_watermark > opt.high_watermark) {
            throw std::runtime_error("watermarks must satisfy low <= high <= buffer_limit");
        }
    }
    return opt;
}

std::vector<uint8_t> make_payload(uint32_t bytes, uint32_t src, uint32_t seq) {
    std::vector<uint8_t> out(bytes);
    for (uint32_t i = 0; i < bytes; ++i) {
        out[i] = static_cast<uint8_t>((src * 17 + seq * 31 + i) & 0xffu);
    }
    return out;
}

BusRouteTable make_route(const Options& opt, const std::vector<BusNodeT>& nodes) {
    BusRouteTable route;
    if (opt.topology == "mesh") {
        genroute_mesh2d_xy(nodes, opt.mesh_x, opt.mesh_y, true, route);
    } else if (opt.topology == "ring") {
        genroute_single_ring(nodes, route);
    } else {
        genroute_double_ring(nodes, route);
    }
    return route;
}

BusPortT choose_dst(const Options& opt, BusPortT src, std::mt19937& rng) {
    std::uniform_int_distribution<uint32_t> node_dist(0, opt.nodes - 1);
    std::uniform_real_distribution<double> prob(0.0, 1.0);

    if (opt.pattern == "hotspot" && src != opt.hotspot_node && prob(rng) < opt.hotspot_prob) {
        return opt.hotspot_node;
    }

    BusPortT dst = src;
    while (dst == src) {
        dst = node_dist(rng);
    }
    return dst;
}

void drain_ready_messages(SymmetricMultiChannelBus& bus, const Options& opt, Metrics& metrics) {
    for (BusPortT p = 0; p < opt.nodes; ++p) {
        while (bus.can_recv(p, 0)) {
            std::vector<uint8_t> recv;
            if (!bus.recv(p, 0, recv)) {
                break;
            }
            metrics.delivered_messages++;
            metrics.delivered_bytes += recv.size();
        }
    }
}

void print_csv_header() {
    std::cout
        << "topology,pattern,nodes,mesh_x,mesh_y,ticks,sim_ticks,drain_ticks_used,"
        << "payload_bytes,channel_width,link_width,route_latency,congestion,buffer_limit,"
        << "high_watermark,low_watermark,offered_load,hotspot_node,hotspot_prob,seed,"
        << "offered_messages,injected_messages,source_blocked_messages,delivered_messages,"
        << "undelivered_messages,in_flight_messages,delivery_ratio,drain_timeout,"
        << "delivered_bytes,throughput_msg_per_tick,throughput_bytes_per_tick,"
        << "avg_packet_latency,blocked_cycles,congested_cycles,edge_busy_cycles\n";
}

void print_csv_row(const Options& opt, const SymmetricMultiChannelBus& bus, const Metrics& metrics) {
    const NocStatistics st = bus.statistics();
    const double delivery_ratio = metrics.injected_messages == 0
        ? 0.0
        : static_cast<double>(metrics.delivered_messages) / static_cast<double>(metrics.injected_messages);
    const double throughput_msg = st.tick == 0
        ? 0.0
        : static_cast<double>(metrics.delivered_messages) / static_cast<double>(st.tick);
    const double throughput_bytes = st.tick == 0
        ? 0.0
        : static_cast<double>(metrics.delivered_bytes) / static_cast<double>(st.tick);
    const uint64_t undelivered = metrics.undelivered_messages();
    const uint64_t in_flight = undelivered;
    const bool drain_timeout = metrics.drain_timeout(opt.drain_ticks);

    std::cout << std::fixed << std::setprecision(6)
              << opt.topology << ','
              << opt.pattern << ','
              << opt.nodes << ','
              << opt.mesh_x << ','
              << opt.mesh_y << ','
              << opt.ticks << ','
              << st.tick << ','
              << metrics.drain_ticks_used << ','
              << opt.payload_bytes << ','
              << opt.channel_width << ','
              << opt.link_width << ','
              << opt.route_latency << ','
              << (opt.congestion_enabled ? 1 : 0) << ','
              << opt.buffer_limit << ','
              << opt.high_watermark << ','
              << opt.low_watermark << ','
              << opt.offered_load << ','
              << opt.hotspot_node << ','
              << opt.hotspot_prob << ','
              << opt.seed << ','
              << metrics.offered_messages << ','
              << metrics.injected_messages << ','
              << metrics.source_blocked_messages << ','
              << metrics.delivered_messages << ','
              << undelivered << ','
              << in_flight << ','
              << delivery_ratio << ','
              << (drain_timeout ? 1 : 0) << ','
              << metrics.delivered_bytes << ','
              << throughput_msg << ','
              << throughput_bytes << ','
              << st.average_packet_latency() << ','
              << st.blocked_cycles << ','
              << st.congested_cycles << ','
              << st.edge_busy_cycles << '\n';
}

int run(const Options& opt) {
    std::vector<BusNodeT> nodes(opt.nodes);
    std::vector<BusPortT> ports(opt.nodes);
    std::vector<BusNodeT> port_to_node(opt.nodes);
    for (uint32_t i = 0; i < opt.nodes; ++i) {
        nodes[i] = i;
        ports[i] = i;
        port_to_node[i] = i;
    }

    BusRouteTable route = make_route(opt, nodes);

    NocConfig cfg;
    cfg.link_width_byte = opt.link_width;
    cfg.route_latency = opt.route_latency;
    cfg.congestion.enabled = opt.congestion_enabled;
    cfg.congestion.node_buffer_limit = opt.buffer_limit;
    cfg.congestion.high_watermark = opt.high_watermark;
    cfg.congestion.low_watermark = opt.low_watermark;

    SymmetricMultiChannelBus bus(ports, port_to_node, {opt.channel_width}, route, cfg, "bench_noc");

    std::mt19937 rng(opt.seed);
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    std::vector<uint32_t> seq(opt.nodes, 0);
    Metrics metrics;

    for (uint32_t t = 0; t < opt.ticks; ++t) {
        for (BusPortT src = 0; src < opt.nodes; ++src) {
            if (prob(rng) >= opt.offered_load) {
                continue;
            }

            metrics.offered_messages++;
            BusPortT dst = choose_dst(opt, src, rng);
            const std::vector<uint8_t> payload = make_payload(opt.payload_bytes, src, seq[src]++);
            if (bus.send(src, dst, 0, payload)) {
                metrics.injected_messages++;
            } else {
                metrics.source_blocked_messages++;
            }
        }

        bus.apply_next_tick();
        drain_ready_messages(bus, opt, metrics);
    }

    for (uint32_t t = 0; t < opt.drain_ticks && metrics.delivered_messages < metrics.injected_messages; ++t) {
        bus.apply_next_tick();
        drain_ready_messages(bus, opt, metrics);
        metrics.drain_ticks_used++;
    }

    if (opt.csv_header) {
        print_csv_header();
    }
    print_csv_row(opt, bus, metrics);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options opt = parse_args(argc, argv);
        return run(opt);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        usage(argv[0]);
        return 1;
    }
}
