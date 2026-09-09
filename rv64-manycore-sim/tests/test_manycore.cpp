#include "manycore/manycore_system.hpp"
#include "manycore/rv64_manycore_system.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

#define CHECK(expr) do { if (!(expr)) { \
  std::cerr << "CHECK failed: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; \
  std::exit(1); } } while (false)

std::vector<uint8_t> le64(uint64_t value) {
  std::vector<uint8_t> out(8);
  for (size_t i = 0; i < out.size(); ++i) out[i] = static_cast<uint8_t>(value >> (i * 8));
  return out;
}

void test_protocol_and_shared_memory() {
  manycore::MemoryRequest req;
  req.op = manycore::MemoryOp::Write;
  req.hart_id = 7;
  req.transaction_id = 42;
  req.address = 0x1234;
  req.size = 8;
  req.data = le64(0xfeedfacecafebeefull);
  const auto decoded = manycore::decode_request(manycore::encode_request(req));
  CHECK(decoded.hart_id == 7);
  CHECK(decoded.transaction_id == 42);
  CHECK(decoded.address == 0x1234);
  CHECK(decoded.data == req.data);

  manycore::SharedMemory mem;
  mem.write_u64(0x80, 0x1122334455667788ull);
  CHECK(mem.read_u64(0x80) == 0x1122334455667788ull);
}

void test_two_core_round_trip() {
  manycore::ManycoreConfig cfg;
  cfg.core_count = 2;
  cfg.link_width_bytes = 16;
  cfg.route_latency = 2;
  cfg.router_buffer_limit = 4;
  cfg.memory_latency = 5;
  cfg.memory_queue_capacity = 2;
  manycore::ManycoreSystem system(cfg);

  system.core(0).load_program({manycore::ScriptOp::write(0x100, le64(11)),
                               manycore::ScriptOp::read(0x200, 8),
                               manycore::ScriptOp::halt()});
  system.core(1).load_program({manycore::ScriptOp::write(0x200, le64(22)),
                               manycore::ScriptOp::read(0x100, 8),
                               manycore::ScriptOp::halt()});
  CHECK(system.run(5000));
  CHECK(system.memory_controller().memory().read_u64(0x100) == 11);
  CHECK(system.memory_controller().memory().read_u64(0x200) == 22);
  CHECK(system.core(0).last_read_data() == le64(22));
  CHECK(system.core(1).last_read_data() == le64(11));
  CHECK(system.core(0).stats().retired_ops == 2);
  CHECK(system.core(1).stats().retired_ops == 2);
}

uint64_t run_contention_case(uint32_t memory_latency) {
  manycore::ManycoreConfig cfg;
  cfg.core_count = 4;
  cfg.link_width_bytes = 8;
  cfg.route_latency = 3;
  cfg.router_buffer_limit = 2;
  cfg.memory_latency = memory_latency;
  cfg.memory_queue_capacity = 1;
  manycore::ManycoreSystem system(cfg);
  for (size_t i = 0; i < 4; ++i) {
    system.core(i).load_program({
        manycore::ScriptOp::write(0x1000 + i * 8, le64(i + 1)),
        manycore::ScriptOp::read(0x1000 + ((i + 1) % 4) * 8, 8),
        manycore::ScriptOp::halt(),
    });
  }
  CHECK(system.run(20000));
  CHECK(system.memory_controller().stats().received_requests == 8);
  return system.current_tick();
}

void test_memory_contention_increases_runtime() {
  const uint64_t fast = run_contention_case(2);
  const uint64_t slow = run_contention_case(20);
  CHECK(slow > fast);
}

uint32_t encode_i(int32_t imm, uint8_t rs1, uint8_t funct3, uint8_t rd, uint8_t opcode) {
  return (static_cast<uint32_t>(imm) & 0xfffu) << 20 |
         uint32_t{rs1} << 15 | uint32_t{funct3} << 12 | uint32_t{rd} << 7 | opcode;
}

