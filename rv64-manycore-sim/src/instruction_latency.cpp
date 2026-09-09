#include "manycore/instruction_latency.hpp"

#include <stdexcept>

namespace manycore {

uint32_t instruction_latency(const rv64archsem::DecodedInst& inst,
                             const InstructionLatencyConfig& config) {
  using rv64archsem::ControlType;
  using rv64archsem::OPCode;
  uint32_t value = config.integer_alu;
  if (inst.opcode == OPCode::opv) {
    value = inst.instName == rv64archsem::InstName::vsetvli ||
                        inst.instName == rv64archsem::InstName::vsetivli ||
                        inst.instName == rv64archsem::InstName::vsetvl
                ? config.vector_config
                : config.vector_integer_alu;
  } else {
    switch (inst.controlType) {
      case ControlType::branch:
      case ControlType::direct_jump:
      case ControlType::indirect_jump:
      case ControlType::call:
      case ControlType::ret:
        value = config.branch_jump;
        break;
      case ControlType::load:
      case ControlType::store:
        value = config.memory_issue;
        break;
      case ControlType::csr:
        value = config.csr;
        break;
      case ControlType::fence:
      case ControlType::fencei:
      case ControlType::fence_tso:
        value = config.fence;
        break;
      default:
        if ((inst.opcode == OPCode::op || inst.opcode == OPCode::op32) &&
            inst.funct7 == 0x01) {
          value = inst.funct3 >= 4 ? config.integer_divide : config.integer_multiply;
        }
        break;
    }
  }
  if (value == 0) throw std::runtime_error("instruction latency must be non-zero");
  return value;
}

}  // namespace manycore
