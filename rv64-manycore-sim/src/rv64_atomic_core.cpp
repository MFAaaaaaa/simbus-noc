#include "manycore/rv64_atomic_core.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace manycore {
namespace {
constexpr simbus::ChannelT kRequestChannel = 0;
constexpr simbus::ChannelT kResponseChannel = 1;
constexpr uint64_t kCsrCycle = 0xc00;
constexpr uint64_t kCsrInstret = 0xc02;
constexpr uint64_t kCsrMhartid = 0xf14;
constexpr uint64_t kCsrVstart = 0x008;
constexpr uint64_t kCsrVxsat = 0x009;
constexpr uint64_t kCsrVxrm = 0x00a;
constexpr uint64_t kCsrVcsr = 0x00f;
constexpr uint64_t kCsrVl = 0xc20;
constexpr uint64_t kCsrVtype = 0xc21;
constexpr uint64_t kCsrVlenb = 0xc22;
constexpr uint64_t kVectorIllegal = uint64_t{1} << 63;
constexpr size_t kVectorRegisterBytes = rv64archsem::RVV_VLEN / 8;

bool is_vector_config_instruction(rv64archsem::InstName name) {
  using rv64archsem::InstName;
  return name == InstName::vsetvli || name == InstName::vsetivli ||
         name == InstName::vsetvl;
}

bool is_supported_vector_integer_instruction(rv64archsem::InstName name) {
  using rv64archsem::InstName;
  switch (name) {
    case InstName::vadd_vv: case InstName::vsub_vv:
    case InstName::vminu_vv: case InstName::vmin_vv:
    case InstName::vmaxu_vv: case InstName::vmax_vv:
    case InstName::vand_vv: case InstName::vor_vv: case InstName::vxor_vv:
    case InstName::vadd_vx: case InstName::vsub_vx: case InstName::vrsub_vx:
    case InstName::vminu_vx: case InstName::vmin_vx:
    case InstName::vmaxu_vx: case InstName::vmax_vx:
    case InstName::vand_vx: case InstName::vor_vx: case InstName::vxor_vx:
    case InstName::vadd_vi: case InstName::vrsub_vi:
    case InstName::vand_vi: case InstName::vor_vi: case InstName::vxor_vi:
    case InstName::vmv_v_v: case InstName::vmv_v_i:
    case InstName::vmv_x_s: case InstName::vmv_s_x:
      return true;
    default:
      return false;
  }
}

std::vector<rv64archsem::MemLoadDescriptor> normalize_unit_stride_loads(
    uint64_t base, std::vector<rv64archsem::MemLoadDescriptor> descriptors) {
  for (auto& descriptor : descriptors) {
    descriptor.addr = base + descriptor.reg_offset;
  }
  std::vector<rv64archsem::MemLoadDescriptor> merged;
  for (const auto& descriptor : descriptors) {
    if (!merged.empty() && merged.back().addr / 8 == descriptor.addr / 8 &&
        merged.back().addr + merged.back().size == descriptor.addr &&
        merged.back().reg_offset + merged.back().size == descriptor.reg_offset) {
      merged.back().size = static_cast<uint16_t>(merged.back().size + descriptor.size);
    } else {
      merged.push_back(descriptor);
    }
  }
  return merged;
}

std::vector<rv64archsem::MemStoreDescriptor> normalize_unit_stride_stores(
    uint64_t base, std::vector<rv64archsem::MemStoreDescriptor> descriptors) {
  uint64_t offset = 0;
  std::vector<rv64archsem::MemStoreDescriptor> merged;
  for (auto& descriptor : descriptors) {
    descriptor.addr = base + offset;
    offset += descriptor.raw_data.size();
    if (!merged.empty() && merged.back().addr / 8 == descriptor.addr / 8 &&
        merged.back().addr + merged.back().raw_data.size() == descriptor.addr) {
      merged.back().raw_data.insert(merged.back().raw_data.end(),
                                    descriptor.raw_data.begin(),
                                    descriptor.raw_data.end());
    } else {
      merged.push_back(std::move(descriptor));
    }
  }
  return merged;
}

uint32_t configured_vector_element_bytes(const rv64archsem::VecContext& context) {
  return uint32_t{1} << static_cast<uint32_t>(context.sew);
}

uint32_t vector_memory_element_bytes(const rv64archsem::DecodedInst& inst) {
  switch (inst.funct3) {
    case 0: return 1;
    case 5: return 2;
    case 6: return 4;
    case 7: return 8;
    default: throw std::runtime_error("unsupported vector memory element width");
  }
}

uint64_t sign_extend(uint64_t value, unsigned bits) {
  const uint64_t shift = 64 - bits;
  return static_cast<uint64_t>(static_cast<int64_t>(value << shift) >> shift);
}

std::string hex_value(uint64_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << value;
  return out.str();
}
}