uint32_t encode_r(uint8_t funct7, uint8_t rs2, uint8_t rs1, uint8_t funct3,
                  uint8_t rd, uint8_t opcode = 0x33) {
  return uint32_t{funct7} << 25 | uint32_t{rs2} << 20 | uint32_t{rs1} << 15 |
         uint32_t{funct3} << 12 | uint32_t{rd} << 7 | opcode;
}

uint32_t encode_s(int32_t imm, uint8_t rs2, uint8_t rs1, uint8_t funct3) {
  const uint32_t value = static_cast<uint32_t>(imm) & 0xfffu;
  return ((value >> 5) << 25) | uint32_t{rs2} << 20 | uint32_t{rs1} << 15 |
         uint32_t{funct3} << 12 | (value & 0x1fu) << 7 | 0x23u;
}

uint32_t encode_u(uint32_t upper, uint8_t rd) {
  return (upper << 12) | uint32_t{rd} << 7 | 0x37u;
}

uint32_t encode_csr(uint16_t csr, uint8_t rs1, uint8_t funct3, uint8_t rd) {
  return uint32_t{csr} << 20 | uint32_t{rs1} << 15 | uint32_t{funct3} << 12 |
         uint32_t{rd} << 7 | 0x73u;
}

void test_rv64_integer_latency() {
  const auto image = manycore::ProgramImage::from_words({
      encode_i(6, 0, 0, 1, 0x13),       // addi x1,x0,6
      encode_i(3, 0, 0, 2, 0x13),       // addi x2,x0,3
      encode_r(1, 2, 1, 0, 3),          // mul x3,x1,x2
      encode_r(1, 2, 3, 4, 4),          // div x4,x3,x2
      0x00000073u,                       // ecall
  });
  manycore::ManycoreConfig cfg;
  cfg.core_count = 1;
  manycore::InstructionLatencyConfig latency;
  latency.integer_multiply = 4;
  latency.integer_divide = 11;
  manycore::Rv64ManycoreSystem system(image, cfg, latency);
  CHECK(system.run(1000));
  CHECK(system.core(0).reg(3) == 18);
  CHECK(system.core(0).reg(4) == 6);
  CHECK(system.core(0).stats().retired_instructions == 4);
  CHECK(system.core(0).stats().execute_wait_cycles >= 17);
}

void test_rv64_multicore_noc_load_store() {
  const auto image = manycore::ProgramImage::from_words({
      encode_csr(0xf14, 0, 2, 2),        // csrrs x2,mhartid,x0
      encode_i(3, 2, 1, 3, 0x13),        // slli x3,x2,3
      encode_u(1, 1),                     // lui x1,0x1
      encode_r(0, 3, 1, 0, 1),           // add x1,x1,x3
      encode_i(1, 2, 0, 4, 0x13),        // addi x4,x2,1
      encode_s(0, 4, 1, 3),               // sd x4,0(x1)
      encode_i(0, 1, 3, 5, 0x03),         // ld x5,0(x1)
      0x00000073u,                         // ecall
  });
  manycore::ManycoreConfig cfg;
  cfg.core_count = 4;
  cfg.link_width_bytes = 8;
  cfg.route_latency = 3;
  cfg.router_buffer_limit = 2;
  cfg.memory_latency = 12;
  cfg.memory_queue_capacity = 1;
  manycore::Rv64ManycoreSystem system(image, cfg);
  CHECK(system.run(20000));
  for (uint32_t hart = 0; hart < 4; ++hart) {
    CHECK(system.memory_controller().memory().read_u64(0x1000 + hart * 8) == hart + 1);
    CHECK(system.core(hart).reg(5) == hart + 1);
    CHECK(system.core(hart).stats().loads == 1);
    CHECK(system.core(hart).stats().stores == 1);
    CHECK(system.core(hart).stats().memory_wait_cycles > 0);
  }
  CHECK(system.noc().statistics().blocked_cycles > 0);
}

