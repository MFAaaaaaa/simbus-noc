#include "simbus/routetable.h"

namespace simbus {
namespace {

void assert_unique_nodes(const std::vector<BusNodeT>& nodes) {
    std::set<BusNodeT> unique(nodes.begin(), nodes.end());
    require(unique.size() == nodes.size(), "route table generation failed: duplicate node id");
}

}  // namespace

void route_insert(SrcNodeT src, DstNodeT dst, BusNodeT next, BusRouteTable& table) {
    table[src][dst] = next;
}

void route_delete(SrcNodeT src, DstNodeT dst, BusRouteTable& table) {
    auto src_it = table.find(src);
    if (src_it == table.end()) {
        return;
    }
    src_it->second.erase(dst);
    if (src_it->second.empty()) {
        table.erase(src_it);
    }
}

void genroute_single_ring(const std::vector<BusNodeT>& nodes, BusRouteTable& out) {
    assert_unique_nodes(nodes);
    require(!nodes.empty(), "single ring needs at least one node");

    out.clear();
    const uint32_t cnt = static_cast<uint32_t>(nodes.size());
    if (cnt == 1) {
        route_insert(nodes[0], nodes[0], nodes[0], out);
        return;
    }

    for (uint32_t i = 0; i < cnt; ++i) {
        const BusNodeT src = nodes[i];
        uint32_t idx = (i + 1) % cnt;
        const BusNodeT next = nodes[idx];
        for (uint32_t j = 0; j < cnt - 1; ++j) {
            route_insert(src, nodes[idx], next, out);
            idx = (idx + 1) % cnt;
        }
        route_insert(src, src, src, out);
    }
}

void genroute_double_ring(const std::vector<BusNodeT>& nodes, BusRouteTable& out) {
    assert_unique_nodes(nodes);
    require(!nodes.empty(), "double ring needs at least one node");

    out.clear();
    const uint32_t cnt = static_cast<uint32_t>(nodes.size());
    if (cnt == 1) {
        route_insert(nodes[0], nodes[0], nodes[0], out);
        return;
    }

    for (uint32_t i = 0; i < cnt; ++i) {
        const BusNodeT src = nodes[i];
        route_insert(src, src, src, out);

        uint32_t idx = (i + 1) % cnt;
        BusNodeT next = nodes[idx];
        for (uint32_t j = 0; j < (cnt / 2); ++j) {
            route_insert(src, nodes[idx], next, out);
            idx = (idx + 1) % cnt;
        }

        next = nodes[(i + cnt - 1) % cnt];
        while (idx != i) {
            route_insert(src, nodes[idx], next, out);
            idx = (idx + 1) % cnt;
        }
    }
}

void genroute_mesh2d_xy(
    const std::vector<BusNodeT>& nodes_by_x_then_y,
    uint32_t cnt_x,
    uint32_t cnt_y,
    bool double_direct,
    BusRouteTable& out) {
    require(cnt_x > 0 && cnt_y > 0, "mesh dimensions must be non-zero");
    require(nodes_by_x_then_y.size() == static_cast<size_t>(cnt_x) * cnt_y,
            "mesh node vector size does not match cnt_x * cnt_y");
    assert_unique_nodes(nodes_by_x_then_y);

    auto node_at = [&](uint32_t x, uint32_t y) -> BusNodeT {
        return nodes_by_x_then_y[x + y * cnt_x];
    };

    out.clear();
    for (uint32_t y = 0; y < cnt_y; ++y) {
        for (uint32_t x = 0; x < cnt_x; ++x) {
            const BusNodeT src = node_at(x, y);
            route_insert(src, src, src, out);

            for (uint32_t dy = 0; dy < cnt_y; ++dy) {
                for (uint32_t dx = 0; dx < cnt_x; ++dx) {
                    const BusNodeT dst = node_at(dx, dy);
                    if (dst == src) {
                        continue;
                    }

                    BusNodeT next = src;
                    if (dx != x) {
                        const uint32_t right_dist = (dx + cnt_x - x) % cnt_x;
                        const uint32_t left_dist = (x + cnt_x - dx) % cnt_x;
                        const bool go_left = double_direct && left_dist < right_dist;
                        const uint32_t nx = go_left ? (x + cnt_x - 1) % cnt_x : (x + 1) % cnt_x;
                        next = node_at(nx, y);
                    } else {
                        const uint32_t down_dist = (dy + cnt_y - y) % cnt_y;
                        const uint32_t up_dist = (y + cnt_y - dy) % cnt_y;
                        const bool go_up = double_direct && up_dist < down_dist;
                        const uint32_t ny = go_up ? (y + cnt_y - 1) % cnt_y : (y + 1) % cnt_y;
                        next = node_at(x, ny);
                    }
                    route_insert(src, dst, next, out);
                }
            }
        }
    }
}

bool route_table_valid(const std::vector<BusNodeT>& nodes, const BusRouteTable& table, std::string* error) {
    std::set<BusNodeT> node_set(nodes.begin(), nodes.end());
    if (node_set.size() != nodes.size()) {
        if (error) *error = "duplicate node id in node list";
        return false;
    }

    for (const auto src : nodes) {
        for (const auto dst : nodes) {
            BusNodeT cur = src;
            std::set<BusNodeT> seen;
            for (uint32_t hop = 0; hop <= nodes.size(); ++hop) {
                if (cur == dst) {
                    break;
                }
                if (!seen.insert(cur).second) {
                    if (error) *error = str_join("route has a loop: ", src, " -> ", dst);
                    return false;
                }
                auto src_it = table.find(cur);
                if (src_it == table.end()) {
                    if (error) *error = str_join("missing route source ", cur, " for dst ", dst);
                    return false;
                }
                auto dst_it = src_it->second.find(dst);
                if (dst_it == src_it->second.end()) {
                    if (error) *error = str_join("missing route entry ", cur, " -> ", dst);
                    return false;
                }
                cur = dst_it->second;
                if (node_set.find(cur) == node_set.end()) {
                    if (error) *error = str_join("next hop not in node list: ", cur);
                    return false;
                }
            }
            if (cur != dst) {
                if (error) *error = str_join("unreachable route: ", src, " -> ", dst);
                return false;
            }
        }
    }
    return true;
}

void assert_route_table_valid(const std::vector<BusNodeT>& nodes, const BusRouteTable& table) {
    std::string error;
    require(route_table_valid(nodes, table, &error), error);
}

void print_route_table(const BusRouteTable& table, std::ostream& out) {
    std::map<BusNodeT, std::map<BusNodeT, BusNodeT>> ordered;
    for (const auto& [src, dst_map] : table) {
        for (const auto& [dst, next] : dst_map) {
            ordered[src][dst] = next;
        }
    }

    for (const auto& [src, dst_map] : ordered) {
        for (const auto& [dst, next] : dst_map) {
            out << "0x" << std::hex << src << "->0x" << dst << ":0x" << next << std::dec << " ";
        }
        out << '\n';
    }
}

}  // namespace simbus