Rv64AtomicCore::Rv64AtomicCore(uint32_t hart_id, simbus::BusInterfaceV2& bus,
                               simbus::BusPortT port, simbus::BusPortT memory_port,
                               const ProgramImage& program,
                               InstructionLatencyConfig latency,
                               uint64_t stack_top,
                               std::optional<uint64_t> tohost_address)
    : hart_id_(hart_id), bus_(bus), port_(port), memory_port_(memory_port),
      program_(program), latency_(latency), tohost_address_(tohost_address),
      pc_(program.base_address()) {
  x_[2] = stack_top - static_cast<uint64_t>(hart_id) * 0x10000ull;
}

void Rv64AtomicCore::write_reg(uint8_t index, uint64_t value) {
  if (index != 0) x_[index] = value;
}

uint64_t Rv64AtomicCore::read_csr(uint64_t address) const {
  if (address == kCsrVstart) return vector_context_.vstart;
  if (address == kCsrVxsat) return vector_context_.vcsr & 1u;
  if (address == kCsrVxrm) return (vector_context_.vcsr >> 1) & 3u;
  if (address == kCsrVcsr) return vector_context_.vcsr & 7u;
  if (address == kCsrVl) return vector_context_.vl;
  if (address == kCsrVtype) return vtype_;
  if (address == kCsrVlenb) return kVectorRegisterBytes;
  if (address == kCsrCycle) return stats_.ticks;
  if (address == kCsrInstret) return stats_.retired_instructions;
  if (address == kCsrMhartid) return hart_id_;
  const auto it = csr_.find(address);
  return it == csr_.end() ? 0 : it->second;
}

void Rv64AtomicCore::write_csr(uint64_t address, uint64_t value) {
  if (address == kCsrVstart) {
    vector_context_.vstart = static_cast<uint32_t>(value);
    return;
  }
  if (address == kCsrVxsat) {
    vector_context_.vcsr = (vector_context_.vcsr & ~uint64_t{1}) | (value & 1u);
    return;
  }
  if (address == kCsrVxrm) {
    vector_context_.vcsr = (vector_context_.vcsr & ~uint64_t{6}) | ((value & 3u) << 1);
    return;
  }
  if (address == kCsrVcsr) {
    vector_context_.vcsr = value & 7u;
    return;
  }
  if (address == kCsrVl || address == kCsrVtype || address == kCsrVlenb) return;
  if (address == kCsrCycle || address == kCsrInstret || address == kCsrMhartid) return;
  csr_[address] = value;
}

void Rv64AtomicCore::fetch_decode() {
  try {
    const uint16_t low = program_.fetch16(pc_);
    if ((low & 0x3u) != 0x3u) {
      fault("compressed instructions are not supported in this stage at " + hex_value(pc_));
      return;
    }
    const uint32_t raw = program_.fetch32(pc_);
    rv64archsem::DecodedInst decoded;
    if (!rv64archsem::decode(raw, decoded)) {
      fault("decode failed at " + hex_value(pc_) + " instruction " + hex_value(raw));
      return;
    }
    pending_ = PendingInstruction{decoded, raw, pc_, pc_ + 4};
    cycles_left_ = instruction_latency(decoded, latency_);
    state_ = Rv64CoreState::ExecuteWait;
  } catch (const std::exception& error) {
    fault(error.what());
  }
}

void Rv64AtomicCore::advance_execute() {
  if (cycles_left_ > 0) {
    --cycles_left_;
    ++stats_.execute_wait_cycles;
  }
  if (cycles_left_ == 0) execute_ready_instruction();
}

