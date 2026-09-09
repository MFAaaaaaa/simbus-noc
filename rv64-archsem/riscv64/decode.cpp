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

#include "deccommon.hpp"
#include "rvi/bitpack.hpp"
#include "rva/bitpack.hpp"
#include "rvf/bitpack.hpp"
#include "rvv/decodev.hpp"

#include <array>

namespace rv64archsem {

inline bool _decode_rvc_op0(const uint32_t &instWord, DecodedInst &inst) {

    /**
     * 15 14 13 | 12 11 10   | 9 8 7 | 6 5       | 4 3 2 | 1 0 |
     * 000      |  nzuimm[5:4|9:6|2|3]           | rd'   | 00  | c.addi4spn
     * 001      | uimm[5:3]  | rs1'  | uimm[7:6] | rd'   | 00  | c.fld
     * 010      | uimm[5:3]  | rs1'  | uimm[2|6] | rd'   | 00  | c.lw
     * 011      | uimm[5:3]  | rs1'  | uimm[7:6] | rd'   | 00  | c.ld
     * 101      | uimm[5:3]  | rs1'  | uimm[7:6] | rd'   | 00  | c.fsd
     * 110      | uimm[5:3]  | rs1'  | uimm[2|6] | rd'   | 00  | c.sw
     * 111      | uimm[5:3]  | rs1'  | uimm[7:6] | rd'   | 00  | c.sd
     */

    uint8_t funct = extract<15, 13>(instWord) & 0x7;

    uint32_t uimmd = (extract<12, 10>(instWord) << 3) | (extract<6, 5>(instWord) << 6);
    uint32_t uimmw = (extract<12, 10>(instWord) << 3) | (extract<5, 5>(instWord) << 6) | (extract<6, 6>(instWord) << 2);
    uint32_t uimm4spn = (extract<12, 11>(instWord) << 4) | (extract<10, 7>(instWord) << 6) | (extract<6, 6>(instWord) << 2) | (extract<5, 5>(instWord) << 3);
    uint8_t rd_ = extract<4, 2>(instWord) & 0x7;
    uint8_t rs1_ = extract<9, 7>(instWord) & 0x7;

    inst.instType = InstType::C;
    inst.rd = rd_ + 8;
    inst.rdType = RegType::gpreg;
    inst.rs1 = rs1_ + 8;
    inst.rs1Type = RegType::gpreg;

    switch (funct)
    {
    case 0: // c.addi4spn
        if (rd_ == 0) return false;
        inst.opcode = OPCode::opimm;
        inst.controlType = ControlType::none;
        inst.instName = InstName::addi;
        inst.imm = uimm4spn;
        inst.funct3 = static_cast<uint8_t>(RV64IntOP73::ADD) & 0x7;
        inst.funct7 = static_cast<uint8_t>(RV64IntOP73::ADD) >> 3;
        inst.rs1 = 2; // x2
        inst.rs1Type = RegType::gpreg;
        inst.rdType = RegType::gpreg;
        break;
    case 1: // c.fld
        inst.opcode = OPCode::loadfp;
        inst.controlType = ControlType::load;
        inst.instName = InstName::fld;
        inst.imm = uimmd;
        inst.funct3 = static_cast<uint8_t>(RV64LSWidth3::DWORD);
        inst.funct7 = 0;
        inst.rs1Type = RegType::gpreg;
        inst.rdType = RegType::freg;
        break;
    case 2: // c.lw
        inst.opcode = OPCode::load;
        inst.controlType = ControlType::load;
        inst.instName = InstName::lw;
        inst.imm = uimmw;
        inst.funct3 = static_cast<uint8_t>(RV64LSWidth3::WORD);
        inst.funct7 = 0;
        inst.rs1Type = RegType::gpreg;
        inst.rdType = RegType::gpreg;
        inst.rdExt = RDExtType::sign32;
        break;
    case 3: // c.ld
        inst.opcode = OPCode::load;
        inst.controlType = ControlType::load;
        inst.instName = InstName::ld;
        inst.imm = uimmd;
        inst.funct3 = static_cast<uint8_t>(RV64LSWidth3::DWORD);
        inst.funct7 = 0;
        inst.rs1Type = RegType::gpreg;
        inst.rdType = RegType::gpreg;
        break;
    case 5: // c.fsd
        inst.opcode = OPCode::storefp;
        inst.controlType = ControlType::store;
        inst.instName = InstName::fsd;
        inst.imm = uimmd;
        inst.funct3 = static_cast<uint8_t>(RV64LSWidth3::DWORD);
        inst.funct7 = 0;
        inst.rs1Type = RegType::gpreg;
        inst.rs2Type = RegType::freg;
        inst.rs2 = rd_ + 8;
        inst.rd = 0; // not used
        break;
    case 6: // c.sw
        inst.opcode = OPCode::store;
        inst.controlType = ControlType::store;
        inst.instName = InstName::sw;
        inst.imm = uimmw;
        inst.funct3 = static_cast<uint8_t>(RV64LSWidth3::WORD);
        inst.funct7 = 0;
        inst.rs1Type = RegType::gpreg;
        inst.rs2Type = RegType::gpreg;
        inst.rs2 = rd_ + 8;
        inst.rd = 0; // not used
        break;
    case 7: // c.sd
        inst.opcode = OPCode::store;
        inst.controlType = ControlType::store;
        inst.instName = InstName::sd;
        inst.imm = uimmd;
        inst.funct3 = static_cast<uint8_t>(RV64LSWidth3::DWORD);
        inst.funct7 = 0;
        inst.rs1Type = RegType::gpreg;
        inst.rs2Type = RegType::gpreg;
        inst.rs2 = rd_ + 8;
        inst.rd = 0; // not used
        break;
    default:
        return false;
    }

    return true;
}

inline bool _decode_rvc_op1(const uint32_t &instWord, DecodedInst &inst) {
    
    uint8_t funct = extract<15, 13>(instWord) & 0x7;

    /**
     * 15 14 13 | 12       | 11 10 | 9 8 7     | 6 5 | 4 3 2       | 1 0 |
     * 000      | nzimm[5] | rs1/rd            | imm[4:0]          | 01  | c.addi
     * 001      | imm[5]   | rs1/rd            | imm[4:0]          | 01  | c.addiw
     * 010      | imm[5]   | rs1/rd            | imm[4:0]          | 01  | c.li
     * 011      | nzimm[9] |     2             | nzimm[4|6|8|7|5]  | 01  | c.addi16sp
     * 011      | nzimm[17]|   rd              | nzimm[16:12]      | 01  | c.lui
     * 
     * 100      | nzuimm[5]| 00    | rs1'/rd'  | nzuimm[4:0]       | 01  | c.srli
     * 100      | nzuimm[5]| 01    | rs1'/rd'  | nzuimm[4:0]       | 01  | c.srai
     * 100      | imm[5]   | 10    | rs1'/rd'  | imm[4:0]          | 01  | c.andi
     * 100      | 0        | 11    | rs1'/rd'  | 00  | rs2'        | 01  | c.sub
     * 100      | 0        | 11    | rs1'/rd'  | 01  | rs2'        | 01  | c.xor
     * 100      | 0        | 11    | rs1'/rd'  | 10  | rs2'        | 01  | c.or
     * 100      | 0        | 11    | rs1'/rd'  | 11  | rs2'        | 01  | c.and
     * 100      | 1        | 11    | rs1'/rd'  | 00  | rs2'        | 01  | c.subw
     * 100      | 1        | 11    | rs1'/rd'  | 01  | rs2'        | 01  | c.addw
     * 
     * 101      |  imm[11|4|9:8|10|6|7|3:1|5]                      | 01  | c.j
     * 110      | imm[8:4:3]       | rs1'      | imm[7:6|2|1|5]    | 01  | c.beqz
     * 111      | imm[8:4:3]       | rs1'      | imm[7:6|2|1|5]    | 01  | c.bnez
     */

    uint32_t uimm5 = (extract<12, 12>(instWord) << 5) | (extract<6, 2>(instWord));
    int64_t imm5 = signExtend<5>(uimm5);
    int64_t immb = signExtend<8>(
        (extract<12, 12>(instWord) << 8) |
        (extract<11, 10>(instWord) << 3) |
        (extract<6, 5>(instWord) << 6) |
        (extract<4, 3>(instWord) << 1) |
        (extract<2, 2>(instWord) << 5)
    );
    uint8_t subfunct = extract<11,10>(instWord) & 0x3;
    // uint8_t optype = ((extract<12, 12>(instWord) << 2) | extract<6,5>(instWord)) & 0x7;

    // uint8_t full_rdrs1_ = extract<11, 7>(instWord) & 0x1F;
    uint8_t rdrs1_ = extract<9, 7>(instWord) & 0x7;
    uint8_t rs2_ = extract<4, 2>(instWord) & 0x7;

    inst.instType = InstType::C;

    switch (funct)
    {
    case 0: // c.addi
    {
        uint8_t rd = extract<11, 7>(instWord) & 0x1F;
        inst.opcode = OPCode::opimm;
        inst.controlType = ControlType::none;
        inst.instName = InstName::addi;
        inst.imm = imm5;
        inst.funct3 = 0;
        inst.funct7 = 0;
        inst.rd = rd;
        inst.rs1 = rd;
        inst.rdType = RegType::gpreg;
        inst.rs1Type = RegType::gpreg;
        break;
    }
    case 1: // c.addiw
    {
        uint8_t rd = extract<11, 7>(instWord) & 0x1F;
        if (rd == 0) return false;
        inst.opcode = OPCode::opimm32;
        inst.controlType = ControlType::none;
        inst.instName = InstName::addiw;
        inst.imm = imm5;
        inst.funct3 = 0;
        inst.funct7 = 0;
        inst.rd = rd;
        inst.rs1 = rd;
        inst.rdType = RegType::gpreg;
        inst.rs1Type = RegType::gpreg;
        inst.rdExt = RDExtType::sign32;
        break;
    }
    case 2: // c.li
    {
        uint8_t rd = extract<11, 7>(instWord) & 0x1F;
        if (rd == 0) return false;
        inst.opcode = OPCode::opimm;
        inst.controlType = ControlType::none;
        inst.instName = InstName::addi;
        inst.imm = imm5;
        inst.funct3 = 0;
        inst.funct7 = 0;
        inst.rd = rd;
        inst.rs1 = 0;
        inst.rdType = RegType::gpreg;
        inst.rs1Type = RegType::gpreg;
        break;
    }
    case 3: // c.addi16sp or c.lui
    {
        uint8_t rd = extract<11, 7>(instWord) & 0x1F;
        if (rd == 2) {
            // c.addi16sp
            int64_t imm16sp = signExtend<9>(
                (extract<12, 12>(instWord) << 9) |
                (extract<4, 3>(instWord) << 7) |
                (extract<5, 5>(instWord) << 6) |
                (extract<2, 2>(instWord) << 5) |
                (extract<6, 6>(instWord) << 4)
            );
            if (imm16sp == 0) return false;
            inst.opcode = OPCode::opimm;
            inst.controlType = ControlType::none;
            inst.instName = InstName::addi;
            inst.imm = imm16sp;
            inst.funct3 = 0;
            inst.funct7 = 0;
            inst.rd = 2;
            inst.rs1 = 2;
            inst.rdType = RegType::gpreg;
            inst.rs1Type = RegType::gpreg;
        } else {
            // c.lui
            int64_t imm17 = signExtend<17>(
                (extract<12, 12>(instWord) << 17) |
                (extract<6, 2>(instWord) << 12)
            );
            if (imm17 == 0) return false;
            inst.opcode = OPCode::lui;
            inst.controlType = ControlType::none;
            inst.instName = InstName::lui;
            inst.imm = imm17;
            inst.funct3 = 0;
            inst.funct7 = 0;
            inst.rd = rd;
            inst.rs1 = 0;
            inst.rdType = RegType::gpreg;
            inst.rs1Type = RegType::gpreg;
        }
        break;
    }
    case 4: // 100
    {
        inst.rd = rdrs1_ + 8;
        inst.rs1 = rdrs1_ + 8;
        inst.rdType = RegType::gpreg;
        inst.rs1Type = RegType::gpreg;
        if (subfunct == 0) {
            // c.srli or c.srai
            uint8_t shamt = extract<6, 2>(instWord) & 0x1F;
            inst.opcode = OPCode::opimm;
            inst.controlType = ControlType::none;
            inst.imm = shamt;
            inst.funct3 = 5;
            if (extract<12, 12>(instWord) == 0) {
                inst.funct7 = 0; // srli
                inst.instName = InstName::srli;
            } else {
                inst.funct7 = 32; // srai
                inst.instName = InstName::srai;
            }
        } else if (subfunct == 1) {
            // c.andi
            inst.opcode = OPCode::opimm;
            inst.controlType = ControlType::none;
            inst.instName = InstName::andi;
            inst.imm = imm5;
            inst.funct3 = 7;
            inst.funct7 = 0;
        } else if (subfunct == 2) {
            // c.sub, c.xor, c.or, c.and, c.subw, c.addw
            uint8_t subsub = extract<6, 5>(instWord) & 0x3;
            inst.rs2 = rs2_ + 8;
            inst.rs2Type = RegType::gpreg;
            inst.controlType = ControlType::none;
            if (extract<12, 12>(instWord) == 0) {
                inst.opcode = OPCode::op;
                switch (subsub) {
                case 0: inst.funct3 = 0; inst.funct7 = 32; inst.instName = InstName::sub; break; // sub
                case 1: inst.funct3 = 4; inst.funct7 = 0; inst.instName = InstName::xor_; break; // xor
                case 2: inst.funct3 = 6; inst.funct7 = 0; inst.instName = InstName::or_; break; // or
                case 3: inst.funct3 = 7; inst.funct7 = 0; inst.instName = InstName::and_; break; // and
                }
            } else {
                inst.opcode = OPCode::op32;
                switch (subsub) {
                case 0: inst.funct3 = 0; inst.funct7 = 32; inst.instName = InstName::subw; break; // subw
                case 1: inst.funct3 = 0; inst.funct7 = 0; inst.instName = InstName::addw; break; // addw
                default: return false;
                }
                inst.rdExt = RDExtType::sign32;
            }
        } else {
            return false;
        }
        break;
    }
    case 5: // c.j
    {
        int64_t immj = signExtend<11>(
            (extract<12, 12>(instWord) << 11) |
            (extract<8, 8>(instWord) << 10) |
            (extract<10, 9>(instWord) << 8) |
            (extract<6, 6>(instWord) << 7) |
            (extract<7, 7>(instWord) << 6) |
            (extract<2, 2>(instWord) << 5) |
            (extract<11, 11>(instWord) << 4) |
            (extract<5, 3>(instWord) << 1)
        );
        inst.opcode = OPCode::jal;
        inst.controlType = ControlType::direct_jump;
        inst.instName = InstName::jal;
        inst.imm = immj;
        inst.funct3 = 0;
        inst.funct7 = 0;
        inst.rd = 0;
        inst.rdType = RegType::gpreg;
        break;
    }
    case 6: // c.beqz
    {
        inst.opcode = OPCode::branch;
        inst.controlType = ControlType::branch;
        inst.instName = InstName::beq;
        inst.imm = immb;
        inst.funct3 = 0; // beq
        inst.funct7 = 0;
        inst.rs1 = rdrs1_ + 8;
        inst.rs1Type = RegType::gpreg;
        inst.rs2 = 0;
        inst.rs2Type = RegType::gpreg;
        break;
    }
    case 7: // c.bnez
    {
        inst.opcode = OPCode::branch;
        inst.controlType = ControlType::branch;
        inst.instName = InstName::bne;
        inst.imm = immb;
        inst.funct3 = 1; // bne
        inst.funct7 = 0;
        inst.rs1 = rdrs1_ + 8;
        inst.rs1Type = RegType::gpreg;
        inst.rs2 = 0;
        inst.rs2Type = RegType::gpreg;
        break;
    }
    default:
        return false;
    }

    return true;
}

inline bool _decode_rvc_op2(const uint32_t &instWord, DecodedInst &inst) {
    
    uint8_t funct = extract<15, 13>(instWord) & 0x7;

    /**
     * 15 14 13 | 12        | 11 10 9 8 7 | 6 5 4 3 2     | 1 0 |
     * 000      | nzuimm[5] | rs1/rd      | nzuimm[4:0]   | 10  | c.slli
     * 001      | uimm[5]   | rs1/rd      | uimm[4:3|8:6] | 10  | c.fldsp
     * 010      | uimm[5]   | rs1/rd      | uimm[4:2|7:6] | 10  | c.lwsp
     * 011      | uimm[5]   | rs1/rd      | uimm[4:3|8:6] | 10  | c.ldsp
     * 
     * 100      | 0         | rs1 =/= 0   | 0             | 10  | c.jr
     * 100      | 0         | rd =/= 0    | rs2 =/= 0     | 10  | c.mv
     * 100      | 1         | 0           | 0             | 10  | c.ebreak
     * 100      | 1         | rs1 =/= 0   | 0             | 10  | c.jalr
     * 100      | 1         | rs1/rd =/= 0| rs2 =/= 0     | 10  | c.add
     * 
     * 101      | uimm[5:3|8:6]           | rs2           | 10  | c.fsdsp
     * 110      | uimm[5:2|7:6]           | rs2           | 10  | c.swsp
     * 111      | uimm[5:3|8:6]           | rs2           | 10  | c.sdsp
     */

    inst.instType = InstType::C;

    switch (funct)
    {
    case 0: // c.slli
    {
        uint8_t rd = extract<11, 7>(instWord) & 0x1F;
        if (rd == 0) return false;
        inst.opcode = OPCode::opimm;
        inst.controlType = ControlType::none;
        inst.instName = InstName::slli;
        inst.imm = (extract<12, 12>(instWord) << 5) | extract<6, 2>(instWord);
        inst.funct3 = 1;
        inst.funct7 = 0;
        inst.rd = rd;
        inst.rs1 = rd;
        inst.rdType = RegType::gpreg;
        inst.rs1Type = RegType::gpreg;
        break;
    }
    case 1: // c.fldsp
    {
        uint8_t rd = extract<11, 7>(instWord) & 0x1F;
        inst.opcode = OPCode::loadfp;
        inst.controlType = ControlType::load;
        inst.instName = InstName::fld;
        inst.imm = (extract<12, 12>(instWord) << 5) | (extract<6, 5>(instWord) << 3) | (extract<4, 2>(instWord) << 6);
        inst.funct3 = 3;
        inst.funct7 = 0;
        inst.rd = rd;
        inst.rs1 = 2;
        inst.rdType = RegType::freg;
        inst.rs1Type = RegType::gpreg;
        break;
    }
    case 2: // c.lwsp
    {
        uint8_t rd = extract<11, 7>(instWord) & 0x1F;
        if (rd == 0) return false;
        inst.opcode = OPCode::load;
        inst.controlType = ControlType::load;
        inst.instName = InstName::lw;
        inst.imm = (extract<12, 12>(instWord) << 5) | (extract<6, 4>(instWord) << 2) | (extract<3, 2>(instWord) << 6);
        inst.funct3 = 2;
        inst.funct7 = 0;
        inst.rd = rd;
        inst.rs1 = 2;
        inst.rdType = RegType::gpreg;
        inst.rs1Type = RegType::gpreg;
        inst.rdExt = RDExtType::sign32;
        break;
    }
    case 3: // c.ldsp
    {
        uint8_t rd = extract<11, 7>(instWord) & 0x1F;
        if (rd == 0) return false;
        inst.opcode = OPCode::load;
        inst.controlType = ControlType::load;
        inst.instName = InstName::ld;
        inst.imm = (extract<12, 12>(instWord) << 5) | (extract<6, 5>(instWord) << 3) | (extract<4, 2>(instWord) << 6);
        inst.funct3 = 3;
        inst.funct7 = 0;
        inst.rd = rd;
        inst.rs1 = 2;
        inst.rdType = RegType::gpreg;
        inst.rs1Type = RegType::gpreg;
        break;
    }
    case 4: // 100
    {
        uint8_t rdrs1 = extract<11, 7>(instWord) & 0x1F;
        uint8_t rs2 = extract<6, 2>(instWord) & 0x1F;
        if (extract<12, 12>(instWord) == 0) {
            if (rs2 == 0) {
                // c.jr
                if (rdrs1 == 0) return false;
                inst.opcode = OPCode::jalr;
                inst.controlType = ControlType::indirect_jump;
                inst.instName = InstName::jalr;
                inst.imm = 0;
                inst.funct3 = 0;
                inst.funct7 = 0;
                inst.rd = 0;
                inst.rs1 = rdrs1;
                inst.rdType = RegType::gpreg;
                inst.rs1Type = RegType::gpreg;
            } else {
                // c.mv
                if (rdrs1 == 0 || rs2 == 0) return false;
                inst.opcode = OPCode::opimm;
                inst.controlType = ControlType::none;
                inst.instName = InstName::addi;
                inst.imm = rs2;
                inst.funct3 = 0;
                inst.funct7 = 0;
                inst.rd = rdrs1;
                inst.rs1 = 0;
                inst.rdType = RegType::gpreg;
                inst.rs1Type = RegType::gpreg;
            }
        } else {
            if (rs2 == 0) {
                if (rdrs1 == 0) {
                    // c.ebreak
                    inst.opcode = OPCode::system;
                    inst.controlType = ControlType::ebreak;
                    inst.instName = InstName::ebreak;
                    inst.imm = 1;
                    inst.funct3 = 0;
                    inst.funct7 = 0;
                    inst.rd = 0;
                    inst.rs1 = 0;
                    inst.rdType = RegType::gpreg;
                    inst.rs1Type = RegType::gpreg;
                } else {
                    // c.jalr
                    inst.opcode = OPCode::jalr;
                    inst.controlType = ControlType::call;
                    inst.instName = InstName::jalr;
                    inst.imm = 0;
                    inst.funct3 = 0;
                    inst.funct7 = 0;
                    inst.rd = 1;
                    inst.rs1 = rdrs1;
                    inst.rdType = RegType::gpreg;
                    inst.rs1Type = RegType::gpreg;
                }
            } else {
                // c.add
                if (rdrs1 == 0 || rs2 == 0) return false;
                inst.opcode = OPCode::op;
                inst.controlType = ControlType::none;
                inst.instName = InstName::add;
                inst.imm = 0;
                inst.funct3 = 0;
                inst.funct7 = 0;
                inst.rd = rdrs1;
                inst.rs1 = rdrs1;
                inst.rs2 = rs2;
                inst.rdType = RegType::gpreg;
                inst.rs1Type = RegType::gpreg;
                inst.rs2Type = RegType::gpreg;
            }
        }
        break;
    }
    case 5: // c.fsdsp
    {
        inst.opcode = OPCode::storefp;
        inst.controlType = ControlType::store;
        inst.instName = InstName::fsd;
        inst.imm = (extract<12, 10>(instWord) << 3) | (extract<9, 7>(instWord) << 6);
        inst.funct3 = 3;
        inst.funct7 = 0;
        inst.rs1 = 2;
        inst.rs2 = extract<6, 2>(instWord) & 0x1F;
        inst.rs1Type = RegType::gpreg;
        inst.rs2Type = RegType::freg;
        inst.rd = 0;
        break;
    }
    case 6: // c.swsp
    {
        inst.opcode = OPCode::store;
        inst.controlType = ControlType::store;
        inst.instName = InstName::sw;
        inst.imm = (extract<12, 9>(instWord) << 2) | (extract<8, 7>(instWord) << 6);
        inst.funct3 = 2;
        inst.funct7 = 0;
        inst.rs1 = 2;
        inst.rs2 = extract<6, 2>(instWord) & 0x1F;
        inst.rs1Type = RegType::gpreg;
        inst.rs2Type = RegType::gpreg;
        inst.rd = 0;
        break;
    }
    case 7: // c.sdsp
    {
        inst.opcode = OPCode::store;
        inst.controlType = ControlType::store;
        inst.instName = InstName::sd;
        inst.imm = (extract<12, 10>(instWord) << 3) | (extract<9, 7>(instWord) << 6);
        inst.funct3 = 3;
        inst.funct7 = 0;
        inst.rs1 = 2;
        inst.rs2 = extract<6, 2>(instWord) & 0x1F;
        inst.rs1Type = RegType::gpreg;
        inst.rs2Type = RegType::gpreg;
        inst.rd = 0;
        break;
    }
    default:
        return false;
    }

    return true;
}

bool decode_rvc(const uint32_t &instWord, DecodedInst &inst) {
    switch (instWord & 3)
    {
    case 0: return _decode_rvc_op0(instWord, inst);
    case 1: return _decode_rvc_op1(instWord, inst);
    case 2: return _decode_rvc_op2(instWord, inst);
    default:
        break;
    }
    return false;
}

bool decode(const uint32_t &instWord, DecodedInst &inst) {

    inst = DecodedInst{}; // reset
    inst.rawInst = instWord;

    if constexpr (SUPPORT_EXTENSION & EXT_C) {
        if ((instWord & 0x3) != 0x3) {
            return decode_rvc(instWord, inst);
        }
    }

    OPCode opcode = static_cast<OPCode>(extract<6, 0>(instWord));
    inst.opcode = opcode;

    switch (opcode)
    {
    case OPCode::load:
        setupI(instWord, inst);
        inst.controlType = ControlType::load;
        if ((inst.instName = rv64_load_inst_name(inst.funct3)) == InstName::illegal) {
            return false;
        }
        switch (inst.instName) {
        case InstName::lb: inst.rdExt = RDExtType::sign8; break;
        case InstName::lh: inst.rdExt = RDExtType::sign16; break;
        case InstName::lw: inst.rdExt = RDExtType::sign32; break;
        default: break;
        }
        break;
    case OPCode::loadfp:
        return decode_loadfp(instWord, inst);
    case OPCode::miscmem:
        setupI(instWord, inst);
        if (inst.funct3 == 0) {
            inst.controlType = ControlType::fence;
            inst.instName = InstName::fence;
        } else if (inst.funct3 == 1) {
            inst.controlType = ControlType::fencei;
            inst.instName = InstName::fence_i;
        } else {
            return false;
        }
        break;
    case OPCode::opimm:
        setupI(instWord, inst);
        if (inst.funct3 == 1) {
            if (extract<31, 26>(instWord) != 0) {
                return false;
            }
            inst.instName = InstName::slli;
        } else if (inst.funct3 == 5) {
            const uint32_t funct6 = extract<31, 26>(instWord);
            if (funct6 == 0) {
                inst.instName = InstName::srli;
            } else if (funct6 == 0x10) {
                inst.instName = InstName::srai;
            } else {
                return false;
            }
        } else if ((inst.instName = rv64_imm64_inst_name(inst.funct3)) == InstName::illegal) {
             return false;
        }
        break;
    case OPCode::auipc:
        setupU(instWord, inst);
        inst.instName = InstName::auipc;
        break;
    case OPCode::opimm32:
        setupI(instWord, inst);
        if (inst.funct3 == 1) {
            if (inst.funct7 != 0) {
                return false;
            }
            inst.instName = InstName::slliw;
        } else if (inst.funct3 == 5) {
            if (inst.funct7 == 0) {
                inst.instName = InstName::srliw;
            } else if (inst.funct7 == 0x20) {
                inst.instName = InstName::sraiw;
            } else {
                return false;
            }
        } else if ((inst.instName = rv64_imm32_inst_name(inst.funct3)) == InstName::illegal) {
             return false;
        }
        inst.rdExt = RDExtType::sign32;
        break;
    case OPCode::store:
        setupS(instWord, inst);
        inst.controlType = ControlType::store;
        if ((inst.instName = rv64_store_inst_name(inst.funct3)) == InstName::illegal) {
            return false;
        }
        break;
    case OPCode::storefp:
        return decode_storefp(instWord, inst);
    case OPCode::amo:
        setupR(instWord, inst);
        inst.controlType = ControlType::amo;
        if ((inst.instName = rv64_amo_inst_name(inst.funct7, inst.funct3)) == InstName::illegal) {
            return false;
        }
        if (inst.funct3 == 2) { // amo.w
            inst.rdExt = RDExtType::sign32;
        }
        break;
    case OPCode::op:
        setupR(instWord, inst);
        if ((inst.instName = rv64_op64_inst_name(inst.funct7, inst.funct3)) == InstName::illegal) {
            return false;
        }
        break;
    case OPCode::lui:
        setupU(instWord, inst);
        inst.instName = InstName::lui;
        break;
    case OPCode::op32:
        setupR(instWord, inst);
        if ((inst.instName = rv64_op32_inst_name(inst.funct7, inst.funct3)) == InstName::illegal) {
            return false;
        }
        inst.rdExt = RDExtType::sign32;
        break;
    case OPCode::madd:
        setupR4(instWord, inst);
        if ((inst.instName = rv64_fmadd_inst_name(inst.funct7)) == InstName::illegal) {
            return false;
        }
        break;
    case OPCode::msub:
        setupR4(instWord, inst);
        if ((inst.instName = rv64_fmsub_inst_name(inst.funct7)) == InstName::illegal) {
            return false;
        }
        break;
    case OPCode::nmsub:
        setupR4(instWord, inst);
        if ((inst.instName = rv64_fnmsub_inst_name(inst.funct7)) == InstName::illegal) {
            return false;
        }
        break;
    case OPCode::nmadd:
        setupR4(instWord, inst);
        if ((inst.instName = rv64_fnmadd_inst_name(inst.funct7)) == InstName::illegal) {
            return false;
        }
        break;
    case OPCode::opfp:
    {
        setupR(instWord, inst);
        if ((inst.instName = rv64_fpop_inst_name(inst.funct7, inst.funct3, inst.rs2)) == InstName::illegal) {
            return false;
        }
        RV64FPOP5 fpop = static_cast<RV64FPOP5>(extract<31, 27>(instWord));
        if (!rv64_fpop_is_i_rd(fpop)) {
            inst.rdType = RegType::freg;
        }
        if (!rv64_fpop_is_i_s1(fpop)) {
            inst.rs1Type = RegType::freg;
        }
        if (!rv64_fpop_has_s2(fpop)) {
            inst.rs2Type = RegType::none;
        } else {
            inst.rs2Type = RegType::freg;
        }
        if (!rv64_fpop_is_valid(inst.funct7, inst.funct3, inst.rs2)) {
            return false;
        }
        break;
    }
    case OPCode::opv:
        return decode_opv(instWord, inst);
    case OPCode::branch:
        setupB(instWord, inst);
        inst.controlType = ControlType::branch;
        if ((inst.instName = rv64_branch_inst_name(inst.funct3)) == InstName::illegal) {
            return false;
        }
        break;
    case OPCode::jalr:
        setupI(instWord, inst);
        if (inst.rd == 1 && inst.rs1 != 1) {
            inst.controlType = ControlType::call;
        } else if (inst.rd == 0 && inst.rs1 == 1) {
            inst.controlType = ControlType::ret;
        } else {
            inst.controlType = ControlType::indirect_jump;
        }
        inst.instName = InstName::jalr;
        break;
    case OPCode::jal:
        setupJ(instWord, inst);
        if (inst.rd == 1 || inst.rd == 5) {
            inst.controlType = ControlType::call;
        } else {
            inst.controlType = ControlType::direct_jump;
        }
        inst.instName = InstName::jal;
        break;
    case OPCode::system:
        inst.funct3 = extract<14, 12>(instWord) & 0x7;
        inst.rd = extract<11, 7>(instWord) & 0x1F;
        inst.rs1 = extract<19, 15>(instWord) & 0x1F;
        inst.imm = extract<31, 20>(instWord) & 0xFFF;
        inst.funct7 = extract<31, 25>(instWord) & 0x7F;
        {
            uint32_t csr = inst.imm & 0xFFF;
            if (inst.funct7 == 0b0001001) { // sfence.vma
                inst.controlType = ControlType::sfence_vma;
                inst.instName = InstName::sfence_vma;
            } else if (inst.funct3 == 0) {
                if (csr == 0) {
                    inst.controlType = ControlType::ecall;
                    inst.instName = InstName::ecall;
                } else if (csr == 1) {
                    inst.controlType = ControlType::ebreak;
                    inst.instName = InstName::ebreak;
                } else if (csr == 0x102) {
                    inst.controlType = ControlType::sret;
                    inst.instName = InstName::sret;
                } else if (csr == 0x302) {
                    inst.controlType = ControlType::mret;
                    inst.instName = InstName::mret;
                } else if (csr == 0x7b2) {
                    inst.controlType = ControlType::uret;
                    inst.instName = InstName::uret;
                } else if (csr == 0x105) {
                    inst.controlType = ControlType::wfi;
                    inst.instName = InstName::wfi;
                } else {
                    return false;
                }
            } else {
                inst.controlType = ControlType::csr;
                inst.rdType = RegType::gpreg;
                switch (inst.funct3)
                {
                case 0b001:
                    inst.instName = InstName::csrrw;
                    inst.rs1Type = RegType::gpreg;
                    break;
                case 0b010:
                    inst.instName = InstName::csrrs;
                    inst.rs1Type = RegType::gpreg;
                    break;
                case 0b011:
                    inst.instName = InstName::csrrc;
                    inst.rs1Type = RegType::gpreg;
                    break;
                case 0b101:
                    inst.instName = InstName::csrrwi;
                    inst.rs1Type = RegType::none;
                    break;
                case 0b110:
                    inst.instName = InstName::csrrsi;
                    inst.rs1Type = RegType::none;
                    break;
                case 0b111:
                    inst.instName = InstName::csrrci;
                    inst.rs1Type = RegType::none;
                    break;
                default:
                    return false;
                }
            }
        }
        break;
    default:
        return false;
    }

    return true;
}




} // namespace rv64archsem
