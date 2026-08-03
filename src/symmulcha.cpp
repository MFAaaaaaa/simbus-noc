#include "simbus/symmulcha.h"

namespace simbus {

SymmetricMultiChannelBus::SymmetricMultiChannelBus(
    const std::vector<BusPortT>& port_ids,
    const std::vector<BusNodeT>& port_to_node,
    const std::vector<uint32_t>& channels_width_byte,
    const BusRouteTable& route,
    NocConfig config,
    std::string name)
    : cha_cnt_(static_cast<uint32_t>(channels_width_byte.size())),
      link_width_byte_(config.link_width_byte),
      route_latency_(config.route_latency),
      cha_widths_(channels_width_byte),
      init_port_to_node_(port_to_node),
      route_(route),
      config_(config),
      name_(std::move(name)) {
    validate_config();
    require(port_ids.size() == port_to_node.size(), "port_ids and port_to_node must have the same size");
    require(!port_ids.empty(), "NoC needs at least one port");
    build_network(port_ids, port_to_node);
}

SymmetricMultiChannelBus::~SymmetricMultiChannelBus() {
    delete_all_packets();
}

void SymmetricMultiChannelBus::validate_config() const {
    require(cha_cnt_ > 0, "NoC needs at least one channel");
    require(link_width_byte_ > 0, "link_width_byte must be non-zero");
    for (uint32_t c = 0; c < cha_widths_.size(); ++c) {
        require(cha_widths_[c] > 0, str_join("channel width must be non-zero: channel ", c));
        require(cha_widths_[c] <= 65536, str_join("channel width too large: channel ", c));
    }

    if (config_.congestion.enabled) {
        require(config_.congestion.node_buffer_limit > 0, "node_buffer_limit must be non-zero");
        require(config_.congestion.high_watermark <= config_.congestion.node_buffer_limit,
                "high_watermark must be <= node_buffer_limit");
        require(config_.congestion.low_watermark <= config_.congestion.high_watermark,
                "low_watermark must be <= high_watermark");
    }
}

void SymmetricMultiChannelBus::validate_port(BusPortT port) const {
    require(ports_.find(port) != ports_.end(), str_join("unknown port: ", port));
}

void SymmetricMultiChannelBus::validate_channel(ChannelT channel) const {
    require(channel < cha_cnt_, str_join("unknown channel: ", channel));
}

void SymmetricMultiChannelBus::build_network(
    const std::vector<BusPortT>& port_ids,
    const std::vector<BusNodeT>& port_to_node) {
    std::set<BusPortT> unique_ports;
    std::set<BusNodeT> node_ids;
    for (size_t i = 0; i < port_ids.size(); ++i) {
        require(unique_ports.insert(port_ids[i]).second, str_join("duplicate port id: ", port_ids[i]));
        port2node_[port_ids[i]] = port_to_node[i];
        node_ids.insert(port_to_node[i]);
    }

    std::vector<BusNodeT> nodes_vec(node_ids.begin(), node_ids.end());
    assert_route_table_valid(nodes_vec, route_);

    for (BusNodeT node_id : node_ids) {
        auto [it, inserted] = nodes_.emplace(node_id, NodeStruct{});
        (void)inserted;
        it->second.myid = node_id;
    }

    for (const auto& [port, node] : port2node_) {
        auto [port_it, inserted] = ports_.emplace(port, PortStruct{});
        (void)inserted;
        port_it->second.send_buf.resize(cha_cnt_);
        port_it->second.recv_buf.resize(cha_cnt_);
        nodes_.at(node).port.emplace(port, &port_it->second);
    }

    for (auto& [_, node] : nodes_) {
        node.sd_ptr = node.port.begin();
    }

    std::set<std::pair<BusNodeT, BusNodeT>> all_edges;
    for (const auto& [src, dst_map] : route_) {
        for (const auto& [dst, next] : dst_map) {
            if (src == dst || next == src) {
                continue;
            }
            require(nodes_.find(src) != nodes_.end(), str_join("route src is not a known node: ", src));
            require(nodes_.find(next) != nodes_.end(), str_join("route next hop is not a known node: ", next));
            all_edges.emplace(src, next);
            all_edges.emplace(next, src);  // keep full-duplex pair available, matching original bus model style
        }
    }

    edges_.resize(all_edges.size());
    size_t index = 0;
    for (const auto& [from, to] : all_edges) {
        EdgeInChannel& edge = edges_[index++];
        edge.input.assign(cha_cnt_, nullptr);
        edge.output.assign(cha_cnt_, nullptr);
        edge.from = from;
        edge.to = to;
        nodes_.at(from).txs.emplace(to, &edge);
        nodes_.at(to).rxs.push_back(&edge);
    }
}

void SymmetricMultiChannelBus::can_send(BusPortT port, std::vector<bool>& out) const {
    validate_port(port);
    out.assign(cha_cnt_, false);
    for (ChannelT c = 0; c < cha_cnt_; ++c) {
        out[c] = can_send(port, c);
    }
}

bool SymmetricMultiChannelBus::can_send(BusPortT port, ChannelT channel) const {
    validate_port(port);
    validate_channel(channel);

    const PortStruct& p = ports_.at(port);
    if (!p.send_buf[channel].empty()) {
        return false;
    }

    if (config_.congestion.enabled) {
        const BusNodeT node_id = port2node_.at(port);
        const NodeStruct& node = nodes_.at(node_id);
        if (node.congested || node.buffered_packets >= config_.congestion.node_buffer_limit) {
            return false;
        }
    }

    return true;
}

bool SymmetricMultiChannelBus::send(
    BusPortT port,
    BusPortT dst_port,
    ChannelT channel,
    const std::vector<uint8_t>& data) {
    validate_port(port);
    validate_port(dst_port);
    validate_channel(channel);

    if (!can_send(port, channel)) {
        return false;
    }

    const uint32_t channel_width = cha_widths_[channel];
    const uint32_t padded_len = align_up(static_cast<uint32_t>(std::max<size_t>(data.size(), 1)), channel_width);
    const uint32_t pac_cnt = padded_len / channel_width;
    require(pac_cnt <= UINT16_MAX, "message is too large for 16-bit packet index/count fields");

    PortStruct& p = ports_.at(port);
    std::list<MsgPack*>& sbuf = p.send_buf[channel];
    const XmitIDT xmt = xmtid_alloc_++;
    uint32_t pos = 0;

    for (uint32_t i = 0; i < pac_cnt; ++i) {
        auto* pack = new MsgPack();
        pack->src = port;
        pack->dst = dst_port;
        pack->tgt = port2node_.at(dst_port);
        pack->cha = channel;
        pack->xmtid = xmt;
        pack->len = static_cast<uint32_t>(data.size());
        pack->pac_idx = static_cast<uint16_t>(i);
        pack->pac_cnt = static_cast<uint16_t>(pac_cnt);
        pack->tx_start_tick = current_tick_;
        pack->data.resize(channel_width, 0);

        const uint32_t copy_len = static_cast<uint32_t>(
            std::min<size_t>(channel_width, data.size() > pos ? data.size() - pos : 0));
        if (copy_len > 0) {
            std::copy_n(data.begin() + pos, copy_len, pack->data.begin());
        }
        pos += channel_width;
        sbuf.push_back(pack);
    }

    return true;
}

void SymmetricMultiChannelBus::can_recv(BusPortT port, std::vector<bool>& out) const {
    validate_port(port);
    out.assign(cha_cnt_, false);
    for (ChannelT c = 0; c < cha_cnt_; ++c) {
        out[c] = can_recv(port, c);
    }
}

bool SymmetricMultiChannelBus::can_recv(BusPortT port, ChannelT channel) const {
    validate_port(port);
    validate_channel(channel);

    const auto& rbuf = ports_.at(port).recv_buf[channel];
    return !rbuf.empty() && rbuf.size() >= rbuf.front()->pac_cnt;
}

bool SymmetricMultiChannelBus::recv(BusPortT port, ChannelT channel, std::vector<uint8_t>& buf) {
    validate_port(port);
    validate_channel(channel);

    std::list<MsgPack*>& rbuf = ports_.at(port).recv_buf[channel];
    if (rbuf.empty() || rbuf.size() < rbuf.front()->pac_cnt) {
        return false;
    }

    const uint32_t pac_cnt = rbuf.front()->pac_cnt;
    const uint32_t channel_width = cha_widths_[channel];
    const uint32_t len = rbuf.front()->len;
    std::vector<uint8_t> out;
    out.reserve(static_cast<size_t>(pac_cnt) * channel_width);

    for (uint32_t i = 0; i < pac_cnt; ++i) {
        MsgPack* pack = rbuf.front();
        require(pack->pac_idx == i, str_join("packet order error at idx ", i));
        out.insert(out.end(), pack->data.begin(), pack->data.end());

        stats_.transmitted_packets++;
        stats_.packet_latency_sum += current_tick_ - pack->tx_start_tick;
        const uint64_t key = (static_cast<uint64_t>(pack->src) << 32) | pack->dst;
        transmit_cnt_[key]++;

        delete pack;
        rbuf.pop_front();
    }

    out.resize(len);
    buf.swap(out);
    stats_.received_messages++;
    return true;
}

void SymmetricMultiChannelBus::apply_next_tick() {
    for (auto& [_, node] : nodes_) {
        process_node(node);
    }
    for (auto& edge : edges_) {
        process_edge(edge);
    }
    refresh_all_congestion_states();
    ++current_tick_;
    stats_.tick = current_tick_;
}

void SymmetricMultiChannelBus::clear_statistics() {
    stats_ = NocStatistics{};
    stats_.tick = current_tick_;
    transmit_cnt_.clear();
    for (auto& [_, node] : nodes_) {
        node.busy_cycles = 0;
        node.passed_packs = 0;
        node.congested_cycles = 0;
        node.blocked_cycles = 0;
    }
    for (auto& edge : edges_) {
        edge.busy_cycles = 0;
    }
}

NocStatistics SymmetricMultiChannelBus::statistics() const {
    NocStatistics out = stats_;
    out.tick = current_tick_;
    out.blocked_cycles = 0;
    out.congested_cycles = 0;
    out.edge_busy_cycles = 0;
    for (const auto& [_, node] : nodes_) {
        out.blocked_cycles += node.blocked_cycles;
        out.congested_cycles += node.congested_cycles;
    }
    for (const auto& edge : edges_) {
        out.edge_busy_cycles += edge.busy_cycles;
    }
    return out;
}

uint32_t SymmetricMultiChannelBus::node_buffered_packets(BusNodeT node) const {
    auto it = nodes_.find(node);
    require(it != nodes_.end(), str_join("unknown node: ", node));
    return it->second.buffered_packets;
}

bool SymmetricMultiChannelBus::node_congested(BusNodeT node) const {
    auto it = nodes_.find(node);
    require(it != nodes_.end(), str_join("unknown node: ", node));
    return it->second.congested;
}

void SymmetricMultiChannelBus::print_setup_info(std::ostream& out) const {
    out << "NoC name: " << name_ << '\n';
    out << "channels: " << cha_cnt_ << '\n';
    out << "link_width_byte: " << link_width_byte_ << '\n';
    out << "route_latency: " << route_latency_ << '\n';
    out << "congestion.enabled: " << config_.congestion.enabled << '\n';
    out << "congestion.node_buffer_limit: " << config_.congestion.node_buffer_limit << '\n';
    for (const auto& [port, node] : port2node_) {
        out << "port " << port << " -> node " << node << '\n';
    }
    for (const auto& edge : edges_) {
        out << "edge " << edge.from << " -> " << edge.to << '\n';
    }
}

void SymmetricMultiChannelBus::print_statistics(std::ostream& out) const {
    const auto s = statistics();
    out << "tick: " << s.tick << '\n';
    out << "received_messages: " << s.received_messages << '\n';
    out << "transmitted_packets: " << s.transmitted_packets << '\n';
    out << "average_packet_latency: " << s.average_packet_latency() << '\n';
    out << "blocked_cycles: " << s.blocked_cycles << '\n';
    out << "congested_cycles: " << s.congested_cycles << '\n';
    out << "edge_busy_cycles: " << s.edge_busy_cycles << '\n';
}

bool SymmetricMultiChannelBus::can_accept_local_packet(NodeStruct& node) const {
    (void)node;
    // Destination reassembly/receive buffers are intentionally unbounded,
    // matching the original simulator simplification. Bounding them would
    // deadlock multi-packet messages when the destination cannot assemble the
    // full message before hitting the limit. Congestion control is therefore
    // applied to forwarding/router pipeline buffers.
    return true;
}

bool SymmetricMultiChannelBus::can_accept_forward_packet(NodeStruct& node) const {
    if (!config_.congestion.enabled) {
        return true;
    }
    return node.buffered_packets < config_.congestion.node_buffer_limit;
}

bool SymmetricMultiChannelBus::can_accept_candidate(NodeStruct& node, const Candidate& cand) const {
    if (!cand.pack) {
        return false;
    }
    if (cand.pack->tgt == node.myid) {
        return can_accept_local_packet(node);
    }
    return can_accept_forward_packet(node);
}

SymmetricMultiChannelBus::Candidate SymmetricMultiChannelBus::find_candidate(NodeStruct& node) {
    Candidate cand;

    if (!node.rxs.empty()) {
        for (size_t i = 0; i < node.rxs.size(); ++i) {
            EdgeInChannel* edge = node.rxs[node.rx_ptr];
            for (ChannelT c = 0; c < cha_cnt_; ++c) {
                if (edge->output[c] != nullptr) {
                    cand.source = Candidate::Source::EdgeOutput;
                    cand.pack = edge->output[c];
                    cand.edge = edge;
                    cand.channel = c;
                    return cand;
                }
            }
            node.rx_ptr = (node.rx_ptr + 1) % node.rxs.size();
        }
    }

    if (!node.port.empty()) {
        for (size_t i = 0; i < node.port.size(); ++i) {
            PortStruct* p = node.sd_ptr->second;
            for (ChannelT c = 0; c < cha_cnt_; ++c) {
                auto& l = p->send_buf[c];
                if (!l.empty()) {
                    cand.source = Candidate::Source::PortSend;
                    cand.pack = l.front();
                    cand.port = p;
                    cand.list = &l;
                    cand.channel = c;
                    return cand;
                }
            }
            ++node.sd_ptr;
            if (node.sd_ptr == node.port.end()) {
                node.sd_ptr = node.port.begin();
            }
        }
    }

    return cand;
}

void SymmetricMultiChannelBus::detach_candidate(NodeStruct& node, Candidate& cand) {
    switch (cand.source) {
        case Candidate::Source::EdgeOutput:
            require(cand.edge != nullptr, "bad edge candidate");
            require(cand.edge->output[cand.channel] == cand.pack, "edge candidate changed before detach");
            cand.edge->output[cand.channel] = nullptr;
            if (!node.rxs.empty()) {
                node.rx_ptr = (node.rx_ptr + 1) % node.rxs.size();
            }
            break;
        case Candidate::Source::PortSend:
            require(cand.list != nullptr, "bad port candidate");
            require(!cand.list->empty() && cand.list->front() == cand.pack, "port candidate changed before detach");
            cand.list->pop_front();
            if (!node.port.empty()) {
                ++node.sd_ptr;
                if (node.sd_ptr == node.port.end()) {
                    node.sd_ptr = node.port.begin();
                }
            }
            break;
        case Candidate::Source::None:
            break;
    }
}

void SymmetricMultiChannelBus::admit_candidate(NodeStruct& node, MsgPack* pack) {
    if (pack->tgt == node.myid) {
        deliver_to_local_port(node, pack);
        node.passed_packs++;
        return;
    }

    node.pipeline.push_back(PipelinedPack{pack, route_latency_});
    if (config_.congestion.enabled) {
        node.buffered_packets++;
    }
}

void SymmetricMultiChannelBus::deliver_to_local_port(NodeStruct& node, MsgPack* pack) {
    const XmitIDT xmt = pack->xmtid;
    const ChannelT cha = pack->cha;
    const uint32_t cnt = pack->pac_cnt;
    const BusPortT dst = pack->dst;

    auto port_it = node.port.find(dst);
    require(port_it != node.port.end(), str_join("destination port ", dst, " is not attached to node ", node.myid));

    if (cnt <= 1) {
        port_it->second->recv_buf[cha].push_back(pack);
        return;
    }

    auto& l = node.order_buf[xmt];
    auto iter = l.begin();
    for (; iter != l.end() && (*iter)->pac_idx < pack->pac_idx; ++iter) {
    }
    l.insert(iter, pack);

    if (l.size() == pack->pac_cnt) {
        port_it->second->recv_buf[cha].splice(port_it->second->recv_buf[cha].end(), l);
        node.order_buf.erase(xmt);
    }
}

void SymmetricMultiChannelBus::process_node(NodeStruct& node) {
    bool busy = false;

    Candidate cand = find_candidate(node);
    if (cand.pack != nullptr) {
        if (can_accept_candidate(node, cand)) {
            detach_candidate(node, cand);
            admit_candidate(node, cand.pack);
            busy = true;
        } else {
            node.blocked_cycles++;
            busy = true;
        }
    }

    for (auto iter = node.pipeline.begin(); iter != node.pipeline.end();) {
        if (iter->cycle_remained > 0) {
            --iter->cycle_remained;
            ++iter;
            busy = true;
            continue;
        }

        MsgPack* p = iter->pack;
        const BusNodeT next = next_hop(node.myid, p->tgt);
        auto tx_it = node.txs.find(next);
        require(tx_it != node.txs.end(), str_join("missing edge ", node.myid, " -> ", next));
        EdgeInChannel* edge = tx_it->second;
        const ChannelT cha = p->cha;
        if (edge->input[cha] == nullptr) {
            edge->input[cha] = p;
            iter = node.pipeline.erase(iter);
            if (config_.congestion.enabled) {
                require(node.buffered_packets > 0, "router buffer underflow on pipeline release");
                node.buffered_packets--;
            }
            busy = true;
        } else {
            node.blocked_cycles++;
            ++iter;
            busy = true;
        }
    }

    update_congestion_state(node);
    if (node.congested) {
        node.congested_cycles++;
    }
    if (busy) {
        node.busy_cycles++;
    }
}

void SymmetricMultiChannelBus::process_edge(EdgeInChannel& edge) {
    uint32_t total_sz = 0;
    for (ChannelT c = 0; c < cha_cnt_ && total_sz < link_width_byte_; ++c) {
        if (edge.input[c] != nullptr && edge.output[c] == nullptr) {
            const uint32_t packet_sz = static_cast<uint32_t>(edge.input[c]->data.size());
            if (total_sz > 0 && total_sz + packet_sz > link_width_byte_) {
                continue;
            }
            edge.output[c] = edge.input[c];
            edge.input[c] = nullptr;
            total_sz += packet_sz;
            edge.busy_cycles++;
        }
    }
}

void SymmetricMultiChannelBus::update_congestion_state(NodeStruct& node) {
    if (!config_.congestion.enabled) {
        node.congested = false;
        return;
    }

    if (!node.congested && node.buffered_packets >= config_.congestion.high_watermark) {
        node.congested = true;
    } else if (node.congested && node.buffered_packets <= config_.congestion.low_watermark) {
        node.congested = false;
    }
}

void SymmetricMultiChannelBus::refresh_all_congestion_states() {
    for (auto& [_, node] : nodes_) {
        update_congestion_state(node);
    }
}

BusNodeT SymmetricMultiChannelBus::next_hop(BusNodeT src, BusNodeT dst) const {
    if (src == dst) {
        return src;
    }
    auto src_it = route_.find(src);
    require(src_it != route_.end(), str_join("route source not found: ", src));
    auto dst_it = src_it->second.find(dst);
    require(dst_it != src_it->second.end(), str_join("route entry not found: ", src, " -> ", dst));
    return dst_it->second;
}

void SymmetricMultiChannelBus::delete_all_packets() {
    std::set<MsgPack*> freed;
    auto free_pack = [&](MsgPack* p) {
        if (p && freed.insert(p).second) {
            delete p;
        }
    };

    for (auto& [_, port] : ports_) {
        for (auto& q : port.send_buf) {
            for (auto* p : q) free_pack(p);
            q.clear();
        }
        for (auto& q : port.recv_buf) {
            for (auto* p : q) free_pack(p);
            q.clear();
        }
    }

    for (auto& [_, node] : nodes_) {
        for (auto& pipe_pack : node.pipeline) {
            free_pack(pipe_pack.pack);
        }
        node.pipeline.clear();
        for (auto& [__, q] : node.order_buf) {
            for (auto* p : q) free_pack(p);
        }
        node.order_buf.clear();
    }

    for (auto& edge : edges_) {
        for (auto*& p : edge.input) {
            free_pack(p);
            p = nullptr;
        }
        for (auto*& p : edge.output) {
            free_pack(p);
            p = nullptr;
        }
    }
}

}  // namespace simbus
