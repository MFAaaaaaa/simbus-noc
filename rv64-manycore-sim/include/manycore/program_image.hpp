#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace manycore {

class ProgramImage {
 public:
  explicit ProgramImage(uint64_t base_address = 0x80000000ull);

  static ProgramImage from_file(const std::string& path,
                                uint64_t base_address = 0x80000000ull);
  static ProgramImage from_words(const std::vector<uint32_t>& words,
                                 uint64_t base_address = 0x80000000ull);

  uint16_t fetch16(uint64_t address) const;
  uint32_t fetch32(uint64_t address) const;
  uint64_t base_address() const { return base_address_; }
  size_t size() const { return bytes_.size(); }

 private:
  uint8_t byte_at(uint64_t address) const;

  uint64_t base_address_;
  std::vector<uint8_t> bytes_;
};

}  // namespace manycore
