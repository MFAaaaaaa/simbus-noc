#include "manycore/manycore_system.hpp"

#include "simbus/routetable.h"

#include <stdexcept>

namespace manycore {

std::vector<simbus::BusPortT> ManycoreSystem::make_ports(uint32_t core_count) {
  std::vector<simbus::BusPortT> ports;
  for (uint32_t i = 0; i < core_count; ++i) ports.push_back(i);
  ports.push_back(core_count);
  return ports;
}

std::vector<simbus::BusNodeT> ManycoreSystem::make_nodes(uint32_t core_count) {
  std::vector<simbus::BusNodeT> nodes;
  for (uint32_t i = 0; i <= core_count; ++i) nodes.push_back(i);
  return nodes;
}

simbus::BusRouteTable ManycoreSystem::make_route(uint32_t core_count) {
  simbus::BusRouteTable route;
  simbus::genroute_double_ring(make_nodes(core_count), route);
  return route;
}

simbus::NocConfig ManycoreSystem::make_noc_config(const ManycoreConfig& config) {
  simbus::NocConfig noc;
  noc.link_width_byte = config.link_width_bytes;
  noc.route_latency = config.route_latency;
  noc.congestion.enabled = true;
  noc.congestion.node_buffer_limit = config.router_buffer_limit;
  noc.congestion.high_watermark = config.router_buffer_limit;
  noc.congestion.low_watermark = config.router_buffer_limit / 2;
  return noc;
}

ManycoreSystem::ManycoreSystem(ManycoreConfig config)
    : config_(config),
      memory_port_(config.core_count),
      noc_(make_ports(config.core_count), make_nodes(config.core_count), {config.link_width_bytes,
           config.link_width_bytes}, make_route(config.core_count), make_noc_config(config),
           "manycore-noc"),
      memory_controller_(noc_, memory_port_,
                         MemoryControllerConfig{config.memory_latency,
                                                config.memory_queue_capacity}) {
  if (config.core_count == 0 || config.router_buffer_limit == 0) {
    throw std::runtime_error("manycore system requires cores and router buffers");
  }
  for (uint32_t i = 0; i < config.core_count; ++i) {
    cores_.emplace_back(new AtomicCore(i, noc_, i, memory_port_));
  }
}

void ManycoreSystem::tick() {
  memory_controller_.tick();
  for (auto& core : cores_) core->tick();
  noc_.apply_next_tick();
  ++current_tick_;
}

bool ManycoreSystem::finished() const {
  for (const auto& core : cores_) {
    if (!core->halted()) return false;
  }
  return memory_controller_.idle();
}

bool ManycoreSystem::run(uint64_t max_ticks) {
  while (current_tick_ < max_ticks && !finished()) tick();
  return finished();
}

}  // namespace manycore

