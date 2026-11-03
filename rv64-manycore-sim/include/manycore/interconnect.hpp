#pragma once
#include "simbus/symmulcha.h"
#include <deque>
#include <map>
#include <optional>
#include <stdexcept>

namespace manycore {
// Reuse the existing protocol so Hart/controller call sites remain unchanged.
class IBus : public simbus::BusInterfaceV2 {
 public: virtual simbus::NocStatistics statistics() const = 0;
};
class RingNoC final : public IBus {
 public:
  RingNoC(uint32_t cores, simbus::NocConfig config);
  void can_send(uint32_t p,std::vector<bool>& o) const override {noc_.can_send(p,o);}
  bool can_send(uint32_t p,uint32_t c) const override {return noc_.can_send(p,c);}
  bool send(uint32_t p,uint32_t d,uint32_t c,const std::vector<uint8_t>& b) override {return noc_.send(p,d,c,b);}
  void can_recv(uint32_t p,std::vector<bool>& o) const override {noc_.can_recv(p,o);}
  bool can_recv(uint32_t p,uint32_t c) const override {return noc_.can_recv(p,c);}
  bool recv(uint32_t p,uint32_t c,std::vector<uint8_t>& b) override {return noc_.recv(p,c,b);}
  void apply_next_tick() override {noc_.apply_next_tick();}
  simbus::NocStatistics statistics() const override {return noc_.statistics();}
 private: simbus::SymmetricMultiChannelBus noc_;
};
struct Distribution {
  uint64_t count=0,sum=0,maximum=0;
  std::map<uint64_t,uint64_t> histogram;
  void add(uint64_t n){++count;sum+=n;maximum=std::max(maximum,n);++histogram[n];}
  double mean()const{return count?double(sum)/count:0;}
  uint64_t p99()const {uint64_t n=0;for(auto [v,c]:histogram){n+=c;if(n>=count-count/100)return v;}return 0;}
};
struct MasterBusStats {
  uint64_t requests=0,grants=0,wait_cycles=0,longest_wait=0,starvation_events=0;
  Distribution grant_wait;
};
struct SharedBusConfig {
  uint32_t address_cycles=1,data_cycles=3,queue_depth=64,receive_slots=0;
  uint64_t starvation_threshold=1000;
  bool fixed_priority=false;
};
class SharedBus final : public IBus {
 public:
  SharedBus(uint32_t ports,SharedBusConfig config={});
  void can_send(uint32_t p,std::vector<bool>& o)const override;
  bool can_send(uint32_t p,uint32_t c)const override;
  bool send(uint32_t p,uint32_t d,uint32_t c,const std::vector<uint8_t>& b)override;
  void can_recv(uint32_t p,std::vector<bool>& o)const override;
  bool can_recv(uint32_t p,uint32_t c)const override;
  bool recv(uint32_t p,uint32_t c,std::vector<uint8_t>& b)override;
  void apply_next_tick()override;
  simbus::NocStatistics statistics()const override;
  uint64_t busy_cycles=0,idle_cycles=0;
  double utilization()const{return busy_cycles+idle_cycles?double(busy_cycles)/(busy_cycles+idle_cycles):0;}
  const std::vector<MasterBusStats>& masters()const{return stats_;}
 private:
  struct Message {uint32_t src,dst,channel;std::vector<uint8_t> bytes;uint64_t enqueued;bool warned=false;};
  void check(uint32_t p,uint32_t c)const;
  SharedBusConfig config_;uint64_t tick_=0,remaining_=0,delivered_=0,credit_stalls_=0,latency_sum_=0,max_reserved_=0;
  uint32_t next_=0;
  std::vector<std::deque<Message>> pending_;
  std::vector<std::vector<std::deque<Message>>> inbox_;
  std::vector<std::vector<uint64_t>> reserved_;
  std::vector<MasterBusStats> stats_;
  std::optional<Message> active_;
};
}