void Rv64AtomicCore::prepare_load() {
  std::vector<rv64archsem::MemLoadDescriptor> descriptors;
  const auto result = rv64archsem::parse_load_scalar(
      pending_->decoded, x_[pending_->decoded.rs1], 0, false, descriptors);
  if (result != rv64archsem::MemParseResult::success || descriptors.size() != 1) {
    throw std::runtime_error("scalar load descriptor generation failed");
  }
  MemoryRequest request;
  request.op = MemoryOp::Read;
  request.hart_id = hart_id_;
  request.transaction_id = next_transaction_id_++;
  request.address = descriptors.front().addr;
  request.size = descriptors.front().size;
  memory_request_ = std::move(request);
  load_rd_ = pending_->decoded.rd;
  load_extension_ = pending_->decoded.rdExt;
  state_ = Rv64CoreState::MemoryIssue;
  ++stats_.loads;
}

void Rv64AtomicCore::prepare_store() {
  std::vector<rv64archsem::MemStoreDescriptor> descriptors;
  const auto& inst = pending_->decoded;
  const auto result = rv64archsem::parse_store_scalar(
      inst, x_[inst.rs1], x_[inst.rs2], 0, false, descriptors);
  if (result != rv64archsem::MemParseResult::success || descriptors.size() != 1) {
    throw std::runtime_error("scalar store descriptor generation failed");
  }
  MemoryRequest request;
  request.op = MemoryOp::Write;
  request.hart_id = hart_id_;
  request.transaction_id = next_transaction_id_++;
  request.address = descriptors.front().addr;
  request.size = static_cast<uint16_t>(descriptors.front().raw_data.size());
  request.data = descriptors.front().raw_data;
  memory_request_ = std::move(request);
  state_ = Rv64CoreState::MemoryIssue;
  ++stats_.stores;
}

void Rv64AtomicCore::prepare_vector_load() {
  const auto& inst = pending_->decoded;
  if (inst.instName != rv64archsem::InstName::vleX_v) {
    throw std::runtime_error("only unit-stride vle is supported in vector memory stage 1");
  }
  if ((inst.funct7 & 1u) == 0 || ((inst.funct7 >> 4) & 7u) != 0) {
    throw std::runtime_error("masked or segmented vector loads are not supported yet");
  }
  if (vector_memory_element_bytes(inst) !=
      configured_vector_element_bytes(vector_context_)) {
    throw std::runtime_error("vector load EEW must equal the configured SEW");
  }
  std::memcpy(vector_context_.vmask.data(), vector_registers_[0].data(),
              kVectorRegisterBytes);
  std::vector<rv64archsem::MemLoadDescriptor> descriptors;
  const auto result = rv64archsem::parse_load_vector(
      inst, vector_context_, x_[inst.rs1], vector_operand(inst.rs2Type, inst.rs2),
      8, false, descriptors);
  if (result != rv64archsem::MemParseResult::success) {
    throw std::runtime_error("vector load descriptor generation failed with result " +
                             std::to_string(static_cast<int>(result)));
  }
  descriptors = normalize_unit_stride_loads(x_[inst.rs1], std::move(descriptors));
  vector_memory_active_ = true;
  vector_memory_is_load_ = true;
  vector_memory_register_ = inst.rd;
  vector_load_buffer_ = read_vector_group(inst.rd);
  vector_load_descriptors_.assign(descriptors.begin(), descriptors.end());
  ++stats_.vector_load_instructions;
  if (vector_load_descriptors_.empty()) {
    vector_memory_active_ = false;
    commit();
    return;
  }
  stage_next_vector_request();
}

