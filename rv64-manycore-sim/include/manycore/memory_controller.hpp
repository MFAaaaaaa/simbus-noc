#pragma once

#include "manycore/memory_protocol.hpp"
#include "manycore/shared_memory.hpp"
#include "simbus/symmulcha.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>

namespace manycore {

struct MemoryControllerConfig {
  uint32_t service_latency = 8;
  size_t request_queue_capacity = 8;
};

struct MemoryControllerStats {
  uint64_t received_requests = 0;
  uint64_t completed_requests = 0;
  uint64_t response_injection_blocked_cycles = 0;
  uint64_t queue_full_cycles = 0;
  size_t max_queue_depth = 0;
};

class MemoryController {
 public:
  MemoryController(simbus::BusInterfaceV2& bus, simbus::BusPortT port,
                   MemoryControllerConfig config = {});

  void tick();
  SharedMemory& memory() { return memory_; }
  const SharedMemory& memory() const { return memory_; }
  const MemoryControllerStats& stats() const { return stats_; }
  bool idle() const;

 private:
  struct WorkItem {
    MemoryRequest request;
    uint32_t cycles_left = 0;
  };

  void receive_request();
  void start_request();
  void advance_request();
  void send_response();

  simbus::BusInterfaceV2& bus_;
  simbus::BusPortT port_;
  MemoryControllerConfig config_;
  SharedMemory memory_;
  std::deque<MemoryRequest> queue_;
  std::optional<WorkItem> active_;
  std::optional<MemoryResponse> response_;
  MemoryControllerStats stats_;
};

}  // namespace manycore

