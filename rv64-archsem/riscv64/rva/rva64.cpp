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

#include "rva/bitpack.hpp"

#include <bit>

using std::bit_cast;

namespace rv64archsem {


uint64_t exec_amo(const DecodedInst& inst, const uint64_t &rs1, uint64_t &mem) {
    uint64_t original = mem;
    int64_t signed_rs1 = bit_cast<int64_t>(rs1);
    int64_t signed_rs2 = bit_cast<int64_t>(mem);
    uint64_t unsigned_rs1 = rs1;
    uint64_t unsigned_rs2 = mem;
    int64_t result = 0;
    if ((inst.funct7 & 3) == 2) {
        // 32bit
        signed_rs1 = static_cast<int32_t>(static_cast<uint32_t>(rs1));
        signed_rs2 = static_cast<int32_t>(static_cast<uint32_t>(mem));
        unsigned_rs1 = static_cast<uint32_t>(rs1);
        unsigned_rs2 = static_cast<uint32_t>(mem);
    }
    switch (inst.funct7 >> 2)
    {
    case 0x00: result = signed_rs1 + signed_rs2; break;
    case 0x01: result = signed_rs1; break;
    case 0x02: result = signed_rs2; break;
    case 0x03: break;
    case 0x04: result = signed_rs1 ^ signed_rs2; break;
    case 0x0c: result = signed_rs1 & signed_rs2; break;
    case 0x08: result = signed_rs1 | signed_rs2; break;
    case 0x10: result = (signed_rs1 < signed_rs2) ? signed_rs1 : signed_rs2; break;
    case 0x14: result = (signed_rs1 > signed_rs2) ? signed_rs1 : signed_rs2; break;
    case 0x18: result = static_cast<int64_t>((unsigned_rs1 < unsigned_rs2) ? unsigned_rs1 : unsigned_rs2); break;
    case 0x1c: result = static_cast<int64_t>((unsigned_rs1 > unsigned_rs2) ? unsigned_rs1 : unsigned_rs2); break;
    }
    if ((inst.funct7 & 3) != 2) {
        mem = bit_cast<uint64_t>(result);
    } else {
        mem = static_cast<uint64_t>(static_cast<uint32_t>(result));
    }
    return original;
}

} // namespace rv64archsem
