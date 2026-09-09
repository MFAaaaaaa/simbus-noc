#include "manycore/manycore_system.hpp"

#include <iomanip>
#include <iostream>

namespace {
std::vector<uint8_t> le64(uint64_t value) {
  std::vector<uint8_t> out(8);
  for (size_t i = 0; i < out.size(); ++i) out[i] = static_cast<uint8_t>(value >> (i * 8));
  return out;
}
}

int main() {
  manycore::ManycoreConfig config;
  config.core_count = 2;
  config.link_width_bytes = 8;
  config.route_latency = 3;
  config.router_buffer_limit = 2;
  config.memory_latency = 12;
  config.memory_queue_capacity = 2;

  manycore::ManycoreSystem system(config);
  system.core(0).load_program({
      manycore::ScriptOp::compute(3),
      manycore::ScriptOp::write(0x1000, le64(0x1111222233334444ull)),
      manycore::ScriptOp::read(0x2000, 8),
      manycore::ScriptOp::halt(),
  });
  system.core(1).load_program({
      manycore::ScriptOp::compute(3),
      manycore::ScriptOp::write(0x2000, le64(0xaaaabbbbccccddddull)),
      manycore::ScriptOp::read(0x1000, 8),
      manycore::ScriptOp::halt(),
  });

  if (!system.run(10000)) {
    std::cerr << "simulation timeout\n";
    return 1;
  }

  std::cout << "ticks=" << system.current_tick() << '\n';
  for (size_t i = 0; i < 2; ++i) {
    const auto& s = system.core(i).stats();
    std::cout << "core" << i << " retired=" << s.retired_ops
              << " compute=" << s.compute_cycles
              << " inject_blocked=" << s.injection_blocked_cycles
              << " memory_wait=" << s.memory_wait_cycles << '\n';
  }
  const auto ns = system.noc().statistics();
  std::cout << "noc packets=" << ns.transmitted_packets
            << " average_packet_latency=" << ns.average_packet_latency()
            << " blocked_cycles=" << ns.blocked_cycles << '\n';
  std::cout << std::hex << "mem[0x1000]=0x"
            << system.memory_controller().memory().read_u64(0x1000)
            << " mem[0x2000]=0x"
            << system.memory_controller().memory().read_u64(0x2000) << '\n';
  return 0;
}

