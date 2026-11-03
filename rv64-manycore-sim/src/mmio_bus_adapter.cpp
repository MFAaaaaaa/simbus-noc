#include "manycore/mmio_bus_adapter.hpp"
#include <limits>
#include <stdexcept>

namespace manycore {
MmioBusAdapter::MmioBusAdapter(uint32_t hart, simbus::BusInterfaceV2& bus,
                             simbus::BusPortT port, simbus::BusPortT memory_port)
    : hart_(hart), bus_(bus), port_(port), memory_port_(memory_port) {}

uint64_t MmioBusAdapter::offset(uint64_t address, uint16_t size) const {
  if (!contains(address) || size != 8 || address % 8 != 0)
    throw std::runtime_error("MMIO requires aligned 64-bit register accesses");
  return address - base;
}
void MmioBusAdapter::reject() { status_ |= error_bit; ++stats_.errors; }

uint64_t MmioBusAdapter::read(uint64_t address, uint16_t size) {
  switch (offset(address, size)) {
    case 0x00: return target_;
    case 0x08: return write_data_;
    case 0x10: return size_;
    case 0x18: return 0;
    case 0x20: return (status_ & ~busy_bit) | (busy()?busy_bit:0);
    case 0x28: return results_.empty()?read_data_:results_.front().data;
    case 0x30: return depth_;
    case 0x38: return requests_.size();
    case 0x40: return results_.empty()?0:results_.front().id;
    case 0x50: return results_.empty()?0:(results_.front().error?error_bit:done_bit);
    default: throw std::runtime_error("unknown MMIO register");
  }
}
void MmioBusAdapter::write(uint64_t address, uint16_t size, uint64_t value) {
  const auto reg = offset(address, size);
  if(reg==0x48){if(value!=1||results_.empty()){reject();return;}results_.pop_front();if(results_.empty())status_&=~done_bit;return;}
  if (reg > 0x28) throw std::runtime_error("unknown/read-only MMIO register");
  if (busy() && depth_==1) { reject(); return; }
  switch (reg) {
    case 0x00: target_ = value; return;
    case 0x08: write_data_ = value; return;
    case 0x10: size_ = value; return;
    case 0x20: status_ &= ~(value & (done_bit | error_bit)); return;
    case 0x28: reject(); return;
    case 0x18: break;
    default: throw std::runtime_error("unknown MMIO register");
  }
  if(requests_.size()+(depth_>1?results_.size():0)>=depth_){reject();return;}
  status_ = depth_==1?0:(status_ & (done_bit|error_bit));
  read_data_ = 0;
  if ((value != 1 && value != 2) ||
      (size_ != 1 && size_ != 2 && size_ != 4 && size_ != 8) ||
      target_ > std::numeric_limits<uint64_t>::max() - (size_ - 1) ||
      (target_ < base + span && target_ + size_ > base)) {
    reject(); return;
  }
  MemoryRequest request;
  request.hart_id = hart_;
  // Reserved ID 0. The hart serializes normal requests behind this adapter.
  if(depth_>1 && next_id_==0x7fffffff) {reject();return;}
  request.transaction_id = depth_==1?0:0x80000000u|next_id_++;
  request.address = target_;
  request.size = static_cast<uint16_t>(size_);
  request.op = value == 1 ? MemoryOp::Read : MemoryOp::Write;
  if (request.op == MemoryOp::Write) {
    for (uint64_t i = 0; i < size_; ++i)
      request.data.push_back(static_cast<uint8_t>(write_data_ >> (8 * i)));
  }
  requests_.push_back({std::move(request),false,clock_});
  status_ |= busy_bit;
  ++stats_.commands;
}
void MmioBusAdapter::tick() {
  ++clock_;
  auto unsent=std::find_if(requests_.begin(),requests_.end(),[](auto& e){return !e.sent;});
  if (unsent!=requests_.end()) {
    if (!bus_.can_send(port_, 0) ||
        !bus_.send(port_, memory_port_, 0, encode_request(unsent->request))) ++stats_.injection_wait;
    else unsent->sent=true;
  }
  if(!busy())return;
  std::vector<uint8_t> bytes;
  if (!bus_.can_recv(port_, 1) || !bus_.recv(port_, 1, bytes)) {
    ++stats_.response_wait; return;
  }
  const auto response = decode_response(bytes);
  auto found=std::find_if(requests_.begin(),requests_.end(),[&](auto& e){return e.sent&&e.request.transaction_id==response.transaction_id;});
  if (response.hart_id != hart_ || found==requests_.end())
    throw std::runtime_error("unexpected MMIO bus response");
  const size_t expected = found->request.op == MemoryOp::Read ? found->request.size : 0;
  read_data_=0;
  const bool failed=response.status != MemoryStatus::Ok || response.data.size() != expected;
  if (failed) reject();
  else {
    for (size_t i = 0; i < response.data.size(); ++i)
      read_data_ |= uint64_t{response.data[i]} << (8 * i);
    ++stats_.completed;
  }
  stats_.latency.add(clock_-found->created);
  if(depth_>1)results_.push_back({response.transaction_id,read_data_,failed});
  requests_.erase(found);
  status_ = (status_ & error_bit) | done_bit | (busy()?busy_bit:0);
}
}  // namespace manycore
