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

enum class RV64AMOOP54 {
    ADD_W     = 0x002,
    SWAP_W    = 0x012,
    LR_W      = 0x022,
    SC_W      = 0x032,
    XOR_W     = 0x042,
    AND_W     = 0x0c2,
    OR_W      = 0x082,
    MIN_W     = 0x102,
    MAX_W     = 0x142,
    MINU_W    = 0x182,
    MAXU_W    = 0x1c2,
    ADD_D     = 0x003,
    SWAP_D    = 0x013,
    LR_D      = 0x023,
    SC_D      = 0x033,
    XOR_D     = 0x043,
    AND_D     = 0x0c3,
    OR_D      = 0x083,
    MIN_D     = 0x103,
    MAX_D     = 0x143,
    MINU_D    = 0x183,
    MAXU_D    = 0x1c3
};

inline constexpr InstName rv64_amo_inst_name(uint32_t funct7, uint32_t funct3) {
    switch (static_cast<RV64AMOOP54>(((funct7 >> 2) << 4) | (funct3 & 0x7)))
    {
    case RV64AMOOP54::ADD_W: return InstName::amoadd_w;
    case RV64AMOOP54::SWAP_W: return InstName::amoswap_w;
    case RV64AMOOP54::LR_W: return InstName::lr_w;
    case RV64AMOOP54::SC_W: return InstName::sc_w;
    case RV64AMOOP54::XOR_W: return InstName::amoxor_w;
    case RV64AMOOP54::AND_W: return InstName::amoand_w;
    case RV64AMOOP54::OR_W: return InstName::amoor_w;
    case RV64AMOOP54::MIN_W: return InstName::amomin_w;
    case RV64AMOOP54::MAX_W: return InstName::amomax_w;
    case RV64AMOOP54::MINU_W: return InstName::amominu_w;
    case RV64AMOOP54::MAXU_W: return InstName::amomaxu_w;
    case RV64AMOOP54::ADD_D: return InstName::amoadd_d;
    case RV64AMOOP54::SWAP_D: return InstName::amoswap_d;
    case RV64AMOOP54::LR_D: return InstName::lr_d;
    case RV64AMOOP54::SC_D: return InstName::sc_d;
    case RV64AMOOP54::XOR_D: return InstName::amoxor_d;
    case RV64AMOOP54::AND_D: return InstName::amoand_d;
    case RV64AMOOP54::OR_D: return InstName::amoor_d;
    case RV64AMOOP54::MIN_D: return InstName::amomin_d;
    case RV64AMOOP54::MAX_D: return InstName::amomax_d;
    case RV64AMOOP54::MINU_D: return InstName::amominu_d;
    case RV64AMOOP54::MAXU_D: return InstName::amomaxu_d;
    default: return InstName::illegal;
    }
}

} // namespace rv64archsem
