#pragma once

#include "manycore/atomic_core.hpp"
#include "manycore/memory_controller.hpp"
#include "simbus/symmulcha.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace manycore {

struct ManycoreConfig {
  uint32_t core_count = 2;
  uint32_t link_width_bytes = 16;
  uint32_t route_latency = 2;
  uint32_t router_buffer_limit = 8;
  uint32_t memory_latency = 8;
  size_t memory_queue_capacity = 8;
  std::optional<uint64_t> tohost_address;
};

class ManycoreSystem {
 public:
  explicit ManycoreSystem(ManycoreConfig config = {});

  AtomicCore& core(size_t index) { return *cores_.at(index); }
  const AtomicCore& core(size_t index) const { return *cores_.at(index); }
  MemoryController& memory_controller() { return memory_controller_; }
  const MemoryController& memory_controller() const { return memory_controller_; }
  const simbus::SymmetricMultiChannelBus& noc() const { return noc_; }

  void tick();
  bool finished() const;
  uint64_t current_tick() const { return current_tick_; }
  bool run(uint64_t max_ticks);

 private:
  static std::vector<simbus::BusPortT> make_ports(uint32_t core_count);
  static std::vector<simbus::BusNodeT> make_nodes(uint32_t core_count);
  static simbus::BusRouteTable make_route(uint32_t core_count);
  static simbus::NocConfig make_noc_config(const ManycoreConfig& config);

  ManycoreConfig config_;
  simbus::BusPortT memory_port_;
  simbus::SymmetricMultiChannelBus noc_;
  MemoryController memory_controller_;
  std::vector<std::unique_ptr<AtomicCore>> cores_;
  uint64_t current_tick_ = 0;
};

}  // namespace manycore
