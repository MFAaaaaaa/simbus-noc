#include "manycore/program_image.hpp"

#include <fstream>
#include <stdexcept>

namespace manycore {

ProgramImage::ProgramImage(uint64_t base_address) : base_address_(base_address) {}

ProgramImage ProgramImage::from_file(const std::string& path, uint64_t base_address) {
  ProgramImage image(base_address);
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::runtime_error("cannot open program image: " + path);
  char ch = 0;
  while (input.get(ch)) image.bytes_.push_back(static_cast<uint8_t>(ch));
  if (image.bytes_.empty()) throw std::runtime_error("program image is empty: " + path);
  return image;
}

ProgramImage ProgramImage::from_words(const std::vector<uint32_t>& words,
                                      uint64_t base_address) {
  ProgramImage image(base_address);
  image.bytes_.reserve(words.size() * 4);
  for (uint32_t word : words) {
    for (unsigned i = 0; i < 4; ++i) image.bytes_.push_back(static_cast<uint8_t>(word >> (i * 8)));
  }
  return image;
}

uint8_t ProgramImage::byte_at(uint64_t address) const {
  if (address < base_address_ || address - base_address_ >= bytes_.size()) {
    throw std::runtime_error("instruction fetch outside program image");
  }
  return bytes_[static_cast<size_t>(address - base_address_)];
}

uint16_t ProgramImage::fetch16(uint64_t address) const {
  return static_cast<uint16_t>(byte_at(address)) |
         (static_cast<uint16_t>(byte_at(address + 1)) << 8);
}

uint32_t ProgramImage::fetch32(uint64_t address) const {
  uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i) value |= static_cast<uint32_t>(byte_at(address + i)) << (i * 8);
  return value;
}

}  // namespace manycore
