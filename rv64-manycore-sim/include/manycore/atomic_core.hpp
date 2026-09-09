#pragma once

#include "manycore/memory_protocol.hpp"
#include "simbus/symmulcha.h"

#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

namespace manycore {

enum class ScriptOpKind {
  Compute,
  Read,
  Write,
  Halt,
};

struct ScriptOp {
  ScriptOpKind kind = ScriptOpKind::Compute;
  uint32_t latency = 1;
  uint64_t address = 0;
  uint16_t size = 0;
  std::vector<uint8_t> data;

  static ScriptOp compute(uint32_t latency);
  static ScriptOp read(uint64_t address, uint16_t size, uint32_t issue_latency = 1);
  static ScriptOp write(uint64_t address, std::vector<uint8_t> data,
                        uint32_t issue_latency = 1);
  static ScriptOp halt();
};

enum class CoreState {
  Ready,
  Executing,
  MemoryIssue,
  MemoryWait,
  Halted,
};

struct CoreStats {
  uint64_t ticks = 0;
  uint64_t retired_ops = 0;
  uint64_t compute_cycles = 0;
  uint64_t injection_blocked_cycles = 0;
  uint64_t memory_wait_cycles = 0;
  uint64_t sent_requests = 0;
  uint64_t received_responses = 0;
};

class AtomicCore {
 public:
  AtomicCore(uint32_t hart_id, simbus::BusInterfaceV2& bus,
             simbus::BusPortT port, simbus::BusPortT memory_port);

  void load_program(std::vector<ScriptOp> program);
  void tick();

  uint32_t hart_id() const { return hart_id_; }
  CoreState state() const { return state_; }
  bool halted() const { return state_ == CoreState::Halted; }
  const CoreStats& stats() const { return stats_; }
  const std::vector<uint8_t>& last_read_data() const { return last_read_data_; }

 private:
  void receive_response();
  void begin_next_op();
  void advance_execution();
  void issue_memory_request();
  void retire_current();

  uint32_t hart_id_;
  simbus::BusInterfaceV2& bus_;
  simbus::BusPortT port_;
  simbus::BusPortT memory_port_;
  std::deque<ScriptOp> program_;
  std::optional<ScriptOp> current_;
  CoreState state_ = CoreState::Ready;
  uint32_t cycles_left_ = 0;
  uint32_t next_transaction_id_ = 1;
  uint32_t waiting_transaction_id_ = 0;
  std::vector<uint8_t> last_read_data_;
  CoreStats stats_;
};

}  // namespace manycore

