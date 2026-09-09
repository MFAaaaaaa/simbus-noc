#pragma once

#include "simbus/businterface.h"
#include "simbus/noc_config.h"
#include "simbus/routetable.h"

namespace simbus {

class SymmetricMultiChannelBus : public BusInterfaceV2 {
public:
    SymmetricMultiChannelBus(
        const std::vector<BusPortT>& port_ids,
        const std::vector<BusNodeT>& port_to_node,
        const std::vector<uint32_t>& channels_width_byte,
        const BusRouteTable& route,
        NocConfig config = {},
        std::string name = "noc");

    ~SymmetricMultiChannelBus() override;

    void can_send(BusPortT port, std::vector<bool>& out) const override;
    bool can_send(BusPortT port, ChannelT channel) const override;
    bool send(BusPortT port, BusPortT dst_port, ChannelT channel, const std::vector<uint8_t>& data) override;

    void can_recv(BusPortT port, std::vector<bool>& out) const override;
    bool can_recv(BusPortT port, ChannelT channel) const override;
    bool recv(BusPortT port, ChannelT channel, std::vector<uint8_t>& buf) override;

    void apply_next_tick() override;

    void clear_statistics();
    NocStatistics statistics() const;
    uint64_t current_tick() const { return current_tick_; }

    uint32_t node_buffered_packets(BusNodeT node) const;
    bool node_congested(BusNodeT node) const;

    void print_setup_info(std::ostream& out) const;
    void print_statistics(std::ostream& out) const;

private:
    struct MsgPack {
        BusPortT src = 0;
        BusPortT dst = 0;
        BusNodeT tgt = 0;
        ChannelT cha = 0;
        XmitIDT xmtid = 0;
        uint32_t len = 0;
        uint16_t pac_idx = 0;
        uint16_t pac_cnt = 0;
        std::vector<uint8_t> data;
        uint64_t tx_start_tick = 0;
    };

    struct PipelinedPack {
        MsgPack* pack = nullptr;
        uint32_t cycle_remained = 0;
    };

    struct EdgeInChannel {
        std::vector<MsgPack*> input;
        std::vector<MsgPack*> output;
        BusNodeT from = 0;
        BusNodeT to = 0;
        uint64_t busy_cycles = 0;
    };

    struct PortStruct {
        std::vector<std::list<MsgPack*>> recv_buf;
        std::vector<std::list<MsgPack*>> send_buf;
    };

    struct NodeStruct {
        BusNodeT myid = 0;
        std::unordered_map<BusNodeT, EdgeInChannel*> txs;
        std::vector<EdgeInChannel*> rxs;
        uint32_t rx_ptr = 0;

        std::unordered_map<BusPortT, PortStruct*> port;
        std::unordered_map<BusPortT, PortStruct*>::iterator sd_ptr;

        std::list<PipelinedPack> pipeline;
        std::unordered_map<XmitIDT, std::list<MsgPack*>> order_buf;

        uint32_t buffered_packets = 0;
        bool congested = false;

        uint64_t busy_cycles = 0;
        uint64_t passed_packs = 0;
        uint64_t congested_cycles = 0;
        uint64_t blocked_cycles = 0;
    };

    struct Candidate {
        enum class Source { None, EdgeOutput, PortSend } source = Source::None;
        MsgPack* pack = nullptr;
        EdgeInChannel* edge = nullptr;
        ChannelT channel = 0;
        PortStruct* port = nullptr;
        std::list<MsgPack*>* list = nullptr;
    };

    uint32_t cha_cnt_ = 0;
    uint32_t link_width_byte_ = 0;
    uint32_t route_latency_ = 0;
    uint32_t xmtid_alloc_ = 0;
    uint64_t current_tick_ = 0;

    std::vector<uint32_t> cha_widths_;
    std::vector<BusNodeT> init_port_to_node_;
    std::unordered_map<BusPortT, BusNodeT> port2node_;
    BusRouteTable route_;
    NocConfig config_;
    std::string name_;

    std::unordered_map<BusNodeT, NodeStruct> nodes_;
    std::unordered_map<BusPortT, PortStruct> ports_;
    std::vector<EdgeInChannel> edges_;

    NocStatistics stats_;
    std::unordered_map<uint64_t, uint64_t> transmit_cnt_;

    void validate_config() const;
    void validate_port(BusPortT port) const;
    void validate_channel(ChannelT channel) const;
    void build_network(const std::vector<BusPortT>& port_ids, const std::vector<BusNodeT>& port_to_node);

    void process_node(NodeStruct& node);
    void process_edge(EdgeInChannel& edge);

    Candidate find_candidate(NodeStruct& node);
    bool can_accept_local_packet(NodeStruct& node) const;
    bool can_accept_forward_packet(NodeStruct& node) const;
    bool can_accept_candidate(NodeStruct& node, const Candidate& cand) const;
    void detach_candidate(NodeStruct& node, Candidate& cand);
    void admit_candidate(NodeStruct& node, MsgPack* pack);
    void deliver_to_local_port(NodeStruct& node, MsgPack* pack);
    void update_congestion_state(NodeStruct& node);
    void refresh_all_congestion_states();
    void delete_all_packets();

    BusNodeT next_hop(BusNodeT src, BusNodeT dst) const;
};

}  // namespace simbus
