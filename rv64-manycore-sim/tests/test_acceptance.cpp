#include "manycore/rv64_manycore_system.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>

#define CHECK(x) do { if (!(x)) throw std::runtime_error("failed: " #x); } while (0)
using namespace manycore;

void test_receive_reservations() {
  simbus::BusRouteTable route;
  simbus::genroute_double_ring({0, 1, 2}, route);
  simbus::NocConfig config;
  config.link_width_byte = 4;
  config.receive_message_slots = 1;
  config.max_message_bytes = 128;
  config.congestion.node_buffer_limit = 2;
  config.congestion.high_watermark = 2;
  config.congestion.low_watermark = 1;
  simbus::SymmetricMultiChannelBus bus({0, 1, 2}, {0, 1, 2}, {4, 4}, route, config);
  std::vector<uint8_t> payload(97, 0x6a), received;
  CHECK(bus.send(0, 2, 0, payload));
  CHECK(bus.reserved_messages(2, 0) == 1);
  CHECK(!bus.send(1, 2, 0, payload));
  CHECK(bus.send(1, 2, 1, payload)); // independent response-channel quota
  for (int i = 0; i < 2000; ++i) bus.apply_next_tick();
  CHECK(bus.reserved_messages(2, 0) == 1); // arrival does not release credit
  CHECK(!bus.send(1, 2, 0, payload));
  CHECK(bus.recv(2, 0, received)); CHECK(received == payload);
  CHECK(bus.reserved_messages(2, 0) == 0);
  CHECK(bus.recv(2, 1, received)); CHECK(received == payload);
  CHECK(bus.send(1, 2, 0, payload));
  for (int i = 0; i < 2000; ++i) bus.apply_next_tick();
  CHECK(bus.recv(2, 0, received)); CHECK(received == payload);
  CHECK(bus.statistics().destination_credit_stalls >= 2);
  CHECK(bus.statistics().max_reserved_messages == 1);
  bool rejected = false;
  try { bus.send(0, 2, 0, std::vector<uint8_t>(129)); }
  catch (const std::runtime_error&) { rejected = true; }
  CHECK(rejected); CHECK(bus.reserved_messages(2, 0) == 0);
}

void test_adapter_validation_and_response_stall() {
  simbus::BusRouteTable route;
  simbus::genroute_double_ring({0, 1}, route);
  simbus::NocConfig config;
  config.receive_message_slots = 1;
  simbus::SymmetricMultiChannelBus bus({0, 1}, {0, 1}, {8, 8}, route, config);
  MemoryController memory(bus, 1, {5, 1});
  MmioBusAdapter adapter(0, bus, 0, 1);
  const auto base = MmioBusAdapter::base;
  auto reg = [&](uint64_t off, uint64_t value) { adapter.write(base + off, 8, value); };
  auto status = [&]() { return adapter.read(base + 32, 8); };
  auto drain = [&]() {
    for (int i = 0; i < 5000 && (adapter.busy() || !memory.idle()); ++i) {
      memory.tick(); adapter.tick(); bus.apply_next_tick();
    }
    CHECK(!adapter.busy()); CHECK(memory.idle());
  };
  reg(16, 3); reg(24, 1); CHECK(status() == 4); CHECK(adapter.stats().commands == 0);
  reg(16, 8); reg(0, base); reg(24, 1); CHECK(status() == 4);
  reg(0, std::numeric_limits<uint64_t>::max()); reg(24, 1); CHECK(status() == 4);
  reg(0, 0x6000); reg(24, 99); CHECK(status() == 4);
  bool rejected = false;
  try { adapter.read(base, 4); } catch (const std::runtime_error&) { rejected = true; }
  CHECK(rejected);
  for (uint64_t width : {1, 2, 4, 8}) {
    reg(16, width); reg(8, 0xfedcba9876543210ull); reg(24, 2);
    CHECK(status() == 1);
    reg(24, 2); CHECK(status() == 5); // busy command rejected, original survives
    drain(); CHECK(status() == 6);
    reg(32, 6); CHECK(status() == 0);
    reg(24, 1); drain(); CHECK(status() == 2);
    const uint64_t mask = width == 8 ? ~uint64_t{0} : (uint64_t{1} << (width * 8)) - 1;
    CHECK(adapter.read(base + 40, 8) == (0xfedcba9876543210ull & mask));
  }
  // Hold the sole response credit, then verify the controller waits and recovers.
  CHECK(bus.send(1, 0, 1, encode_response({MemoryStatus::Ok, 0, 999, {}})));
  reg(24, 1);
  for (int i = 0; i < 200; ++i) { memory.tick(); bus.apply_next_tick(); if (i == 0) adapter.tick(); }
  CHECK(memory.stats().response_injection_blocked_cycles > 0);
  std::vector<uint8_t> discarded;
  CHECK(bus.recv(0, 1, discarded));
  drain(); CHECK(status() == 2);
  CHECK(adapter.stats().commands == 9); CHECK(adapter.stats().completed == 9);
}

