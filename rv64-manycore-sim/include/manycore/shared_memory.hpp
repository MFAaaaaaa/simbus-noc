#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace manycore {

class SharedMemory {
 public:
  std::vector<uint8_t> read(uint64_t address, size_t size) const;
  void write(uint64_t address, const std::vector<uint8_t>& data);
  uint64_t read_u64(uint64_t address, size_t size = 8) const;
  void write_u64(uint64_t address, uint64_t value, size_t size = 8);

 private:
  std::map<uint64_t, uint8_t> bytes_;
};

}  // namespace manycore

