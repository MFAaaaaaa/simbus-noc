#pragma once

#include "manycore/instruction_latency.hpp"
#include "manycore/memory_protocol.hpp"
#include "manycore/program_image.hpp"
#include "simbus/symmulcha.h"

#include <riscv64.hpp>

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace manycore {

enum class Rv64CoreState {
  FetchDecode,
  ExecuteWait,
  MemoryIssue,
  MemoryWait,
  Halted,
  Faulted,
};

struct Rv64CoreStats {
  uint64_t ticks = 0;
  uint64_t retired_instructions = 0;
  uint64_t execute_wait_cycles = 0;
  uint64_t injection_blocked_cycles = 0;
  uint64_t memory_wait_cycles = 0;
  uint64_t loads = 0;
  uint64_t stores = 0;
  uint64_t vector_config_instructions = 0;
  uint64_t vector_alu_instructions = 0;
  uint64_t vector_load_instructions = 0;
  uint64_t vector_store_instructions = 0;
  uint64_t vector_memory_transactions = 0;
};

class Rv64AtomicCore {
 public:
  Rv64AtomicCore(uint32_t hart_id, simbus::BusInterfaceV2& bus,
                 simbus::BusPortT port, simbus::BusPortT memory_port,
                 const ProgramImage& program,
                 InstructionLatencyConfig latency = {},
                 uint64_t stack_top = 0x88000000ull,
                 std::optional<uint64_t> tohost_address = std::nullopt);

  void tick();
  bool halted() const { return state_ == Rv64CoreState::Halted; }
  bool faulted() const { return state_ == Rv64CoreState::Faulted; }
  bool finished() const { return halted() || faulted(); }
  Rv64CoreState state() const { return state_; }
  uint64_t pc() const { return pc_; }
  uint64_t reg(size_t index) const { return x_.at(index); }
  uint64_t vector_reg_word(size_t reg, size_t word) const {
    return vector_registers_.at(reg).at(word);
  }
  uint32_t vl() const { return vector_context_.vl; }
  uint64_t vtype() const { return vtype_; }
  uint32_t vstart() const { return vector_context_.vstart; }
  uint64_t vcsr() const { return vector_context_.vcsr; }
  int exit_code() const { return exit_code_; }
  const std::string& stop_reason() const { return stop_reason_; }
  const Rv64CoreStats& stats() const { return stats_; }

 private:
  struct PendingInstruction {
    rv64archsem::DecodedInst decoded;
    uint32_t raw = 0;
    uint64_t pc = 0;
    uint64_t next_pc = 0;
  };

  void fetch_decode();
  void advance_execute();
  void execute_ready_instruction();
  void prepare_load();
  void prepare_store();
  void prepare_vector_load();
  void prepare_vector_store();
  void stage_next_vector_request();
  void finish_vector_memory_response(const MemoryResponse& response);
  void issue_memory_request();
  void receive_memory_response();
  void execute_csr();
  void execute_vector_config();
  void execute_vector_alu();
  std::vector<uint64_t> vector_operand(rv64archsem::RegType type,
                                       uint8_t index) const;
  std::vector<uint64_t> read_vector_group(uint8_t base) const;
  void write_vector_group(uint8_t base, const std::vector<uint64_t>& words);
  uint32_t vector_group_registers() const;
  bool handle_tohost_write(const MemoryRequest& request);
  uint64_t read_csr(uint64_t address) const;
  void write_csr(uint64_t address, uint64_t value);
  void write_reg(uint8_t index, uint64_t value);
  void commit();
  void stop(int code, std::string reason);
  void fault(std::string reason);

  uint32_t hart_id_;
  simbus::BusInterfaceV2& bus_;
  simbus::BusPortT port_;
  simbus::BusPortT memory_port_;
  const ProgramImage& program_;
  InstructionLatencyConfig latency_;
  std::optional<uint64_t> tohost_address_;
  Rv64CoreState state_ = Rv64CoreState::FetchDecode;
  uint64_t pc_ = 0;
  std::array<uint64_t, 32> x_{};
  std::array<std::array<uint64_t, rv64archsem::RVV_VLEN / 64>, 32>
      vector_registers_{};
  rv64archsem::VecContext vector_context_{
      rv64archsem::RVVSEW::e8, rv64archsem::RVVLMUL::m1, 0, 0, 0,
      std::vector<uint8_t>(rv64archsem::RVV_VLEN / 8)};
  uint64_t vtype_ = uint64_t{1} << 63;
  uint64_t fcsr_ = 0;
  std::unordered_map<uint64_t, uint64_t> csr_;
  std::optional<PendingInstruction> pending_;
  std::optional<MemoryRequest> memory_request_;
  std::deque<rv64archsem::MemLoadDescriptor> vector_load_descriptors_;
  std::deque<rv64archsem::MemStoreDescriptor> vector_store_descriptors_;
  std::optional<rv64archsem::MemLoadDescriptor> active_vector_load_;
  std::vector<uint64_t> vector_load_buffer_;
  uint8_t vector_memory_register_ = 0;
  bool vector_memory_active_ = false;
  bool vector_memory_is_load_ = false;
  uint8_t load_rd_ = 0;
  rv64archsem::RDExtType load_extension_ = rv64archsem::RDExtType::zero;
  uint32_t cycles_left_ = 0;
  uint32_t next_transaction_id_ = 1;
  int exit_code_ = 0;
  std::string stop_reason_;
  Rv64CoreStats stats_;
};

}  // namespace manycore