manycore::ProgramImage make_tohost_program(uint16_t value) {
  return manycore::ProgramImage::from_words({
      encode_i(value, 0, 0, 1, 0x13),   // addi x1,x0,value
      encode_u(1, 2),                    // lui x2,0x1
      encode_s(0, 1, 2, 3),              // sd x1,0(x2)
      0x0000006fu,                        // jal x0,0 (must not be reached)
  });
}

void test_tohost_termination_protocol() {
  manycore::ManycoreConfig pass_cfg;
  pass_cfg.core_count = 4;
  pass_cfg.link_width_bytes = 8;
  pass_cfg.route_latency = 3;
  pass_cfg.router_buffer_limit = 2;
  pass_cfg.memory_latency = 10;
  pass_cfg.memory_queue_capacity = 1;
  pass_cfg.tohost_address = 0x1000;
  manycore::Rv64ManycoreSystem pass_system(make_tohost_program(1), pass_cfg);
  CHECK(pass_system.run(20000));
  CHECK(pass_system.program_succeeded());
  for (uint32_t hart = 0; hart < pass_cfg.core_count; ++hart) {
    CHECK(pass_system.core(hart).exit_code() == 0);
    CHECK(pass_system.core(hart).stop_reason() == "tohost pass");
    CHECK(pass_system.core(hart).stats().retired_instructions == 3);
  }

  manycore::ManycoreConfig fail_cfg;
  fail_cfg.core_count = 1;
  fail_cfg.tohost_address = 0x1000;
  manycore::Rv64ManycoreSystem fail_system(make_tohost_program(5), fail_cfg);
  CHECK(fail_system.run(1000));
  CHECK(!fail_system.program_succeeded());
  CHECK(fail_system.core(0).exit_code() == 2);
  CHECK(fail_system.core(0).stop_reason() == "tohost failure 2");
}

void test_rv64_vector_stage1() {
  const auto image = manycore::ProgramImage::from_words({
      encode_i(4, 0, 0, 5, 0x13),        // addi t0,x0,4
      0x0102f357u,                        // vsetvli t1,t0,e32,m1,tu,mu
      encode_csr(0xc20, 0, 2, 8),         // csrr s0,vl
      encode_csr(0xc21, 0, 2, 9),         // csrr s1,vtype
      encode_csr(0xc22, 0, 2, 11),        // csrr a1,vlenb
      0x5e01b0d7u,                        // vmv.v.i v1,3
      0x5e023157u,                        // vmv.v.i v2,4
      0x021101d7u,                        // vadd.vv v3,v1,v2
      0x423023d7u,                        // vmv.x.s t2,v3
      0x00000073u,                        // ecall
  });
  manycore::ManycoreConfig cfg;
  cfg.core_count = 1;
  manycore::InstructionLatencyConfig latency;
  latency.vector_config = 2;
  latency.vector_integer_alu = 6;
  manycore::Rv64ManycoreSystem system(image, cfg, latency);
  CHECK(system.run(2000));
  CHECK(system.program_succeeded());
  CHECK(system.core(0).vl() == 4);
  CHECK(system.core(0).vtype() == 0x10);
  CHECK(system.core(0).reg(7) == 7);
  CHECK(system.core(0).reg(8) == 4);
  CHECK(system.core(0).reg(9) == 0x10);
  CHECK(system.core(0).reg(11) == 32);
  CHECK(system.core(0).vector_reg_word(3, 0) == 0x0000000700000007ull);
  CHECK(system.core(0).vector_reg_word(3, 1) == 0x0000000700000007ull);
  CHECK(system.core(0).stats().vector_config_instructions == 1);
  CHECK(system.core(0).stats().vector_alu_instructions == 4);
  CHECK(system.core(0).stats().execute_wait_cycles >= 27);

  const auto lmul2_image = manycore::ProgramImage::from_words({
      encode_i(4, 0, 0, 5, 0x13),        // addi t0,x0,4
      0x0112f357u,                        // vsetvli t1,t0,e32,m2,tu,mu
      0x5e01b157u,                        // vmv.v.i v2,3
      0x5e023257u,                        // vmv.v.i v4,4
      0x02220357u,                        // vadd.vv v6,v2,v4
      0x426023d7u,                        // vmv.x.s t2,v6
      0x00000073u,                        // ecall
  });
  manycore::Rv64ManycoreSystem lmul2_system(lmul2_image, cfg);
  CHECK(lmul2_system.run(1000));
  CHECK(lmul2_system.core(0).reg(7) == 7);
  CHECK(lmul2_system.core(0).vtype() == 0x11);
  CHECK(lmul2_system.core(0).vector_reg_word(6, 0) ==
        0x0000000700000007ull);
}

