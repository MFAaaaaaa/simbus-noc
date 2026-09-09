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

#include "riscv64.hpp"

#include "rvi/bitpack.hpp"
#include "intcast.hpp"

#include <bit>

using std::bit_cast;

namespace rv64archsem {

constexpr uint64_t mulh_impl(const uint64_t &a, const uint64_t &b) {
    __int128_t signed_a = static_cast<__int128_t>(bit_cast<int64_t>(a));
    __int128_t signed_b = static_cast<__int128_t>(bit_cast<int64_t>(b));
    __int128_t result = signed_a * signed_b;
    __uint128_t unsigned_result = static_cast<__uint128_t>(result);
    return static_cast<uint64_t>(unsigned_result >> 64);
}

constexpr uint64_t mulhsu_impl(const uint64_t &a, const uint64_t &b) {
    __int128_t signed_a = static_cast<__int128_t>(bit_cast<int64_t>(a));
    __uint128_t unsigned_b = static_cast<__uint128_t>(b);
    __int128_t result = signed_a * static_cast<__int128_t>(unsigned_b);
    return static_cast<uint64_t>(result >> 64);
}

constexpr uint64_t mulhu_impl(const uint64_t &a, const uint64_t &b) {
    __uint128_t unsigned_a = static_cast<__uint128_t>(a);
    __uint128_t unsigned_b = static_cast<__uint128_t>(b);
    __uint128_t result = unsigned_a * unsigned_b;
    return static_cast<uint64_t>(result >> 64);
}


bool exec_branch(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, uint64_t &pc) {
    bool res = false;
    switch (static_cast<RV64BranchOP3>(inst.funct3))
    {
    case RV64BranchOP3::BEQ : res = (bit_cast<uint64_t>(rs1) == bit_cast<uint64_t>(rs2)); break;
    case RV64BranchOP3::BNE : res = (bit_cast<uint64_t>(rs1) != bit_cast<uint64_t>(rs2)); break;
    case RV64BranchOP3::BLT : res = (bit_cast<int64_t>(rs1) < bit_cast<int64_t>(rs2)); break;
    case RV64BranchOP3::BGE : res = (bit_cast<int64_t>(rs1) >= bit_cast<int64_t>(rs2)); break;
    case RV64BranchOP3::BLTU: res = (bit_cast<uint64_t>(rs1) < bit_cast<uint64_t>(rs2)); break;
    case RV64BranchOP3::BGEU: res = (bit_cast<uint64_t>(rs1) >= bit_cast<uint64_t>(rs2)); break;
    }
    if (res) {
        pc = static_cast<uint64_t>(static_cast<int64_t>(pc) + inst.imm);
    }
    return res;
}

uint64_t exec_opimm(const DecodedInst& inst, const uint64_t &rs1) {
    const uint64_t imm = static_cast<uint64_t>(inst.imm);
    switch (inst.instName)
    {
    case InstName::addi:  return rs1 + imm;
    case InstName::slli:  return rs1 << (imm & 0x3F);
    case InstName::slti:  return (static_cast<int64_t>(rs1) < inst.imm) ? 1 : 0;
    case InstName::sltiu: return (rs1 < imm) ? 1 : 0;
    case InstName::xori:  return rs1 ^ imm;
    case InstName::srli:  return rs1 >> (imm & 0x3F);
    case InstName::srai:  return static_cast<uint64_t>(static_cast<int64_t>(rs1) >> (imm & 0x3F));
    case InstName::ori:   return rs1 | imm;
    case InstName::andi:  return rs1 & imm;
    default: return 0;
    }
}

uint64_t exec_opimm32(const DecodedInst& inst, const uint64_t &rs1) {
    const uint32_t a = static_cast<uint32_t>(rs1);
    const uint64_t imm = static_cast<uint64_t>(inst.imm);
    uint32_t result = 0;
    switch (inst.instName)
    {
    case InstName::addiw: result = a + static_cast<uint32_t>(imm); break;
    case InstName::slliw: result = a << (imm & 0x1F); break;
    case InstName::srliw: result = a >> (imm & 0x1F); break;
    case InstName::sraiw: result = static_cast<uint32_t>(static_cast<int32_t>(a) >> (imm & 0x1F)); break;
    default: return 0;
    }
    return int_cast<uint64_t>(int_cast<int32_t>(result));
}

uint64_t exec_op(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    switch (static_cast<RV64IntOP73>((inst.funct7 << 3) | inst.funct3))
    {
    case RV64IntOP73::ADD: return rs1 + rs2;
    case RV64IntOP73::SUB: return rs1 - rs2;
    case RV64IntOP73::SLL: return rs1 << (rs2 & 0x3F);
    case RV64IntOP73::SLT: return (static_cast<int64_t>(rs1) < static_cast<int64_t>(rs2)) ? 1 : 0;
    case RV64IntOP73::SLTU: return (rs1 < rs2) ? 1 : 0;
    case RV64IntOP73::XOR: return rs1 ^ rs2;
    case RV64IntOP73::SRL: return rs1 >> (rs2 & 0x3F);
    case RV64IntOP73::SRA: return static_cast<uint64_t>(static_cast<int64_t>(rs1) >> (rs2 & 0x3F));
    case RV64IntOP73::OR: return rs1 | rs2;
    case RV64IntOP73::AND: return rs1 & rs2;
    case RV64IntOP73::MUL: return static_cast<uint64_t>((static_cast<__uint128_t>(rs1) * static_cast<__uint128_t>(rs2)));
    case RV64IntOP73::MULH: return mulh_impl(rs1, rs2);
    case RV64IntOP73::MULHSU: return mulhsu_impl(rs1, rs2);
    case RV64IntOP73::MULHU: return mulhu_impl(rs1, rs2);
    case RV64IntOP73::DIV: {
        int64_t a = static_cast<int64_t>(rs1);
        int64_t b = static_cast<int64_t>(rs2);
        if (b == 0) return static_cast<uint64_t>(-1);
        if (a == INT64_MIN && b == -1) return static_cast<uint64_t>(INT64_MIN);
        return static_cast<uint64_t>(a / b);
    }
    case RV64IntOP73::DIVU: {
        if (rs2 == 0) return UINT64_MAX;
        return rs1 / rs2;
    }
    case RV64IntOP73::REM: {
        int64_t a = static_cast<int64_t>(rs1);
        int64_t b = static_cast<int64_t>(rs2);
        if (b == 0) return rs1;
        if (a == INT64_MIN && b == -1) return 0;
        return static_cast<uint64_t>(a % b);
    }
    case RV64IntOP73::REMU: {
        if (rs2 == 0) return rs1;
        return rs1 % rs2;
    }
    default: return 0;
    }
}

uint64_t exec_op32(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    const uint32_t a = static_cast<uint32_t>(rs1);
    const uint32_t b = static_cast<uint32_t>(rs2);
    uint32_t result = 0;
    switch (inst.instName)
    {
    case InstName::addw: result = a + b; break;
    case InstName::subw: result = a - b; break;
    case InstName::sllw: result = a << (b & 0x1F); break;
    case InstName::srlw: result = a >> (b & 0x1F); break;
    case InstName::sraw: result = static_cast<uint32_t>(static_cast<int32_t>(a) >> (b & 0x1F)); break;
    case InstName::mulw: result = a * b; break;
    case InstName::divw: {
        const int32_t sa = static_cast<int32_t>(a);
        const int32_t sb = static_cast<int32_t>(b);
        if (sb == 0) {
            result = UINT32_MAX;
        } else if (sa == INT32_MIN && sb == -1) {
            result = static_cast<uint32_t>(INT32_MIN);
        } else {
            result = static_cast<uint32_t>(sa / sb);
        }
        break;
    }
    case InstName::divuw:
        result = (b == 0) ? UINT32_MAX : (a / b);
        break;
    case InstName::remw: {
        const int32_t sa = static_cast<int32_t>(a);
        const int32_t sb = static_cast<int32_t>(b);
        if (sb == 0) {
            result = a;
        } else if (sa == INT32_MIN && sb == -1) {
            result = 0;
        } else {
            result = static_cast<uint32_t>(sa % sb);
        }
        break;
    }
    case InstName::remuw:
        result = (b == 0) ? a : (a % b);
        break;
    default: return 0;
    }
    return int_cast<uint64_t>(int_cast<int32_t>(result));
}

uint64_t exec_csr(const DecodedInst& inst, const uint32_t hibit, const uint32_t lowbit, const uint64_t &rs1, uint64_t &csrVal) {
    uint8_t csrOp = (inst.funct3 & 3);
    uint64_t mask = ((1ULL << (hibit - lowbit + 1)) - 1) << lowbit;
    uint64_t original = (csrVal & mask) >> lowbit;
    uint64_t aligned_rs1 = (rs1 << lowbit) & mask;
    switch (csrOp)
    {
    case 1: // CSRRW
        csrVal = (csrVal & ~mask) | aligned_rs1;
        break;
    case 2: // CSRRS
        csrVal = csrVal | aligned_rs1;
        break;
    case 3: // CSRRC
        csrVal = csrVal & (~aligned_rs1);
        break;
    }
    return original;
}


} // namespace rv64archsem
