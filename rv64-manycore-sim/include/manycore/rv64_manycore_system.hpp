#pragma once

#include "manycore/manycore_system.hpp"
#include "manycore/program_image.hpp"
#include "manycore/rv64_atomic_core.hpp"

#include <memory>
#include <vector>

namespace manycore {

class Rv64ManycoreSystem {
 public:
  Rv64ManycoreSystem(ProgramImage program, ManycoreConfig config = {},
                     InstructionLatencyConfig latency = {});

  Rv64AtomicCore& core(size_t index) { return *cores_.at(index); }
  const Rv64AtomicCore& core(size_t index) const { return *cores_.at(index); }
  MemoryController& memory_controller() { return memory_controller_; }
  const MemoryController& memory_controller() const { return memory_controller_; }
  const simbus::SymmetricMultiChannelBus& noc() const { return noc_; }

  void tick();
  bool finished() const;
  bool faulted() const;
  bool program_succeeded() const;
  bool run(uint64_t max_ticks);
  uint64_t current_tick() const { return current_tick_; }

 private:
  static std::vector<simbus::BusPortT> make_ports(uint32_t core_count);
  static std::vector<simbus::BusNodeT> make_nodes(uint32_t core_count);
  static simbus::BusRouteTable make_route(uint32_t core_count);
  static simbus::NocConfig make_noc_config(const ManycoreConfig& config);

  ProgramImage program_;
  ManycoreConfig config_;
  simbus::BusPortT memory_port_;
  simbus::SymmetricMultiChannelBus noc_;
  MemoryController memory_controller_;
  std::vector<std::unique_ptr<Rv64AtomicCore>> cores_;
  uint64_t current_tick_ = 0;
};

}  // namespace manycore
