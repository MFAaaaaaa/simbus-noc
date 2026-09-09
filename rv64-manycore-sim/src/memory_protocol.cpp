#include "manycore/memory_protocol.hpp"

#include <stdexcept>

namespace manycore {
namespace {

constexpr uint8_t kProtocolVersion = 1;
constexpr uint8_t kRequestTag = 0x51;
constexpr uint8_t kResponseTag = 0x52;

template <typename T>
void append_le(std::vector<uint8_t>& out, T value) {
  for (size_t i = 0; i < sizeof(T); ++i) {
    out.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xffu));
  }
}

template <typename T>
T take_le(const std::vector<uint8_t>& bytes, size_t& pos) {
  if (pos + sizeof(T) > bytes.size()) {
    throw std::runtime_error("truncated memory protocol message");
  }
  T value = 0;
  for (size_t i = 0; i < sizeof(T); ++i) {
    value |= static_cast<T>(bytes[pos++]) << (i * 8);
  }
  return value;
}

void check_header(const std::vector<uint8_t>& bytes, size_t& pos, uint8_t tag) {
  if (take_le<uint8_t>(bytes, pos) != kProtocolVersion ||
      take_le<uint8_t>(bytes, pos) != tag) {
    throw std::runtime_error("invalid memory protocol header");
  }
}

}  // namespace

std::vector<uint8_t> encode_request(const MemoryRequest& request) {
  if (request.size == 0 || (request.op == MemoryOp::Write && request.data.size() != request.size)) {
    throw std::runtime_error("invalid memory request size");
  }
  std::vector<uint8_t> out;
  append_le<uint8_t>(out, kProtocolVersion);
  append_le<uint8_t>(out, kRequestTag);
  append_le<uint8_t>(out, static_cast<uint8_t>(request.op));
  append_le<uint32_t>(out, request.hart_id);
  append_le<uint32_t>(out, request.transaction_id);
  append_le<uint64_t>(out, request.address);
  append_le<uint16_t>(out, request.size);
  out.insert(out.end(), request.data.begin(), request.data.end());
  return out;
}

MemoryRequest decode_request(const std::vector<uint8_t>& bytes) {
  size_t pos = 0;
  check_header(bytes, pos, kRequestTag);
  MemoryRequest request;
  request.op = static_cast<MemoryOp>(take_le<uint8_t>(bytes, pos));
  request.hart_id = take_le<uint32_t>(bytes, pos);
  request.transaction_id = take_le<uint32_t>(bytes, pos);
  request.address = take_le<uint64_t>(bytes, pos);
  request.size = take_le<uint16_t>(bytes, pos);
  if (request.size == 0 || (request.op != MemoryOp::Read && request.op != MemoryOp::Write)) {
    throw std::runtime_error("invalid decoded memory request");
  }
  request.data.assign(bytes.begin() + static_cast<std::ptrdiff_t>(pos), bytes.end());
  if ((request.op == MemoryOp::Read && !request.data.empty()) ||
      (request.op == MemoryOp::Write && request.data.size() != request.size)) {
    throw std::runtime_error("invalid memory request payload");
  }
  return request;
}

std::vector<uint8_t> encode_response(const MemoryResponse& response) {
  std::vector<uint8_t> out;
  append_le<uint8_t>(out, kProtocolVersion);
  append_le<uint8_t>(out, kResponseTag);
  append_le<uint8_t>(out, static_cast<uint8_t>(response.status));
  append_le<uint32_t>(out, response.hart_id);
  append_le<uint32_t>(out, response.transaction_id);
  append_le<uint16_t>(out, static_cast<uint16_t>(response.data.size()));
  out.insert(out.end(), response.data.begin(), response.data.end());
  return out;
}

MemoryResponse decode_response(const std::vector<uint8_t>& bytes) {
  size_t pos = 0;
  check_header(bytes, pos, kResponseTag);
  MemoryResponse response;
  response.status = static_cast<MemoryStatus>(take_le<uint8_t>(bytes, pos));
  response.hart_id = take_le<uint32_t>(bytes, pos);
  response.transaction_id = take_le<uint32_t>(bytes, pos);
  const uint16_t size = take_le<uint16_t>(bytes, pos);
  response.data.assign(bytes.begin() + static_cast<std::ptrdiff_t>(pos), bytes.end());
  if (response.data.size() != size) {
    throw std::runtime_error("invalid memory response payload");
  }
  return response;
}

}  // namespace manycore

