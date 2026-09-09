#include "manycore/shared_memory.hpp"

#include <stdexcept>

namespace manycore {

std::vector<uint8_t> SharedMemory::read(uint64_t address, size_t size) const {
  std::vector<uint8_t> out(size, 0);
  for (size_t i = 0; i < size; ++i) {
    const auto it = bytes_.find(address + i);
    if (it != bytes_.end()) out[i] = it->second;
  }
  return out;
}

void SharedMemory::write(uint64_t address, const std::vector<uint8_t>& data) {
  for (size_t i = 0; i < data.size(); ++i) bytes_[address + i] = data[i];
}

uint64_t SharedMemory::read_u64(uint64_t address, size_t size) const {
  if (size > 8) throw std::runtime_error("read_u64 size exceeds 8");
  const auto data = read(address, size);
  uint64_t value = 0;
  for (size_t i = 0; i < data.size(); ++i) value |= uint64_t{data[i]} << (i * 8);
  return value;
}

void SharedMemory::write_u64(uint64_t address, uint64_t value, size_t size) {
  if (size > 8) throw std::runtime_error("write_u64 size exceeds 8");
  std::vector<uint8_t> data(size);
  for (size_t i = 0; i < size; ++i) data[i] = static_cast<uint8_t>(value >> (i * 8));
  write(address, data);
}

}  // namespace manycore