void test_rv64_vector_unit_stride_memory() {
  const auto image = manycore::ProgramImage::from_words({
      encode_u(1, 5),                     // lui t0,0x1
      encode_i(1, 0, 0, 6, 0x13),
      encode_s(0, 6, 5, 2),               // sw t1,0(t0)
      encode_i(2, 0, 0, 6, 0x13),
      encode_s(4, 6, 5, 2),               // sw t1,4(t0)
      encode_i(3, 0, 0, 6, 0x13),
      encode_s(8, 6, 5, 2),               // sw t1,8(t0)
      encode_i(4, 0, 0, 6, 0x13),
      encode_s(12, 6, 5, 2),              // sw t1,12(t0)
      encode_i(0x100, 5, 0, 7, 0x13),     // addi t2,t0,0x100
      encode_i(4, 0, 0, 8, 0x13),         // addi s0,x0,4
      0x010474d7u,                        // vsetvli s1,s0,e32,m1,tu,mu
      0x0202e107u,                        // vle32.v v2,(t0)
      0x0220b257u,                        // vadd.vi v4,v2,1
      0x0203e227u,                        // vse32.v v4,(t2)
      encode_i(0, 7, 2, 14, 0x03),        // lw a4,0(t2)
      encode_i(4, 7, 2, 15, 0x03),        // lw a5,4(t2)
      encode_i(8, 7, 2, 16, 0x03),        // lw a6,8(t2)
      encode_i(12, 7, 2, 17, 0x03),       // lw a7,12(t2)
      0x00000073u,                        // ecall
  });
  manycore::ManycoreConfig cfg;
  cfg.core_count = 4;
  cfg.link_width_bytes = 8;
  cfg.route_latency = 3;
  cfg.router_buffer_limit = 2;
  cfg.memory_latency = 10;
  cfg.memory_queue_capacity = 1;
  manycore::Rv64ManycoreSystem system(image, cfg);
  CHECK(system.run(50000));
  CHECK(system.program_succeeded());
  for (uint32_t hart = 0; hart < cfg.core_count; ++hart) {
    CHECK(system.core(hart).reg(14) == 2);
    CHECK(system.core(hart).reg(15) == 3);
    CHECK(system.core(hart).reg(16) == 4);
    CHECK(system.core(hart).reg(17) == 5);
    CHECK(system.core(hart).stats().vector_load_instructions == 1);
    CHECK(system.core(hart).stats().vector_store_instructions == 1);
    CHECK(system.core(hart).stats().vector_memory_transactions == 4);
  }
  CHECK(system.memory_controller().memory().read_u64(0x1100) ==
        0x0000000300000002ull);
  CHECK(system.memory_controller().memory().read_u64(0x1108) ==
        0x0000000500000004ull);
  CHECK(system.noc().statistics().blocked_cycles > 0);
}

}  // namespace

int main() {
  test_protocol_and_shared_memory();
  test_two_core_round_trip();
  test_memory_contention_increases_runtime();
  test_rv64_integer_latency();
  test_rv64_multicore_noc_load_store();
  test_tohost_termination_protocol();
  test_rv64_vector_stage1();
  test_rv64_vector_unit_stride_memory();
  std::cout << "[SUMMARY] 8 tests passed\n";
  return 0;
}
