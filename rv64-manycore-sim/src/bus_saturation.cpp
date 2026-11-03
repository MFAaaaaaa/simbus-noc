#include "manycore/interconnect.hpp"
#include "manycore/memory_controller.hpp"
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
using namespace manycore;
// Open-loop traffic, not a Hart performance benchmark. Backlog and outstanding
// are bounded and drops/censored requests are explicitly reported.
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("usage: bus_saturation output.csv (must not exist)");
 {std::ifstream old(argv[1]);if(old.good())throw std::runtime_error("refuse to overwrite output");}
 std::ofstream out(argv[1]);if(!out)throw std::runtime_error("cannot create results");
 out<<"policy,profile,offered_load,observed_offered,throughput,mean_latency,p99_latency,bus_utilization,memory_utilization,memory_queue_full,master,master_completed,grant_wait_mean,grant_wait_p99,max_wait,starvation_events,drops,censored\n";
 for(bool fixed:{false,true})for(bool hot:{false,true})for(double offered:{0.02,0.04,0.08,0.10,0.12,0.14,0.18,0.25,0.40}){
  SharedBusConfig c;c.fixed_priority=fixed;c.queue_depth=64;c.starvation_threshold=1000;
  SharedBus bus(5,c);MemoryController memory(bus,4,{1,64});
  std::mt19937 rng(20261103);std::uniform_real_distribution<double> random(0,1);
  struct Job{uint32_t id;uint64_t born;};
  std::vector<std::deque<Job>> queues(4);std::unordered_map<uint32_t,uint64_t> born;
  std::vector<uint64_t> done(4);Distribution latency;
  uint32_t id=1;uint64_t arrivals=0,drops=0,busy0=0,service0=0,queue0=0;
  constexpr uint64_t warm=2000,ticks=22000;
  for(uint64_t t=0;t<ticks;++t){
   if(t==warm){busy0=bus.busy_cycles;service0=memory.stats().service_cycles;queue0=memory.stats().queue_full_cycles;}
   memory.tick();
   for(uint32_t h=0;h<4;++h){
    const double weight=hot?(h==0?0.8:0.2/3):0.25;
    if(random(rng)<offered*weight){if(t>=warm)++arrivals;if(queues[h].size()+born.size()<4096){queues[h].push_back({id++,t});}else if(t>=warm)++drops;}
    if(!queues[h].empty()){
     auto j=queues[h].front();MemoryRequest req;req.hart_id=h;req.transaction_id=j.id;req.address=0x6000+8*h;req.op=MemoryOp::Write;req.size=8;req.data.assign(8,42);
     if(bus.can_send(h,0)&&bus.send(h,4,0,encode_request(req))){born[j.id]=j.born;queues[h].pop_front();}
    }
    std::vector<uint8_t> bytes;
    if(bus.recv(h,1,bytes)){auto r=decode_response(bytes);if(r.status!=MemoryStatus::Ok)throw std::runtime_error("memory error");auto b=born.at(r.transaction_id);born.erase(r.transaction_id);if(t>=warm){++done[h];latency.add(t-b);}}
   }
   bus.apply_next_tick();
  }
  const double window=ticks-warm;uint64_t completed=0,censored=born.size();for(auto n:done)completed+=n;for(auto& q:queues)censored+=q.size();
  for(uint32_t h=0;h<4;++h){auto& s=bus.masters()[h];out<<(fixed?"fixed":"rr")<<','<<(hot?"hot":"balanced")<<','<<offered<<','<<arrivals/window<<','<<completed/window<<','<<latency.mean()<<','<<latency.p99()<<','<<(bus.busy_cycles-busy0)/window<<','<<(memory.stats().service_cycles-service0)/window<<','<<memory.stats().queue_full_cycles-queue0<<','<<h<<','<<done[h]<<','<<s.grant_wait.mean()<<','<<s.grant_wait.p99()<<','<<s.longest_wait<<','<<s.starvation_events<<','<<drops<<','<<censored<<'\n';}
 }
 std::cout<<"SATURATION_PASS\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
