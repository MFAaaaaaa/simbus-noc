#include "manycore/atomic_core.hpp"

#include <stdexcept>

namespace manycore {
namespace {
constexpr simbus::ChannelT kRequestChannel = 0;
constexpr simbus::ChannelT kResponseChannel = 1;
}

ScriptOp ScriptOp::compute(uint32_t latency) {
  if (latency == 0) throw std::runtime_error("compute latency must be non-zero");
  ScriptOp op;
  op.kind = ScriptOpKind::Compute;
  op.latency = latency;
  return op;
}

ScriptOp ScriptOp::read(uint64_t address, uint16_t size, uint32_t issue_latency) {
  if (size == 0 || issue_latency == 0) throw std::runtime_error("invalid read operation");
  ScriptOp op;
  op.kind = ScriptOpKind::Read;
  op.latency = issue_latency;
  op.address = address;
  op.size = size;
  return op;
}

ScriptOp ScriptOp::write(uint64_t address, std::vector<uint8_t> data,
                         uint32_t issue_latency) {
  if (data.empty() || data.size() > UINT16_MAX || issue_latency == 0) {
    throw std::runtime_error("invalid write operation");
  }
  ScriptOp op;
  op.kind = ScriptOpKind::Write;
  op.latency = issue_latency;
  op.address = address;
  op.size = static_cast<uint16_t>(data.size());
  op.data = std::move(data);
  return op;
}

ScriptOp ScriptOp::halt() {
  ScriptOp op;
  op.kind = ScriptOpKind::Halt;
  return op;
}

AtomicCore::AtomicCore(uint32_t hart_id, simbus::BusInterfaceV2& bus,
                       simbus::BusPortT port, simbus::BusPortT memory_port)
    : hart_id_(hart_id), bus_(bus), port_(port), memory_port_(memory_port) {}

void AtomicCore::load_program(std::vector<ScriptOp> program) {
  if (state_ != CoreState::Ready || current_) {
    throw std::runtime_error("cannot replace a running core program");
  }
  program_ = std::deque<ScriptOp>(program.begin(), program.end());
}

void AtomicCore::receive_response() {
  if (!bus_.can_recv(port_, kResponseChannel)) return;
  std::vector<uint8_t> bytes;
  if (!bus_.recv(port_, kResponseChannel, bytes)) return;
  const auto response = decode_response(bytes);
  if (response.hart_id != hart_id_ || response.transaction_id != waiting_transaction_id_) {
    throw std::runtime_error("core received an unexpected memory response");
  }
  if (response.status != MemoryStatus::Ok) {
    throw std::runtime_error("memory controller rejected a request");
  }
  if (current_ && current_->kind == ScriptOpKind::Read) last_read_data_ = response.data;
  ++stats_.received_responses;
  retire_current();
}

void AtomicCore::begin_next_op() {
  if (program_.empty()) {
    state_ = CoreState::Halted;
    return;
  }
  current_ = std::move(program_.front());
  program_.pop_front();
  if (current_->kind == ScriptOpKind::Halt) {
    current_.reset();
    state_ = CoreState::Halted;
    return;
  }
  cycles_left_ = current_->latency;
  state_ = CoreState::Executing;
}

void AtomicCore::advance_execution() {
  if (cycles_left_ > 0) {
    --cycles_left_;
    ++stats_.compute_cycles;
  }
  if (cycles_left_ != 0) return;
  if (current_->kind == ScriptOpKind::Compute) {
    retire_current();
  } else {
    state_ = CoreState::MemoryIssue;
  }
}

void AtomicCore::issue_memory_request() {
  if (!bus_.can_send(port_, kRequestChannel)) {
    ++stats_.injection_blocked_cycles;
    return;
  }
  MemoryRequest request;
  request.op = current_->kind == ScriptOpKind::Read ? MemoryOp::Read : MemoryOp::Write;
  request.hart_id = hart_id_;
  request.transaction_id = next_transaction_id_++;
  request.address = current_->address;
  request.size = current_->size;
  request.data = current_->data;
  if (!bus_.send(port_, memory_port_, kRequestChannel, encode_request(request))) {
    ++stats_.injection_blocked_cycles;
    return;
  }
  waiting_transaction_id_ = request.transaction_id;
  ++stats_.sent_requests;
  state_ = CoreState::MemoryWait;
}

void AtomicCore::retire_current() {
  current_.reset();
  waiting_transaction_id_ = 0;
  ++stats_.retired_ops;
  state_ = CoreState::Ready;
}

void AtomicCore::tick() {
  if (state_ == CoreState::Halted) return;
  ++stats_.ticks;
  if (state_ == CoreState::MemoryWait) {
    receive_response();
    if (state_ == CoreState::MemoryWait) ++stats_.memory_wait_cycles;
    return;
  }
  if (state_ == CoreState::Ready) begin_next_op();
  if (state_ == CoreState::Executing) advance_execution();
  if (state_ == CoreState::MemoryIssue) issue_memory_request();
}

}  // namespace manycore

