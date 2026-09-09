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

enum class RV64LSWidth3 : uint32_t {
    BYTE    = 0,
    HARF    = 1,
    WORD    = 2,
    DWORD   = 3,
    UBYTE   = 4,
    UHARF   = 5,
    UWORD   = 6,
    QWORD   = 8,
};

inline constexpr InstName rv64_load_inst_name(uint32_t funct3) {
    switch (static_cast<RV64LSWidth3>(funct3))
    {
    case RV64LSWidth3::BYTE: return InstName::lb;
    case RV64LSWidth3::HARF: return InstName::lh;
    case RV64LSWidth3::WORD: return InstName::lw;
    case RV64LSWidth3::DWORD: return InstName::ld;
    case RV64LSWidth3::UBYTE: return InstName::lbu;
    case RV64LSWidth3::UHARF: return InstName::lhu;
    case RV64LSWidth3::UWORD: return InstName::lwu;
    default: return InstName::illegal;
    }
}

inline constexpr InstName rv64_store_inst_name(uint32_t funct3) {
    switch (static_cast<RV64LSWidth3>(funct3))
    {
    case RV64LSWidth3::BYTE: return InstName::sb;
    case RV64LSWidth3::HARF: return InstName::sh;
    case RV64LSWidth3::WORD: return InstName::sw;
    case RV64LSWidth3::DWORD: return InstName::sd;
    default: return InstName::illegal;
    }
}

enum class RV64BranchOP3 : uint32_t {
    BEQ     = 0,
    BNE     = 1,
    BLT     = 4,
    BGE     = 5,
    BLTU    = 6,
    BGEU    = 7
};

inline constexpr InstName rv64_branch_inst_name(uint32_t op) {
    switch (static_cast<RV64BranchOP3>(op))
    {
    case RV64BranchOP3::BEQ: return InstName::beq;
    case RV64BranchOP3::BNE: return InstName::bne;
    case RV64BranchOP3::BLT: return InstName::blt;
    case RV64BranchOP3::BGE: return InstName::bge;
    case RV64BranchOP3::BLTU: return InstName::bltu;
    case RV64BranchOP3::BGEU: return InstName::bgeu;
    default: return InstName::illegal;
    }
}

enum class RV64IntOP73 : uint32_t {
    ADD     = 0x000,
    SUB     = 0x100,
    SLL     = 0x001,
    SLT     = 0x002,
    SLTU    = 0x003,
    XOR     = 0x004,
    SRL     = 0x005,
    SRA     = 0x105,
    OR      = 0x006,
    AND     = 0x007,
    MUL     = 0x008,
    MULH    = 0x009,
    MULHSU  = 0x00a,
    MULHU   = 0x00b,
    DIV     = 0x00c,
    DIVU    = 0x00d,
    REM     = 0x00e,
    REMU    = 0x00f,
};

inline constexpr InstName rv64_op64_inst_name(uint32_t funct7, uint32_t funct3) {
    switch (static_cast<RV64IntOP73>((funct7 << 3) | funct3))
    {
    case RV64IntOP73::ADD: return InstName::add;
    case RV64IntOP73::SUB: return InstName::sub;
    case RV64IntOP73::SLL: return InstName::sll;
    case RV64IntOP73::SLT: return InstName::slt;
    case RV64IntOP73::SLTU: return InstName::sltu;
    case RV64IntOP73::XOR: return InstName::xor_;
    case RV64IntOP73::SRL: return InstName::srl;
    case RV64IntOP73::SRA: return InstName::sra;
    case RV64IntOP73::OR: return InstName::or_;
    case RV64IntOP73::AND: return InstName::and_;
    case RV64IntOP73::MUL: return InstName::mul;
    case RV64IntOP73::MULH: return InstName::mulh;
    case RV64IntOP73::MULHSU: return InstName::mulhsu;
    case RV64IntOP73::MULHU: return InstName::mulhu;
    case RV64IntOP73::DIV: return InstName::div;
    case RV64IntOP73::DIVU: return InstName::divu;
    case RV64IntOP73::REM: return InstName::rem;
    case RV64IntOP73::REMU: return InstName::remu;
    default: return InstName::illegal;
    }
}

inline constexpr InstName rv64_imm64_inst_name(uint32_t funct3) {
    switch (static_cast<RV64IntOP73>(funct3))
    {
    case RV64IntOP73::ADD: return InstName::addi;
    case RV64IntOP73::SLL: return InstName::slli;
    case RV64IntOP73::SLT: return InstName::slti;
    case RV64IntOP73::SLTU: return InstName::sltiu;
    case RV64IntOP73::XOR: return InstName::xori;
    case RV64IntOP73::SRL: return InstName::srli;
    case RV64IntOP73::SRA: return InstName::srai;
    case RV64IntOP73::OR: return InstName::ori;
    case RV64IntOP73::AND: return InstName::andi;
    default: return InstName::illegal;
    }
}

inline constexpr InstName rv64_op32_inst_name(uint32_t funct7, uint32_t funct3) {
    switch (static_cast<RV64IntOP73>((funct7 << 3) | funct3))
    {
    case RV64IntOP73::ADD: return InstName::addw;
    case RV64IntOP73::SUB: return InstName::subw;
    case RV64IntOP73::SLL: return InstName::sllw;
    case RV64IntOP73::SRL: return InstName::srlw;
    case RV64IntOP73::SRA: return InstName::sraw;
    case RV64IntOP73::MUL: return InstName::mulw;
    case RV64IntOP73::DIV: return InstName::divw;
    case RV64IntOP73::DIVU: return InstName::divuw;
    case RV64IntOP73::REM: return InstName::remw;
    case RV64IntOP73::REMU: return InstName::remuw;
    default: return InstName::illegal;
    }
}

inline constexpr InstName rv64_imm32_inst_name(uint32_t funct3) {
    switch (static_cast<RV64IntOP73>(funct3))
    {
    case RV64IntOP73::ADD: return InstName::addiw;
    case RV64IntOP73::SLL: return InstName::slliw;
    case RV64IntOP73::SRL: return InstName::srliw;
    case RV64IntOP73::SRA: return InstName::sraiw;
    default: return InstName::illegal;
    }
}

} // namespace rv64archsem