void Rv64AtomicCore::prepare_vector_store() {
  const auto& inst = pending_->decoded;
  if (inst.instName != rv64archsem::InstName::vseX_v) {
    throw std::runtime_error("only unit-stride vse is supported in vector memory stage 1");
  }
  if ((inst.funct7 & 1u) == 0 || ((inst.funct7 >> 4) & 7u) != 0) {
    throw std::runtime_error("masked or segmented vector stores are not supported yet");
  }
  if (vector_memory_element_bytes(inst) !=
      configured_vector_element_bytes(vector_context_)) {
    throw std::runtime_error("vector store EEW must equal the configured SEW");
  }
  std::memcpy(vector_context_.vmask.data(), vector_registers_[0].data(),
              kVectorRegisterBytes);
  std::vector<rv64archsem::MemStoreDescriptor> descriptors;
  const auto result = rv64archsem::parse_store_vector(
      inst, vector_context_, x_[inst.rs1], vector_operand(inst.rs2Type, inst.rs2),
      read_vector_group(inst.rs3), 8, false, descriptors);
  if (result != rv64archsem::MemParseResult::success) {
    throw std::runtime_error("vector store descriptor generation failed with result " +
                             std::to_string(static_cast<int>(result)));
  }
  descriptors = normalize_unit_stride_stores(x_[inst.rs1], std::move(descriptors));
  vector_memory_active_ = true;
  vector_memory_is_load_ = false;
  vector_memory_register_ = inst.rs3;
  vector_store_descriptors_.assign(
      std::make_move_iterator(descriptors.begin()),
      std::make_move_iterator(descriptors.end()));
  ++stats_.vector_store_instructions;
  if (vector_store_descriptors_.empty()) {
    vector_memory_active_ = false;
    commit();
    return;
  }
  stage_next_vector_request();
}

void Rv64AtomicCore::stage_next_vector_request() {
  MemoryRequest request;
  request.hart_id = hart_id_;
  request.transaction_id = next_transaction_id_++;
  if (vector_memory_is_load_) {
    if (vector_load_descriptors_.empty()) {
      throw std::runtime_error("missing vector load descriptor");
    }
    active_vector_load_ = vector_load_descriptors_.front();
    vector_load_descriptors_.pop_front();
    request.op = MemoryOp::Read;
    request.address = active_vector_load_->addr;
    request.size = active_vector_load_->size;
  } else {
    if (vector_store_descriptors_.empty()) {
      throw std::runtime_error("missing vector store descriptor");
    }
    auto descriptor = std::move(vector_store_descriptors_.front());
    vector_store_descriptors_.pop_front();
    request.op = MemoryOp::Write;
    request.address = descriptor.addr;
    request.size = static_cast<uint16_t>(descriptor.raw_data.size());
    request.data = std::move(descriptor.raw_data);
  }
  memory_request_ = std::move(request);
  ++stats_.vector_memory_transactions;
  state_ = Rv64CoreState::MemoryIssue;
}

void Rv64AtomicCore::finish_vector_memory_response(
    const MemoryResponse& response) {
  if (vector_memory_is_load_) {
    if (!active_vector_load_ || response.data.size() != active_vector_load_->size) {
      throw std::runtime_error("invalid vector load response");
    }
    const size_t byte_capacity = vector_load_buffer_.size() * sizeof(uint64_t);
    const size_t offset = active_vector_load_->reg_offset;
    if (offset + response.data.size() > byte_capacity) {
      throw std::runtime_error("vector load response exceeds destination group");
    }
    std::memcpy(reinterpret_cast<uint8_t*>(vector_load_buffer_.data()) + offset,
                response.data.data(), response.data.size());
    active_vector_load_.reset();
  }

  memory_request_.reset();
  if ((vector_memory_is_load_ && !vector_load_descriptors_.empty()) ||
      (!vector_memory_is_load_ && !vector_store_descriptors_.empty())) {
    stage_next_vector_request();
    return;
  }
  if (vector_memory_is_load_) {
    write_vector_group(vector_memory_register_, vector_load_buffer_);
  }
  vector_load_buffer_.clear();
  vector_memory_active_ = false;
  commit();
}

void Rv64AtomicCore::execute_csr() {
  const auto& inst = pending_->decoded;
  const uint64_t address = static_cast<uint64_t>(inst.imm) & 0xfffu;
  const uint64_t old_value = read_csr(address);
  uint64_t new_value = old_value;
  const bool immediate = inst.funct3 >= 5;
  const uint64_t source = immediate ? inst.rs1 : x_[inst.rs1];
  bool write = true;
  switch (inst.funct3) {
    case 1: case 5: new_value = source; break;
    case 2: case 6: new_value = old_value | source; write = source != 0; break;
    case 3: case 7: new_value = old_value & ~source; write = source != 0; break;
    default: throw std::runtime_error("unsupported CSR operation");
  }
  write_reg(inst.rd, old_value);
  if (write) write_csr(address, new_value);
}

