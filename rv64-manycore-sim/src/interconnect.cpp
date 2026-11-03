#include "manycore/interconnect.hpp"
#include "simbus/routetable.h"
#include <numeric>
namespace manycore {
namespace {
std::vector<uint32_t> ports(uint32_t n){std::vector<uint32_t> p(n+1);std::iota(p.begin(),p.end(),0);return p;}
simbus::BusRouteTable route(uint32_t n){simbus::BusRouteTable r;simbus::genroute_double_ring(ports(n),r);return r;}
}
RingNoC::RingNoC(uint32_t cores,simbus::NocConfig c):noc_(ports(cores),ports(cores),{c.link_width_byte,c.link_width_byte},route(cores),c,"ring-noc"){}
SharedBus::SharedBus(uint32_t n,SharedBusConfig c):config_(c),pending_(n),inbox_(n,std::vector<std::deque<Message>>(2)),reserved_(n,std::vector<uint64_t>(2)),stats_(n){
 if(!n||!c.address_cycles||!c.data_cycles||!c.queue_depth||!c.starvation_threshold)throw std::runtime_error("invalid shared bus config");
}
void SharedBus::check(uint32_t p,uint32_t c)const{if(p>=pending_.size()||c>1)throw std::out_of_range("bus port/channel");}
bool SharedBus::can_send(uint32_t p,uint32_t c)const{check(p,c);return pending_[p].size()<config_.queue_depth;}
void SharedBus::can_send(uint32_t p,std::vector<bool>& o)const{o={can_send(p,0),can_send(p,1)};}
bool SharedBus::send(uint32_t p,uint32_t d,uint32_t c,const std::vector<uint8_t>& b){
 check(p,c);check(d,c);if(!can_send(p,c))return false;
 if(config_.receive_slots&&reserved_[d][c]>=config_.receive_slots){++credit_stalls_;return false;}
 pending_[p].push_back({p,d,c,b,tick_});++reserved_[d][c];max_reserved_=std::max(max_reserved_,reserved_[d][c]);++stats_[p].requests;return true;
}
bool SharedBus::can_recv(uint32_t p,uint32_t c)const{check(p,c);return !inbox_[p][c].empty();}
void SharedBus::can_recv(uint32_t p,std::vector<bool>& o)const{o={can_recv(p,0),can_recv(p,1)};}
bool SharedBus::recv(uint32_t p,uint32_t c,std::vector<uint8_t>& b){if(!can_recv(p,c))return false;b=std::move(inbox_[p][c].front().bytes);inbox_[p][c].pop_front();--reserved_[p][c];return true;}
void SharedBus::apply_next_tick(){
 if(!active_){
  // Responses have priority in both policies to avoid protocol starvation.
  // Requests then use round-robin or fixed ascending master ID priority.
  int selected=-1;
  for(int channel=1;channel>=0&&selected<0;--channel)for(uint32_t k=0;k<pending_.size();++k){
   auto p=config_.fixed_priority?k:(next_+k)%pending_.size();
   if(!pending_[p].empty()&&int(pending_[p].front().channel)==channel){selected=int(p);break;}
  }
  if(selected>=0){auto p=uint32_t(selected);active_=std::move(pending_[p].front());pending_[p].pop_front();
   auto w=tick_-active_->enqueued;stats_[p].grant_wait.add(w);stats_[p].longest_wait=std::max(stats_[p].longest_wait,w);++stats_[p].grants;
   // A response must not reset request round-robin to master 0.
   if(active_->channel==0)next_=(p+1)%pending_.size();
   remaining_=uint64_t(config_.address_cycles)+config_.data_cycles;
  }
 }
 for(uint32_t p=0;p<pending_.size();++p)for(auto& m:pending_[p]){
  auto w=tick_+1-m.enqueued;++stats_[p].wait_cycles;stats_[p].longest_wait=std::max(stats_[p].longest_wait,w);
  if(w>=config_.starvation_threshold&&!m.warned){m.warned=true;++stats_[p].starvation_events;}
 }
 if(active_){++busy_cycles;if(--remaining_==0){latency_sum_+=tick_+1-active_->enqueued;auto d=active_->dst,c=active_->channel;inbox_[d][c].push_back(std::move(*active_));active_.reset();++delivered_;}}
 else ++idle_cycles;
 ++tick_;
}
simbus::NocStatistics SharedBus::statistics()const{
 simbus::NocStatistics n;n.tick=tick_;n.transmitted_packets=delivered_;n.packet_latency_sum=latency_sum_;n.destination_credit_stalls=credit_stalls_;n.max_reserved_messages=max_reserved_;
 for(auto& s:stats_)n.blocked_cycles+=s.wait_cycles;
 return n;
}
}
