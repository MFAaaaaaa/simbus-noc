#pragma once

#include <riscv64.hpp>

#include <cstdint>

namespace manycore {

struct InstructionLatencyConfig {
  uint32_t integer_alu = 1;
  uint32_t integer_multiply = 3;
  uint32_t integer_divide = 18;
  uint32_t branch_jump = 1;
  uint32_t csr = 1;
  uint32_t memory_issue = 1;
  uint32_t fence = 1;
  uint32_t vector_config = 1;
  uint32_t vector_integer_alu = 4;
};

uint32_t instruction_latency(const rv64archsem::DecodedInst& inst,
                             const InstructionLatencyConfig& config);

}  // namespace manycore