void Rv64AtomicCore::execute_vector_config() {
  using rv64archsem::InstName;
  using rv64archsem::RVVLMUL;
  using rv64archsem::RVVSEW;
  const auto& inst = pending_->decoded;

  uint64_t requested_vtype = 0;
  uint64_t avl = 0;
  if (inst.instName == InstName::vsetvl) {
    requested_vtype = x_[inst.rs2];
  } else {
    requested_vtype = static_cast<uint64_t>(inst.imm);
  }
  if (inst.instName == InstName::vsetivli) {
    avl = inst.rs1;
  } else if (inst.rs1 != 0) {
    avl = x_[inst.rs1];
  } else if (inst.rd != 0) {
    avl = std::numeric_limits<uint64_t>::max();
  } else {
    avl = vector_context_.vl;
  }

  const uint64_t vlmul = requested_vtype & 7u;
  const uint64_t vsew = (requested_vtype >> 3) & 7u;
  const bool unsupported = (requested_vtype & ~uint64_t{0xff}) != 0 ||
                           vlmul > 3 || vsew > 3;
  vector_context_.vstart = 0;
  if (unsupported) {
    vtype_ = kVectorIllegal;
    vector_context_.vl = 0;
  } else {
    vtype_ = requested_vtype & 0xffu;
    vector_context_.sew = static_cast<RVVSEW>(vsew);
    vector_context_.lmul = static_cast<RVVLMUL>(vlmul);
    const uint64_t vlmax = rv64archsem::get_max_vector_length(
        vector_context_.sew, vector_context_.lmul);
    vector_context_.vl = static_cast<uint32_t>(std::min(avl, vlmax));
  }
  write_reg(inst.rd, vector_context_.vl);
  ++stats_.vector_config_instructions;
  commit();
}

uint32_t Rv64AtomicCore::vector_group_registers() const {
  switch (vector_context_.lmul) {
    case rv64archsem::RVVLMUL::m1: return 1;
    case rv64archsem::RVVLMUL::m2: return 2;
    case rv64archsem::RVVLMUL::m4: return 4;
    case rv64archsem::RVVLMUL::m8: return 8;
    default: throw std::runtime_error("fractional LMUL is not supported yet");
  }
}

std::vector<uint64_t> Rv64AtomicCore::read_vector_group(uint8_t base) const {
  const uint32_t register_count = vector_group_registers();
  if (base % register_count != 0 || base + register_count > vector_registers_.size()) {
    throw std::runtime_error("misaligned or overflowing vector register group");
  }
  std::vector<uint64_t> words;
  words.reserve(register_count * vector_registers_.front().size());
  for (uint32_t reg = 0; reg < register_count; ++reg) {
    words.insert(words.end(), vector_registers_[base + reg].begin(),
                 vector_registers_[base + reg].end());
  }
  return words;
}

void Rv64AtomicCore::write_vector_group(
    uint8_t base, const std::vector<uint64_t>& words) {
  const uint32_t register_count = vector_group_registers();
  const size_t words_per_register = vector_registers_.front().size();
  if (base % register_count != 0 || base + register_count > vector_registers_.size() ||
      words.size() < register_count * words_per_register) {
    throw std::runtime_error("invalid vector destination register group");
  }
  for (uint32_t reg = 0; reg < register_count; ++reg) {
    std::copy_n(words.begin() + reg * words_per_register, words_per_register,
                vector_registers_[base + reg].begin());
  }
}

std::vector<uint64_t> Rv64AtomicCore::vector_operand(
    rv64archsem::RegType type, uint8_t index) const {
  using rv64archsem::RegType;
  switch (type) {
    case RegType::none:
      return {0};
    case RegType::gpreg:
      return {x_[index]};
    case RegType::vreg:
      return read_vector_group(index);
    case RegType::freg:
      throw std::runtime_error("floating-point vector operands are not supported yet");
  }
  throw std::runtime_error("unknown vector operand type");
}

