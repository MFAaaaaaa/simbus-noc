#include "manycore/rv64_manycore_system.hpp"

#include "simbus/routetable.h"

#include <stdexcept>

namespace manycore {

std::vector<simbus::BusPortT> Rv64ManycoreSystem::make_ports(uint32_t core_count) {
  std::vector<simbus::BusPortT> ports;
  for (uint32_t i = 0; i <= core_count; ++i) ports.push_back(i);
  return ports;
}

std::vector<simbus::BusNodeT> Rv64ManycoreSystem::make_nodes(uint32_t core_count) {
  std::vector<simbus::BusNodeT> nodes;
  for (uint32_t i = 0; i <= core_count; ++i) nodes.push_back(i);
  return nodes;
}

simbus::BusRouteTable Rv64ManycoreSystem::make_route(uint32_t core_count) {
  simbus::BusRouteTable route;
  simbus::genroute_double_ring(make_nodes(core_count), route);
  return route;
}

simbus::NocConfig Rv64ManycoreSystem::make_noc_config(const ManycoreConfig& config) {
  simbus::NocConfig noc;
  noc.link_width_byte = config.link_width_bytes;
  noc.route_latency = config.route_latency;
  noc.congestion.enabled = true;
  noc.congestion.node_buffer_limit = config.router_buffer_limit;
  noc.congestion.high_watermark = config.router_buffer_limit;
  noc.congestion.low_watermark = config.router_buffer_limit / 2;
  return noc;
}

Rv64ManycoreSystem::Rv64ManycoreSystem(ProgramImage program, ManycoreConfig config,
                                       InstructionLatencyConfig latency)
    : program_(std::move(program)), config_(config), memory_port_(config.core_count),
      noc_(make_ports(config.core_count), make_nodes(config.core_count),
           {config.link_width_bytes, config.link_width_bytes}, make_route(config.core_count),
           make_noc_config(config), "rv64-manycore-noc"),
      memory_controller_(noc_, memory_port_,
                         MemoryControllerConfig{config.memory_latency,
                                                config.memory_queue_capacity}) {
  if (config.core_count == 0) throw std::runtime_error("RV64 system needs at least one core");
  for (uint32_t hart = 0; hart < config.core_count; ++hart) {
    cores_.emplace_back(new Rv64AtomicCore(hart, noc_, hart, memory_port_, program_, latency,
                                           0x88000000ull, config.tohost_address));
  }
}

void Rv64ManycoreSystem::tick() {
  memory_controller_.tick();
  for (auto& core : cores_) core->tick();
  noc_.apply_next_tick();
  ++current_tick_;
}

bool Rv64ManycoreSystem::finished() const {
  for (const auto& core : cores_) if (!core->finished()) return false;
  return memory_controller_.idle();
}

bool Rv64ManycoreSystem::faulted() const {
  for (const auto& core : cores_) if (core->faulted()) return true;
  return false;
}

bool Rv64ManycoreSystem::program_succeeded() const {
  if (!finished() || faulted()) return false;
  for (const auto& core : cores_) if (core->exit_code() != 0) return false;
  return true;
}

bool Rv64ManycoreSystem::run(uint64_t max_ticks) {
  while (current_tick_ < max_ticks && !finished() && !faulted()) tick();
  return finished() && !faulted();
}

}  // namespace manycore
