#include "manycore/rv64_manycore_system.hpp"
#include <iostream>
#include <stdexcept>
#define CHECK(v) do{if(!(v))throw std::runtime_error(#v);}while(0)
using namespace manycore;
int main(int argc,char** argv){try{
 CHECK(argc==2);InstructionLatencyConfig c;load_instruction_timing(argv[1],c);
 rv64archsem::DecodedInst i; i.instName=rv64archsem::InstName::vadd_vv;i.opcode=rv64archsem::OPCode::opv;
 CHECK(instruction_latency(i,c,8)==3);CHECK(instruction_latency(i,c,256)==34);
 c.instructions["vadd.vv"]={7,0};CHECK(instruction_latency(i,c,256)==7);
 c.instructions.clear();c.vector_lanes=0;CHECK(instruction_latency(i,c,256)==4);
 auto image=ProgramImage::from_words({0x00100093u,0x00208093u,0x00100073u});
 ManycoreConfig cfg;cfg.core_count=1;InstructionLatencyConfig fast;
 Rv64ManycoreSystem a(image,cfg,fast);CHECK(a.run(100));fast.instructions["addi"]={5,0};
 Rv64ManycoreSystem slow(image,cfg,fast);CHECK(slow.run(100));CHECK(slow.current_tick()==a.current_tick()+8);
 SharedBusConfig bc;bc.address_cycles=1;bc.data_cycles=1;SharedBus b(3,bc);
 CHECK(b.send(0,2,0,{1}));CHECK(b.send(1,2,0,{2}));
 CHECK(!b.can_recv(2,0));b.apply_next_tick();CHECK(!b.can_recv(2,0));b.apply_next_tick();
 std::vector<uint8_t> bytes;CHECK(b.recv(2,0,bytes));CHECK(bytes[0]==1);b.apply_next_tick();b.apply_next_tick();CHECK(b.recv(2,0,bytes));CHECK(bytes[0]==2);CHECK(b.busy_cycles==4);CHECK(b.masters()[1].grant_wait.mean()==2);
 bc.receive_slots=1;SharedBus bounded(3,bc);CHECK(bounded.send(0,2,0,{1}));CHECK(!bounded.send(1,2,0,{2}));bounded.apply_next_tick();bounded.apply_next_tick();CHECK(!bounded.send(1,2,0,{2}));CHECK(bounded.recv(2,0,bytes));CHECK(bounded.send(1,2,0,{2}));
 bc.receive_slots=0;SharedBus alternating(3,bc);
 CHECK(alternating.send(0,2,0,{1}));CHECK(alternating.send(0,2,0,{3}));CHECK(alternating.send(1,2,0,{2}));
 alternating.apply_next_tick();alternating.apply_next_tick();CHECK(alternating.recv(2,0,bytes));CHECK(bytes[0]==1);
 CHECK(alternating.send(2,0,1,{9}));alternating.apply_next_tick();alternating.apply_next_tick();CHECK(alternating.recv(0,1,bytes));
 alternating.apply_next_tick();alternating.apply_next_tick();CHECK(alternating.recv(2,0,bytes));CHECK(bytes[0]==2);
 bc.receive_slots=0;bc.fixed_priority=true;bc.starvation_threshold=3;
 SharedBus priority(3,bc);for(int j=0;j<10;++j)CHECK(priority.send(0,2,0,{1}));CHECK(priority.send(1,2,0,{2}));
 for(int j=0;j<22;++j)priority.apply_next_tick();CHECK(priority.masters()[1].starvation_events==1);CHECK(priority.masters()[1].longest_wait==20);
 // Multiple adapter requests and ordered response FIFO, then bounded overflow.
 SharedBus mb(2);MemoryController memory(mb,1,{1,16});MmioBusAdapter adapter(0,mb,0,1);adapter.set_depth(4);
 auto w=[&](uint64_t a,uint64_t v){adapter.write(MmioBusAdapter::base+a,8,v);};
 for(int j=0;j<4;++j){w(0,0x6000+8*j);w(8,100+j);w(24,2);}CHECK(adapter.stats().commands==4);
 w(24,2);CHECK(adapter.stats().errors==1);
 for(int t=0;t<2000&&adapter.busy();++t){memory.tick();adapter.tick();mb.apply_next_tick();}CHECK(!adapter.busy());CHECK(adapter.stats().completed==4);
 for(int j=0;j<4;++j){CHECK(adapter.read(MmioBusAdapter::base+64,8)==(0x80000000u|uint32_t(j)));w(72,1);CHECK(memory.memory().read_u64(0x6000+8*j,8)==uint64_t(100+j));}
 std::cout<<"BUS_TIMING_PASS\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