void Rv64AtomicCore::execute_vector_alu() {
  using rv64archsem::RegType;
  const auto& inst = pending_->decoded;
  if ((vtype_ & kVectorIllegal) != 0) {
    throw std::runtime_error("vector instruction executed while vtype.vill is set");
  }
  if (!is_supported_vector_integer_instruction(inst.instName)) {
    throw std::runtime_error("vector instruction is outside the stage-1 integer subset");
  }
  std::memcpy(vector_context_.vmask.data(), vector_registers_[0].data(),
              kVectorRegisterBytes);
  auto source1 = vector_operand(inst.rs1Type, inst.rs1);
  auto source2 = vector_operand(inst.rs2Type, inst.rs2);
  std::vector<uint64_t> destination;
  if (inst.rdType == RegType::gpreg) {
    destination = {x_[inst.rd]};
  } else if (inst.rdType == RegType::vreg) {
    destination = read_vector_group(inst.rd);
  } else {
    throw std::runtime_error("unsupported vector destination register type");
  }

  const auto result = rv64archsem::exec_opv(
      inst, vector_context_, destination, source1, source2, fcsr_);
  if (result != rv64archsem::VecEXResult::success) {
    throw std::runtime_error("vector execution failed with result " +
                             std::to_string(static_cast<int>(result)));
  }
  if (inst.rdType == RegType::gpreg) {
    write_reg(inst.rd, destination.at(0));
  } else {
    write_vector_group(inst.rd, destination);
  }
  ++stats_.vector_alu_instructions;
  commit();
}

bool Rv64AtomicCore::handle_tohost_write(const MemoryRequest& request) {
  if (!tohost_address_ || request.op != MemoryOp::Write ||
      request.address != *tohost_address_) {
    return false;
  }
  if (request.data.empty() || request.data.size() > sizeof(uint64_t)) {
    throw std::runtime_error("tohost write must contain 1 to 8 bytes");
  }
  uint64_t value = 0;
  for (size_t i = 0; i < request.data.size(); ++i) {
    value |= uint64_t{request.data[i]} << (i * 8);
  }
  if (value == 0 || (value & 1u) == 0) return false;

  const uint64_t test_number = value >> 1;
  commit();
  if (test_number == 0) {
    stop(0, "tohost pass");
  } else {
    const int code = static_cast<int>(std::min<uint64_t>(test_number, 124));
    stop(code, "tohost failure " + std::to_string(test_number));
  }
  return true;
}

void Rv64AtomicCore::execute_ready_instruction() {
  using rv64archsem::ControlType;
  using rv64archsem::OPCode;
  try {
    auto& inst = pending_->decoded;
    switch (inst.controlType) {
      case ControlType::none:
        if (inst.opcode == OPCode::opv) {
          if (is_vector_config_instruction(inst.instName)) execute_vector_config();
          else execute_vector_alu();
          break;
        }
        switch (inst.opcode) {
          case OPCode::lui: write_reg(inst.rd, static_cast<uint64_t>(inst.imm)); break;
          case OPCode::auipc: write_reg(inst.rd, pending_->pc + static_cast<uint64_t>(inst.imm)); break;
          case OPCode::opimm: write_reg(inst.rd, rv64archsem::exec_opimm(inst, x_[inst.rs1])); break;
          case OPCode::opimm32: write_reg(inst.rd, rv64archsem::exec_opimm32(inst, x_[inst.rs1])); break;
          case OPCode::op: write_reg(inst.rd, rv64archsem::exec_op(inst, x_[inst.rs1], x_[inst.rs2])); break;
          case OPCode::op32: write_reg(inst.rd, rv64archsem::exec_op32(inst, x_[inst.rs1], x_[inst.rs2])); break;
          default: throw std::runtime_error("non-integer opcode is not supported in this stage");
        }
        commit();
        break;
      case ControlType::branch: {
        uint64_t target = pending_->pc;
        if (rv64archsem::exec_branch(inst, x_[inst.rs1], x_[inst.rs2], target)) pending_->next_pc = target;
        commit();
        break;
      }
      case ControlType::direct_jump:
      case ControlType::call:
        write_reg(inst.rd, pending_->next_pc);
        pending_->next_pc = pending_->pc + static_cast<uint64_t>(inst.imm);
        commit();
        break;
      case ControlType::indirect_jump:
      case ControlType::ret:
        write_reg(inst.rd, pending_->next_pc);
        pending_->next_pc = (x_[inst.rs1] + static_cast<uint64_t>(inst.imm)) & ~uint64_t{1};
        commit();
        break;
      case ControlType::load: prepare_load(); break;
      case ControlType::store: prepare_store(); break;
      case ControlType::vload: prepare_vector_load(); break;
      case ControlType::vstore: prepare_vector_store(); break;
      case ControlType::csr: execute_csr(); commit(); break;
      case ControlType::fence:
      case ControlType::fencei:
      case ControlType::fence_tso: commit(); break;
      case ControlType::ecall: stop(static_cast<int>(x_[10]), "ecall"); break;
      case ControlType::ebreak: stop(0, "ebreak"); break;
      case ControlType::amo: throw std::runtime_error("AMO is not supported in this stage");
      default: throw std::runtime_error("privileged or unsupported control instruction");
    }
  } catch (const std::exception& error) {
    fault(error.what());
  }
}

