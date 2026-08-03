#pragma once

#include "simbus/businterface.h"

namespace simbus {

void route_insert(SrcNodeT src, DstNodeT dst, BusNodeT next, BusRouteTable& table);
void route_delete(SrcNodeT src, DstNodeT dst, BusRouteTable& table);

void genroute_single_ring(const std::vector<BusNodeT>& nodes, BusRouteTable& out);
void genroute_double_ring(const std::vector<BusNodeT>& nodes, BusRouteTable& out);
void genroute_mesh2d_xy(
    const std::vector<BusNodeT>& nodes_by_x_then_y,
    uint32_t cnt_x,
    uint32_t cnt_y,
    bool double_direct,
    BusRouteTable& out);

void print_route_table(const BusRouteTable& table, std::ostream& out);
void assert_route_table_valid(const std::vector<BusNodeT>& nodes, const BusRouteTable& table);
bool route_table_valid(const std::vector<BusNodeT>& nodes, const BusRouteTable& table, std::string* error = nullptr);

}  // namespace simbus
