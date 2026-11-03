#pragma once

#include "manycore/memory_protocol.hpp"
#include "simbus/businterface.h"
#include <optional>
#include <deque>
#include "manycore/interconnect.hpp"

namespace manycore {

// Each hart sees its own local register window at this same address.
class MmioBusAdapter {
 public:
  static constexpr uint64_t base = 0x90000000ull;
  static constexpr uint64_t span = 0x100;
  static constexpr uint64_t busy_bit = 1, done_bit = 2, error_bit = 4;
  struct Stats {
    uint64_t commands = 0, completed = 0, errors = 0;
    uint64_t injection_wait = 0, response_wait = 0;
    Distribution latency;
  };
  MmioBusAdapter(uint32_t hart, simbus::BusInterfaceV2& bus,
                 simbus::BusPortT port, simbus::BusPortT memory_port);
  static bool contains(uint64_t address) { return address >= base && address < base + span; }
  uint64_t read(uint64_t address, uint16_t size);
  void write(uint64_t address, uint16_t size, uint64_t value);
  void tick();
  bool busy() const { return !requests_.empty(); }
  void set_depth(uint32_t n) {if(!n||busy())throw std::runtime_error("invalid MMIO depth");depth_=n;}
  const Stats& stats() const { return stats_; }

 private:
  uint64_t offset(uint64_t address, uint16_t size) const;
  void reject();
  uint32_t hart_;
  simbus::BusInterfaceV2& bus_;
  simbus::BusPortT port_, memory_port_;
  uint64_t target_ = 0, write_data_ = 0, size_ = 8, read_data_ = 0, status_ = 0;
  struct Entry {MemoryRequest request;bool sent=false;uint64_t created=0;};
  struct Result {uint32_t id;uint64_t data;bool error;};
  std::deque<Entry> requests_;
  std::deque<Result> results_;
  uint32_t depth_=1,next_id_=0;
  uint64_t clock_=0;
  Stats stats_;
};
}  // namespace manycore