void Rv64AtomicCore::issue_memory_request() {
  if (!bus_.can_send(port_, kRequestChannel)) {
    ++stats_.injection_blocked_cycles;
    return;
  }
  if (!bus_.send(port_, memory_port_, kRequestChannel, encode_request(*memory_request_))) {
    ++stats_.injection_blocked_cycles;
    return;
  }
  state_ = Rv64CoreState::MemoryWait;
}

void Rv64AtomicCore::receive_memory_response() {
  if (!bus_.can_recv(port_, kResponseChannel)) {
    ++stats_.memory_wait_cycles;
    return;
  }
  std::vector<uint8_t> bytes;
  if (!bus_.recv(port_, kResponseChannel, bytes)) {
    ++stats_.memory_wait_cycles;
    return;
  }
  try {
    const auto response = decode_response(bytes);
    if (!memory_request_ || response.hart_id != hart_id_ ||
        response.transaction_id != memory_request_->transaction_id) {
      throw std::runtime_error("unexpected memory response");
    }
    if (response.status != MemoryStatus::Ok) throw std::runtime_error("memory request failed");
    if (vector_memory_active_) {
      finish_vector_memory_response(response);
      return;
    }
    if (memory_request_->op == MemoryOp::Read) {
      if (response.data.size() != memory_request_->size || response.data.size() > 8) {
        throw std::runtime_error("invalid scalar load response size");
      }
      uint64_t value = 0;
      for (size_t i = 0; i < response.data.size(); ++i) value |= uint64_t{response.data[i]} << (i * 8);
      switch (load_extension_) {
        case rv64archsem::RDExtType::sign8: value = sign_extend(value, 8); break;
        case rv64archsem::RDExtType::sign16: value = sign_extend(value, 16); break;
        case rv64archsem::RDExtType::sign32: value = sign_extend(value, 32); break;
        case rv64archsem::RDExtType::zero: break;
      }
      write_reg(load_rd_, value);
    }
    const MemoryRequest completed_request = *memory_request_;
    memory_request_.reset();
    if (!handle_tohost_write(completed_request)) commit();
  } catch (const std::exception& error) {
    fault(error.what());
  }
}

void Rv64AtomicCore::commit() {
  pc_ = pending_->next_pc;
  pending_.reset();
  x_[0] = 0;
  ++stats_.retired_instructions;
  state_ = Rv64CoreState::FetchDecode;
}

void Rv64AtomicCore::stop(int code, std::string reason) {
  exit_code_ = code;
  stop_reason_ = std::move(reason);
  pending_.reset();
  state_ = Rv64CoreState::Halted;
}

void Rv64AtomicCore::fault(std::string reason) {
  exit_code_ = 125;
  stop_reason_ = std::move(reason);
  pending_.reset();
  memory_request_.reset();
  vector_load_descriptors_.clear();
  vector_store_descriptors_.clear();
  active_vector_load_.reset();
  vector_load_buffer_.clear();
  vector_memory_active_ = false;
  state_ = Rv64CoreState::Faulted;
}

void Rv64AtomicCore::tick() {
  if (finished()) return;
  ++stats_.ticks;
  switch (state_) {
    case Rv64CoreState::FetchDecode: fetch_decode(); break;
    case Rv64CoreState::ExecuteWait: advance_execute(); break;
    case Rv64CoreState::MemoryIssue: issue_memory_request(); break;
    case Rv64CoreState::MemoryWait: receive_memory_response(); break;
    case Rv64CoreState::Halted:
    case Rv64CoreState::Faulted: break;
  }
}

}  // namespace manycore
