// MIT License

// Copyright (c) 2024 Meng Chengzhen, in Shandong University

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "riscv64_config.hpp"
#include "riscv64_insts.hpp"

#include <vector>
#include <cstdint>
#include <optional>

namespace rv64archsem {

static_assert((SUPPORT_EXTENSION & EXT_BASE) != 0, "EXT_BASE must be enabled");
static_assert(((SUPPORT_EXTENSION & EXT_V) == 0) || ((SUPPORT_EXTENSION & EXT_F) != 0) || ((SUPPORT_EXTENSION & EXT_D) != 0),
              "If EXT_V is enabled, EXT_F or EXT_D must be enabled");

using std::vector;

enum class RV64LIB_API OPCode {
    load    = 0x03,
    loadfp  = 0x07,
    miscmem = 0x0f,
    opimm   = 0x13,
    auipc   = 0x17,
    opimm32 = 0x1b,
    store   = 0x23,
    storefp = 0x27,
    amo     = 0x2f,
    op      = 0x33,
    lui     = 0x37,
    op32    = 0x3b,
    madd    = 0x43,
    msub    = 0x47,
    nmsub   = 0x4b,
    nmadd   = 0x4f,
    opfp    = 0x53,
    opv     = 0x57,
    branch  = 0x63,
    jalr    = 0x67,
    jal     = 0x6f,
    system  = 0x73,
    nop     = 0
};

enum class RV64LIB_API InstType {
    R,
    I,
    S,
    B,
    U,
    J,
    R4,
    C,
    V,
    N
};

enum class RV64LIB_API RegType {
    none,
    gpreg,  // from general-purpose register
    freg,   // from floating-point register
    vreg,   // from vector register
};

enum class RV64LIB_API RDExtType {
    zero,
    sign8,
    sign16,
    sign32
};

enum class RV64LIB_API VRegView {
    none,   // from vector register with default layout
    m2,     // from vector register with SEW&LMUL *2 layout
    f2,     // from vector register with SEW&LMUL /2 layout
    f4,     // from vector register with SEW&LMUL /4 layout
    f8,     // from vector register with SEW&LMUL /8 layout
    ei8,    // from vector register with SEW=8 layout
    ei16,   // from vector register with SEW=16 layout
    ei32,   // from vector register with SEW=32 layout
    ei64,   // from vector register with SEW=64 layout
    mask,   // from vector register with mask layout
    e0,     // from vector register with one-element layout
    e0m2,   // from vector register with one-element SEW *2 layout
    reg1,   // from vector register with 1 whole register layout
    reg2,   // from vector register with 2 whole registers layout
    reg4,   // from vector register with 4 whole registers layout
    reg8    // from vector register with 8 whole registers layout
};

enum class RV64LIB_API ControlType {
    none,
    branch,
    direct_jump,
    indirect_jump,
    call,
    ret,
    load,
    store,
    vload,
    vstore,
    amo,
    csr,
    fence,
    fencei,
    fence_tso,
    sfence_vma,
    ecall,
    ebreak,
    mret,
    sret,
    uret,
    wfi
};

struct RV64LIB_API DecodedInst {
    uint32_t    rawInst     = 0;
    InstName    instName    = InstName::illegal;
    OPCode      opcode      = OPCode::nop;
    ControlType controlType = ControlType::none;
    InstType    instType    = InstType::N;
    int64_t     imm         = 0;
    RegType     rdType      = RegType::none;
    RegType     rs1Type     = RegType::none;
    RegType     rs2Type     = RegType::none;
    RegType     rs3Type     = RegType::none;
    uint8_t     rd          = 0;
    uint8_t     rs1         = 0;
    uint8_t     rs2         = 0;
    uint8_t     rs3         = 0;
    uint8_t     funct3      = 0;
    uint8_t     funct7      = 0;
    RDExtType   rdExt       = RDExtType::zero;
    VRegView    vrdView     = VRegView::none;
    VRegView    vrs1View    = VRegView::none;
    VRegView    vrs2View    = VRegView::none;
    VRegView    vrs3View    = VRegView::none;
};

/**
 * @brief Decode a RISC-V instruction
 * @param instWord The 32-bit instruction word (or 16-bit for compressed instructions)
 * @param inst The structure to hold the decoded instruction
 * @return true if decoding is successful, false otherwise
 */
RV64LIB_API bool decode(const uint32_t &instWord, DecodedInst &inst);

/**
 * @brief Execute a branch instruction (opcode == branch && controlType == branch)
 * @param inst The decoded instruction
 * @param rs1 The value of source register 1
 * @param rs2 The value of source register 2
 * @param pc The current program counter (will be modified if branch is taken)
 * @return true if the branch is taken, false otherwise
 */
RV64LIB_API bool exec_branch(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, uint64_t &pc);

/**
 * @brief Execute an OP-IMM instruction (opcode == opimm)
 * @param inst The decoded instruction
 * @param rs1 The value of source register 1
 * @return The result of the operation
 */
RV64LIB_API uint64_t exec_opimm(const DecodedInst& inst, const uint64_t &rs1);

/**
 * @brief Execute an OP-IMM32 instruction (opcode == opimm32)
 * @param inst The decoded instruction
 * @param rs1 The value of source register 1
 * @return The result of the operation
 */
RV64LIB_API uint64_t exec_opimm32(const DecodedInst& inst, const uint64_t &rs1);

/**
 * @brief Execute an AMO instruction (without memory access) (opcode == amo && funct7 != (LR or SC))
 * @param inst The decoded instruction
 * @param rs1 The value of source register 1
 * @param mem The memory value to be modified
 * @return The original value in memory before modification
 */
RV64LIB_API uint64_t exec_amo(const DecodedInst& inst, const uint64_t &rs1, uint64_t &mem);

/**
 * @brief Execute an OP instruction (opcode == op)
 * @param inst The decoded instruction
 * @param rs1 The value of source register 1
 * @param rs2 The value of source register 2
 * @return The result of the operation
 */
RV64LIB_API uint64_t exec_op(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2);

/**
 * @brief Execute an OP32 instruction (opcode == op32)
 * @param inst The decoded instruction
 * @param rs1 The value of source register 1
 * @param rs2 The value of source register 2
 * @return The result of the operation
 */
RV64LIB_API uint64_t exec_op32(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2);

/**
 * @brief Execute a CSR instruction (opcode == system && controlType == csr)
 * @param inst The decoded instruction
 * @param hibit The high bit of the CSR field
 * @param lowbit The low bit of the CSR field
 * @param rs1 The value of source register 1
 * @param csrVal The current value of the CSR to be accessed/modified
 * @return The result of the operation
 */
RV64LIB_API uint64_t exec_csr(const DecodedInst& inst, const uint32_t hibit, const uint32_t lowbit, const uint64_t &rs1, uint64_t &csrVal);

// RV64LIB_API uint64_t exec_madd(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3);
// RV64LIB_API uint64_t exec_msub(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3);
// RV64LIB_API uint64_t exec_nmsub(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3);
// RV64LIB_API uint64_t exec_nmadd(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3);
RV64LIB_API uint64_t exec_madd(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr);
RV64LIB_API uint64_t exec_msub(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr);
RV64LIB_API uint64_t exec_nmsub(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr);
RV64LIB_API uint64_t exec_nmadd(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr);

/**
 * @brief Execute an OP-FP instruction (opcode == opfp)
 * @param inst The decoded instruction
 * @param rs1 The value of source register 1
 * @param rs2 The value of source register 2
 * @param fcsr The floating-point control and status register (may be modified)
 * @return The result of the operation
 */
RV64LIB_API uint64_t exec_opfp(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, uint64_t &fcsr);

enum class RV64LIB_API RVVSEW {
    e8  = 0,
    e16 = 1,
    e32 = 2,
    e64 = 3,
};

enum class RV64LIB_API RVVLMUL {
    m1  = 0,    // 1
    m2  = 1,   // 2
    m4  = 2,   // 4
    m8  = 3,   // 8
    mf8 = 4,    // 1/8
    mf4 = 5,    // 1/4
    mf2 = 6,    // 1/2
};

struct RV64LIB_API VecContext {
    RVVSEW sew;     // Standard Element Width in 8, 16, 32, 64
    RVVLMUL lmul;   // Length Multiplier
    uint32_t vl;     // Vector Length
    uint32_t vstart; // Vector Start Index
    uint64_t vcsr;  // Vector Control and Status Register
    vector<uint8_t> vmask; // Vector Mask
};

RV64LIB_API uint32_t get_max_vector_length(const RVVSEW sew, const RVVLMUL lmul);

enum class RV64LIB_API VecEXResult {
    success,
    insufficient_vd,
    insufficient_vs1,
    insufficient_vs2,
    insufficient_vmask,
    unsupported_sew,
    invalid_widen_narrow,
    illegal_instruction
};

/**
 * @brief Execute an OP-V instruction (opcode == opv)
 * @param inst The decoded instruction
 * @param vctx The vector context (SEW, LMUL, VL, VStart)
 * @param vdraw The vector of result values (packed layout according to SEW and VL)
 * @param vrs1raw The vector of source register 1 values (packed layout according to SEW and VL)
 * @param vrs2raw The vector of source register 2 values (packed layout according to SEW and VL)
 * @param vcsr The vector control and status register (may be modified)
 * @return true if the instruction is executed successfully, false otherwise
 */
RV64LIB_API VecEXResult exec_opv(
    const DecodedInst& inst,
    VecContext& vctx,
    vector<uint64_t> &vdraw,
    const vector<uint64_t>& vrs1raw,
    const vector<uint64_t>& vrs2raw,
    uint64_t &fcsr
);


struct RV64LIB_API MemLoadDescriptor {
    uint64_t addr;              // address to load from
    uint16_t size;              // size of data to load in bytes
    uint16_t reg_offset;        // offset from rd begin to store loaded data
    uint16_t first_element_idx; // first element index in vector load, always 0 for scalar load
};

struct RV64LIB_API MemStoreDescriptor {
    vector<uint8_t> raw_data;   // raw data and size to store
    uint64_t addr;              // address to store to
    uint16_t first_element_idx; // first element index in this vector store, always 0 for scalar store
};

enum class RV64LIB_API MemParseResult {
    success,
    illegal_instruction,
    insufficient_vs2,
    insufficient_vs3,
    insufficient_vmask,
    misaligned_address,
};

/**
 * @brief Parse a load instruction to get memory load descriptors (controlType == load)
 * @param inst The decoded instruction
 * @param rs1val The value of source register 1 (base address)
 * @param boundary_bytes A mem-access should not cross boundary_bytes alignment (0 means no restriction)
 * @param misalign_error Whether to treat misaligned access as an error
 * @param desc The vector to hold memory load descriptors
 * @return MemParseResult indicating success or type of error
 */
RV64LIB_API MemParseResult parse_load_scalar(
    const DecodedInst& inst,
    const uint64_t rs1val,
    const uint8_t boundary_bytes,
    const bool misalign_error,
    vector<MemLoadDescriptor>& desc
);

/**
 * @brief Parse a vector load instruction to get memory load descriptors (controlType == vload)
 * @param inst The decoded instruction
 * @param vctx The vector context (SEW, LMUL, VL, VMask)
 * @param vstart The starting index in the vector operation (will be updated if exception occurs)
 * @param rs1val The value of source register 1 (base address)
 * @param vs2raw The raw data of source vector register 2 (for indexed loads)
 * @param boundary_bytes A mem-access should not cross boundary_bytes alignment (0 means no restriction)
 * @param misalign_error Whether to treat misaligned access as an error
 * @param desc The vector to hold memory load descriptors
 * @return MemParseResult indicating success or type of error
 */
RV64LIB_API MemParseResult parse_load_vector(
    const DecodedInst& inst,
    VecContext& vctx,
    const uint64_t rs1val,
    const vector<uint64_t>& vs2raw,
    const uint8_t boundary_bytes,
    const bool misalign_error,
    vector<MemLoadDescriptor>& desc
);

/**
 * @brief Parse a store instruction to get memory store descriptors (controlType == store)
 * @param inst The decoded instruction
 * @param rs1val The value of source register 1 (base address)
 * @param rs2val The value of source register 2 (data to store)
 * @param boundary_bytes A mem-access should not cross boundary_bytes alignment (0 means no restriction)
 * @param misalign_error Whether to treat misaligned access as an error
 * @param desc The vector to hold memory store descriptors
 * @return MemParseResult indicating success or type of error
 */
RV64LIB_API MemParseResult parse_store_scalar(
    const DecodedInst& inst,
    const uint64_t rs1val,
    const uint64_t rs2val,
    const uint8_t boundary_bytes,
    const bool misalign_error,
    vector<MemStoreDescriptor>& desc
);

/**
 * @brief Parse a vector store instruction to get memory store descriptors (controlType == vstore)
 * @param inst The decoded instruction
 * @param vctx The vector context (SEW, LMUL, VL, VMask)
 * @param vstart The starting index in the vector operation (will be updated if exception occurs)
 * @param rs1val The value of source register 1 (base address)
 * @param vs2raw The raw data of source vector register 2 (index or stride)
 * @param vs3raw The raw data of source vector register 3 (data to store)
 * @param boundary_bytes A mem-access should not cross boundary_bytes alignment (0 means no restriction)
 * @param misalign_error Whether to treat misaligned access as an error
 * @param desc The vector to hold memory store descriptors
 * @return MemParseResult indicating success or type of error
 */
RV64LIB_API MemParseResult parse_store_vector(
    const DecodedInst& inst,
    VecContext& vctx,
    const uint64_t rs1val,
    const vector<uint64_t>& vs2raw,
    const vector<uint64_t>& vs3raw,
    const uint8_t boundary_bytes,
    const bool misalign_error,
    vector<MemStoreDescriptor>& desc
);



} // namespace rv64archsem