uint64_t contention(uint32_t service) {
  ManycoreConfig cfg;
  cfg.core_count = 8; cfg.receive_message_slots = 1;
  cfg.memory_latency = service; cfg.memory_queue_capacity = 1;
  cfg.link_width_bytes = 4; cfg.router_buffer_limit = 2;
  ManycoreSystem system(cfg);
  for (uint32_t hart = 0; hart < 8; ++hart) {
    std::vector<ScriptOp> ops;
    for (int i = 0; i < 16; ++i) ops.push_back(ScriptOp::write(0x1000 + hart * 8, {uint8_t(hart)}));
    ops.push_back(ScriptOp::halt()); system.core(hart).load_program(ops);
  }
  CHECK(system.run(200000));
  CHECK(system.memory_controller().stats().completed_requests == 128);
  CHECK(system.noc().statistics().max_reserved_messages == 1);
  CHECK(system.noc().statistics().destination_credit_stalls > 0);
  if (service == 100) {
    CHECK(system.memory_controller().stats().queue_full_cycles > 0);
    CHECK(system.memory_controller().stats().service_cycles == 12800);
  }
  for (uint32_t hart = 0; hart < 8; ++hart) {
    CHECK(system.core(hart).stats().received_responses == 16);
    CHECK(system.memory_controller().memory().read_u64(0x1000 + hart * 8, 1) == hart);
  }
  return system.current_tick();
}

void test_exact_execution_latency() {
  auto image = ProgramImage::from_words({0x00100093, 0x00208093, 0x00100073}); // addi, addi, ebreak
  ManycoreConfig cfg; cfg.core_count = 1;
  InstructionLatencyConfig l;
  Rv64ManycoreSystem fast(image, cfg, l);
  CHECK(fast.run(100));
  l.integer_alu = 5;
  Rv64ManycoreSystem slow(image, cfg, l);
  CHECK(slow.run(100));
  CHECK(slow.current_tick() == fast.current_tick() + 12); // includes ebreak default category
  CHECK(slow.core(0).reg(1) == 3);
  CHECK(slow.core(0).stats().fetch_decode_cycles == 3);
}

void test_halt_drains_mmio() {
  auto i = [](uint32_t imm, uint32_t rs, uint32_t f, uint32_t rd) {
    return imm << 20 | rs << 15 | f << 12 | rd << 7 | 0x13u;
  };
  auto sd = [](uint32_t imm, uint32_t rs) {
    return (imm >> 5) << 25 | rs << 20 | 5u << 15 | 3u << 12 | (imm & 31) << 7 | 0x23u;
  };
  auto image = ProgramImage::from_words({
      i(9, 0, 0, 5), i(28, 5, 1, 5), // x5 = 0x90000000
      0x00006337u, sd(0, 6),           // target=0x6000
      i(77, 0, 0, 6), sd(8, 6),       // data=77
      i(2, 0, 0, 6), sd(24, 6),       // launch write
      0x00100073u});                   // halt with write still outstanding
  ManycoreConfig cfg; cfg.core_count = 1; cfg.memory_latency = 100;
  Rv64ManycoreSystem system(image, cfg);
  CHECK(system.run(2000));
  CHECK(system.program_succeeded());
  CHECK(system.memory_controller().memory().read_u64(0x6000) == 77);
  CHECK(system.core(0).mmio().stats().completed == 1);
}

int main() {
  try {
    test_receive_reservations();
    test_adapter_validation_and_response_stall();
    CHECK(contention(100) > contention(2));
    test_exact_execution_latency();
    test_halt_drains_mmio();
    std::cout << "MMIO_CREDITS_LATENCY_PASS\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
