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
#include <cstring>

namespace rv64archsem {

MemParseResult parse_load_scalar(
    const DecodedInst& inst,
    const uint64_t rs1val,
    const uint8_t boundary_bytes,
    const bool misalign_error,
    vector<MemLoadDescriptor>& desc
) {
    if (inst.controlType != ControlType::load) {
        return MemParseResult::illegal_instruction;
    }
    uint64_t addr = static_cast<uint64_t>(static_cast<int64_t>(rs1val) + inst.imm);
    uint32_t size = 0;
    switch (inst.funct3 & 0x3) {
        case 0x0: size = 1; break; // LB LBU
        case 0x1: size = 2; break; // LH LHU FLH
        case 0x2: size = 4; break; // LW LWU FLW
        case 0x3: size = 8; break; // LD FLD
    }
    if (misalign_error && (addr % size != 0)) {
        return MemParseResult::misaligned_address;
    }

    if (boundary_bytes == 0) {
        // No boundary restriction
        MemLoadDescriptor ldDesc;
        ldDesc.addr = addr;
        ldDesc.size = static_cast<uint16_t>(size);
        ldDesc.reg_offset = 0;
        ldDesc.first_element_idx = 0;
        desc.push_back(std::move(ldDesc));
        return MemParseResult::success;
    }

    // With boundary restriction
    uint64_t end_addr = addr + size - 1;
    uint64_t curr_addr = addr;
    uint64_t end_addr_this_turn = ((curr_addr / boundary_bytes) + 1) * boundary_bytes - 1;
    while (curr_addr <= end_addr) {
        MemLoadDescriptor ldDesc;
        ldDesc.addr = curr_addr;
        if (end_addr <= end_addr_this_turn) {
            // Last segment
            ldDesc.size = static_cast<uint16_t>(end_addr - curr_addr + 1);
        } else {
            ldDesc.size = static_cast<uint16_t>(end_addr_this_turn - curr_addr + 1);
        }
        ldDesc.reg_offset = static_cast<uint16_t>(curr_addr - addr);
        ldDesc.first_element_idx = 0;
        desc.push_back(std::move(ldDesc));

        curr_addr += ldDesc.size;
        end_addr_this_turn += boundary_bytes;
    }
    return MemParseResult::success;
}

MemParseResult parse_store_scalar(
    const DecodedInst& inst,
    const uint64_t rs1val,
    const uint64_t rs2val,
    const uint8_t boundary_bytes,
    const bool misalign_error,
    vector<MemStoreDescriptor>& desc
) {
    if (inst.controlType != ControlType::store) {
        return MemParseResult::illegal_instruction;
    }
    uint64_t addr = static_cast<uint64_t>(static_cast<int64_t>(rs1val) + inst.imm);
    uint32_t size = 0;
    switch (inst.funct3 & 0x3) {
        case 0x0: size = 1; break; // SB
        case 0x1: size = 2; break; // SH FSH
        case 0x2: size = 4; break; // SW FSW
        case 0x3: size = 8; break; // SD FSD
    }
    if (misalign_error && (addr % size != 0)) {
        return MemParseResult::misaligned_address;
    }
    vector<uint8_t> raw_data_buf;
    raw_data_buf.resize(size);
    memcpy(raw_data_buf.data(), &rs2val, size);

    if (boundary_bytes == 0) {
        // No boundary restriction
        MemStoreDescriptor stDesc;
        stDesc.addr = addr;
        stDesc.raw_data = std::move(raw_data_buf);
        stDesc.first_element_idx = 0;
        desc.push_back(std::move(stDesc));
        return MemParseResult::success;
    }

    // With boundary restriction
    uint64_t end_addr = addr + size - 1;
    uint64_t curr_addr = addr;
    uint64_t end_addr_this_turn = ((curr_addr / boundary_bytes) + 1) * boundary_bytes - 1;
    while (curr_addr <= end_addr) {
        MemStoreDescriptor stDesc;
        stDesc.addr = curr_addr;
        uint64_t segsize = (end_addr <= end_addr_this_turn) ? (end_addr - curr_addr + 1) : (end_addr_this_turn - curr_addr + 1);
        stDesc.raw_data.insert(
            stDesc.raw_data.end(),
            raw_data_buf.begin() + static_cast<int64_t>(curr_addr - addr),
            raw_data_buf.begin() + static_cast<int64_t>(curr_addr - addr + segsize)
        );
        stDesc.first_element_idx = 0;
        desc.push_back(std::move(stDesc));

        curr_addr += segsize;
        end_addr_this_turn += boundary_bytes;
    }
    return MemParseResult::success;
}





} // namespace rv64archsem
