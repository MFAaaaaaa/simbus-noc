#pragma once

#include <cstdint>
#include <vector>

namespace manycore {

enum class MemoryOp : uint8_t {
  Read = 1,
  Write = 2,
};

enum class MemoryStatus : uint8_t {
  Ok = 0,
  InvalidRequest = 1,
};

struct MemoryRequest {
  MemoryOp op = MemoryOp::Read;
  uint32_t hart_id = 0;
  uint32_t transaction_id = 0;
  uint64_t address = 0;
  std::vector<uint8_t> data;
  uint16_t size = 0;
};

struct MemoryResponse {
  MemoryStatus status = MemoryStatus::Ok;
  uint32_t hart_id = 0;
  uint32_t transaction_id = 0;
  std::vector<uint8_t> data;
};

std::vector<uint8_t> encode_request(const MemoryRequest& request);
MemoryRequest decode_request(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> encode_response(const MemoryResponse& response);
MemoryResponse decode_response(const std::vector<uint8_t>& bytes);

}  // namespace manycore

