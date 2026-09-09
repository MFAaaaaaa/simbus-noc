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

#include "riscv64.hpp"

#include <cstdint>

namespace rv64archsem {

template<uint32_t hibit, uint32_t lowbit>
inline constexpr uint32_t extract(const uint32_t &value) {
    return (value >> lowbit) & ((1u << (hibit - lowbit + 1)) - 1);
}

template <uint32_t hibit>
inline int64_t signExtend(const uint64_t &value) {
    uint64_t res = (value & (1ULL << hibit)) ? (value | (~0ULL << (hibit + 1))) : value;
    return static_cast<int64_t>(res);
}

inline void setupR(const uint32_t &instWord, DecodedInst &inst) {
    inst.instType = InstType::R;
    inst.rd = extract<11, 7>(instWord) & 0x1F;
    inst.rs1 = extract<19, 15>(instWord) & 0x1F;
    inst.rs2 = extract<24, 20>(instWord) & 0x1F;
    inst.funct3 = extract<14, 12>(instWord) & 0x07;
    inst.funct7 = extract<31, 25>(instWord) & 0x7F;
    inst.rdType = RegType::gpreg;
    inst.rs1Type = RegType::gpreg;
    inst.rs2Type = RegType::gpreg;
}

inline void setupI(const uint32_t &instWord, DecodedInst &inst) {
    inst.instType = InstType::I;
    inst.rd = extract<11, 7>(instWord) & 0x1F;
    inst.rs1 = extract<19, 15>(instWord) & 0x1F;
    inst.imm = signExtend<11>(extract<31, 20>(instWord));
    inst.funct3 = extract<14, 12>(instWord) & 0x07;
    inst.funct7 = extract<31, 25>(instWord) & 0x7F;
    inst.rdType = RegType::gpreg;
    inst.rs1Type = RegType::gpreg;
}

inline void setupS(const uint32_t &instWord, DecodedInst &inst) {
    inst.instType = InstType::S;
    inst.rs1 = extract<19, 15>(instWord) & 0x1F;
    inst.rs2 = extract<24, 20>(instWord) & 0x1F;
    inst.imm = signExtend<11>(
        (extract<31, 25>(instWord) << 5) |
        (extract<11, 7>(instWord))
    );
    inst.funct3 = extract<14, 12>(instWord) & 0x07;
    inst.funct7 = extract<31, 25>(instWord) & 0x7F;
    inst.rs1Type = RegType::gpreg;
    inst.rs2Type = RegType::gpreg;
}

inline void setupB(const uint32_t &instWord, DecodedInst &inst) {
    inst.instType = InstType::B;
    inst.rs1 = extract<19, 15>(instWord) & 0x1F;
    inst.rs2 = extract<24, 20>(instWord) & 0x1F;
    inst.imm = signExtend<12>(
        (extract<31, 31>(instWord) << 12) |
        (extract<7, 7>(instWord) << 11) |
        (extract<30, 25>(instWord) << 5) |
        (extract<11, 8>(instWord) << 1)
    );
    inst.funct3 = extract<14, 12>(instWord) & 0x07;
    inst.funct7 = extract<31, 25>(instWord) & 0x7F;
    inst.rs1Type = RegType::gpreg;
    inst.rs2Type = RegType::gpreg;
}

inline void setupU(const uint32_t &instWord, DecodedInst &inst) {
    inst.instType = InstType::U;
    inst.rd = extract<11, 7>(instWord) & 0x1F;
    inst.imm = signExtend<31>(extract<31, 12>(instWord) << 12);
    inst.funct3 = 0;
    inst.funct7 = 0;
    inst.rdType = RegType::gpreg;
}

inline void setupJ(const uint32_t &instWord, DecodedInst &inst) {
    inst.instType = InstType::J;
    inst.rd = extract<11, 7>(instWord) & 0x1F;
    inst.imm = signExtend<20>(
        (extract<31, 31>(instWord) << 20) |
        (extract<19, 12>(instWord) << 12) |
        (extract<20, 20>(instWord) << 11) |
        (extract<30, 21>(instWord) << 1)
    );
    inst.funct3 = 0;
    inst.funct7 = 0;
    inst.rdType = RegType::gpreg;
}

inline void setupR4(const uint32_t &instWord, DecodedInst &inst) {
    inst.instType = InstType::R4;
    inst.rd = extract<11, 7>(instWord) & 0x1F;
    inst.rs1 = extract<19, 15>(instWord) & 0x1F;
    inst.rs2 = extract<24, 20>(instWord) & 0x1F;
    inst.rs3 = extract<31, 27>(instWord) & 0x1F;
    inst.funct3 = extract<14, 12>(instWord) & 0x07;
    inst.funct7 = extract<26, 25>(instWord) & 0x03;
    inst.rdType = RegType::freg;
    inst.rs1Type = RegType::freg;
    inst.rs2Type = RegType::freg;
    inst.rs3Type = RegType::freg;
}

} // namespace rv64archsem
