#pragma once

#include "simbus/common.h"

namespace simbus {

using BusNodeT = uint32_t;
using BusPortT = uint32_t;
using ChannelT = uint32_t;
using SrcNodeT = BusNodeT;
using DstNodeT = BusNodeT;
using XmitIDT = uint32_t;

// route[src][dst] = next hop node id
using BusRouteTable = std::unordered_map<SrcNodeT, std::unordered_map<DstNodeT, BusNodeT>>;

class BusInterfaceV2 {
public:
    virtual ~BusInterfaceV2() = default;

    virtual void can_send(BusPortT port, std::vector<bool>& out) const = 0;
    virtual bool can_send(BusPortT port, ChannelT channel) const = 0;
    virtual bool send(BusPortT port, BusPortT dst_port, ChannelT channel, const std::vector<uint8_t>& data) = 0;

    virtual void can_recv(BusPortT port, std::vector<bool>& out) const = 0;
    virtual bool can_recv(BusPortT port, ChannelT channel) const = 0;
    virtual bool recv(BusPortT port, ChannelT channel, std::vector<uint8_t>& buf) = 0;

    virtual void apply_next_tick() = 0;
};

class BusRoute {
public:
    virtual ~BusRoute() = default;
    virtual BusNodeT next(BusNodeT this_node, BusNodeT dst_node) const = 0;
};

}  // namespace simbus
