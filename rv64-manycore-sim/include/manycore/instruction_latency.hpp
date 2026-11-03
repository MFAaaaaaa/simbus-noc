#pragma once

#include <riscv64.hpp>

#include <cstdint>
#include <map>
#include <string>

namespace manycore {

struct InstructionLatencyConfig {
  struct Timing { uint32_t base = 1, lanes = 0; };
  std::map<std::string, Timing> instructions;
  uint32_t vector_base = 0, vector_lanes = 0;
  uint32_t integer_alu = 1;
  uint32_t integer_multiply = 3;
  uint32_t integer_divide = 18;
  uint32_t branch_jump = 1;
  uint32_t csr = 1;
  uint32_t memory_issue = 1;
  uint32_t fence = 1;
  uint32_t vector_config = 1;
  uint32_t vector_integer_alu = 4;
  uint32_t vector_transaction_issue = 1;
};

uint32_t instruction_latency(const rv64archsem::DecodedInst& inst,
                             const InstructionLatencyConfig& config, uint32_t vl = 0);
void load_instruction_timing(const std::string& path, InstructionLatencyConfig& config);
std::string instruction_mnemonic(rv64archsem::InstName name);

}  // namespace manycore
