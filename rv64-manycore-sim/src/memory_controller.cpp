#include "manycore/memory_controller.hpp"

#include <algorithm>
#include <stdexcept>

namespace manycore {
namespace {
constexpr simbus::ChannelT kRequestChannel = 0;
constexpr simbus::ChannelT kResponseChannel = 1;
}

MemoryController::MemoryController(simbus::BusInterfaceV2& bus, simbus::BusPortT port,
                                   MemoryControllerConfig config)
    : bus_(bus), port_(port), config_(config) {
  if (config_.service_latency == 0 || config_.request_queue_capacity == 0) {
    throw std::runtime_error("memory controller latency and queue capacity must be non-zero");
  }
}

void MemoryController::receive_request() {
  if (queue_.size() >= config_.request_queue_capacity) {
    if (bus_.can_recv(port_, kRequestChannel)) ++stats_.queue_full_cycles;
    return;
  }
  if (!bus_.can_recv(port_, kRequestChannel)) return;
  std::vector<uint8_t> bytes;
  if (!bus_.recv(port_, kRequestChannel, bytes)) return;
  queue_.push_back(decode_request(bytes));
  ++stats_.received_requests;
  stats_.max_queue_depth = std::max(stats_.max_queue_depth, queue_.size());
}

void MemoryController::start_request() {
  if (active_ || response_ || queue_.empty()) return;
  active_ = WorkItem{std::move(queue_.front()), config_.service_latency};
  queue_.pop_front();
}

void MemoryController::advance_request() {
  if (!active_ || response_) return;
  if (active_->cycles_left > 1) {
    --active_->cycles_left;
    return;
  }
  const MemoryRequest& request = active_->request;
  MemoryResponse response;
  response.hart_id = request.hart_id;
  response.transaction_id = request.transaction_id;
  if (request.op == MemoryOp::Read) {
    response.data = memory_.read(request.address, request.size);
  } else if (request.op == MemoryOp::Write) {
    memory_.write(request.address, request.data);
  } else {
    response.status = MemoryStatus::InvalidRequest;
  }
  response_ = std::move(response);
  active_.reset();
  ++stats_.completed_requests;
}

void MemoryController::send_response() {
  if (!response_) return;
  if (!bus_.can_send(port_, kResponseChannel)) {
    ++stats_.response_injection_blocked_cycles;
    return;
  }
  const simbus::BusPortT destination = response_->hart_id;
  if (!bus_.send(port_, destination, kResponseChannel, encode_response(*response_))) {
    ++stats_.response_injection_blocked_cycles;
    return;
  }
  response_.reset();
}

void MemoryController::tick() {
  send_response();
  receive_request();
  advance_request();
  start_request();
}

bool MemoryController::idle() const {
  return queue_.empty() && !active_ && !response_;
}

}  // namespace manycore

