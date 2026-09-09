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

#include "rvv/decodev.hpp"
#include "deccommon.hpp"
#include "intcast.hpp"

namespace rv64archsem {


bool _decode_loadv(const uint32_t &instWord, DecodedInst &inst) {
    inst.opcode = OPCode::loadfp;
    inst.instType = InstType::V;
    inst.controlType = ControlType::vload;
    inst.rd = extract<11, 7>(instWord) & 0x1f;
    inst.rs1 = extract<19, 15>(instWord) & 0x1f;
    inst.rs2 = extract<24, 20>(instWord) & 0x1f;
    inst.funct3 = extract<14, 12>(instWord) & 0x7;
    inst.funct7 = extract<31, 25>(instWord) & 0x7f;
    inst.rs1Type = RegType::gpreg;
    inst.rdType = RegType::vreg;
    switch (extract<14, 12>(instWord)) // funct3
    {
    case 0x0: inst.vrdView = VRegView::ei8; break;
    case 0x5: inst.vrdView = VRegView::ei16; break;
    case 0x6: inst.vrdView = VRegView::ei32; break;
    case 0x7: inst.vrdView = VRegView::ei64; break;
    }

    if (instWord & (1U << 28)) {
        return false; // reserved MEW bit
    }

    switch (extract<27, 26>(instWord)) // mop
    {
    case 0b00: // unit-stride
    {
        switch (extract<24, 20>(instWord) & 0x1f) // rs2
        {
            case 0x00: // vle
                inst.instName = InstName::vleX_v; break;
            case 0x08: // whole register
                if ((instWord & (1U << 25)) == 0) return false; // no mask
                switch (extract<31, 29>(instWord))
                {
                case 0: inst.vrdView = VRegView::reg1; break;
                case 1: inst.vrdView = VRegView::reg2; break;
                case 3: inst.vrdView = VRegView::reg4; break;
                case 7: inst.vrdView = VRegView::reg8; break;
                default: return false; // reserved
                }
                inst.instName = InstName::vlNreX_v; break;
            case 0x0b: // load byte vector
                if ((instWord & (1U << 25)) == 0) return false; // no mask
                inst.vrdView = VRegView::mask;
                inst.instName = InstName::vlm_v; break;
            case 0x10: // fault-only-first
                inst.instName = InstName::vleXff_v; break;
            default:
                return false;
        }
        break;
    }
    case 0b01: // indexed-unordered
    {
        inst.rs2Type = RegType::vreg;
        inst.instName = InstName::vluxeiX_v;
        inst.vrs2View = inst.vrdView; // vs2 has explict defined layout
        inst.vrdView = VRegView::none; // vd use default layout
        break;
    }
    case 0b10: // strided
    {
        inst.rs2Type = RegType::gpreg;
        inst.instName = InstName::vlseX_v;
        break;
    }
    case 0b11: // indexed-ordered
    {
        inst.rs2Type = RegType::vreg;
        inst.instName = InstName::vloxeiX_v;
        inst.vrs2View = inst.vrdView;
        inst.vrdView = VRegView::none;
        break;
    }
    }
    return true;
}

bool decode_loadfp(const uint32_t &instWord, DecodedInst &inst) {
    uint8_t funct3 = extract<14, 12>(instWord) & 0x7;

    switch (funct3)
    {
    case 0b000: return _decode_loadv(instWord, inst); // vlb
    case 0b001: break; // flh
    case 0b010: break; // flw
    case 0b011: break; // fld
    case 0b100: break; // flq
    case 0b101: // vlh
    case 0b110: // vlw
    case 0b111: // vld
        return _decode_loadv(instWord, inst);
    }

    // load floating-point instruction
    setupI(instWord, inst);
    inst.opcode = OPCode::loadfp;
    inst.rdType = RegType::freg;
    inst.rs1Type = RegType::gpreg;
    switch (funct3)
    {
    case 0b001:
        inst.instName = InstName::flh;
        break;
    case 0b010:
        inst.instName = InstName::flw;
        break;
    case 0b011:
        inst.instName = InstName::fld;
        break;
    case 0b100:
        inst.instName = InstName::flq;
        break;
    default:
        return false;
    }
    inst.controlType = ControlType::load;
    return true;
}

bool _decode_storev(const uint32_t &instWord, DecodedInst &inst) {
    inst.opcode = OPCode::storefp;
    inst.instType = InstType::V;
    inst.controlType = ControlType::vstore;
    inst.rs1 = extract<19, 15>(instWord) & 0x1f;
    inst.rs2 = extract<24, 20>(instWord) & 0x1f;
    inst.rs3 = extract<11, 7>(instWord) & 0x1f;
    inst.funct3 = extract<14, 12>(instWord) & 0x7;
    inst.funct7 = extract<31, 25>(instWord) & 0x7f;
    inst.rs1Type = RegType::gpreg;
    inst.rs3Type = RegType::vreg;
    switch (extract<14, 12>(instWord) & 0x7) // funct3
    {
    case 0x0: inst.vrs3View = VRegView::ei8; break;
    case 0x5: inst.vrs3View = VRegView::ei16; break;
    case 0x6: inst.vrs3View = VRegView::ei32; break;
    case 0x7: inst.vrs3View = VRegView::ei64; break;
    }

    if (instWord & (1U << 28)) {
        return false; // reserved MEW bit
    }

    switch (extract<27, 26>(instWord) & 0x3) // mop
    {
    case 0b00: // unit-stride
    {
        switch (extract<24, 20>(instWord) & 0x1f) // rs2
        {
            case 0x00: // vse
                inst.instName = InstName::vseX_v; break;
            case 0x08: // whole register
                if ((instWord & (1U << 25)) == 0) return false; // no mask
                switch (extract<31, 29>(instWord))
                {
                case 0: inst.vrs3View = VRegView::reg1; break;
                case 1: inst.vrs3View = VRegView::reg2; break;
                case 3: inst.vrs3View = VRegView::reg4; break;
                case 7: inst.vrs3View = VRegView::reg8; break;
                default: return false; // reserved
                }
                inst.instName = InstName::vsNr_v; break;
            case 0x0b: // store byte vector
                if ((instWord & (1U << 25)) == 0) return false; // no mask
                inst.vrs3View = VRegView::mask;
                inst.instName = InstName::vsm_v; break;
            default:
                return false;
        }
        break;
    }
    case 0b01: // indexed-unordered
    {
        inst.instName = InstName::vsuxeiX_v;
        inst.rs2Type = RegType::vreg;
        inst.vrs2View = inst.vrs3View;
        inst.vrs3View = VRegView::none;
        break;
    }
    case 0b10: // strided
    {
        inst.instName = InstName::vsseX_v;
        inst.rs2Type = RegType::gpreg;
        break;
    }
    case 0b11: // indexed-ordered
    {
        inst.instName = InstName::vsoxeiX_v;
        inst.rs2Type = RegType::vreg;
        inst.vrs2View = inst.vrs3View;
        inst.vrs3View = VRegView::none;
        break;
    }
    }
    return true;
}

bool decode_storefp(const uint32_t &instWord, DecodedInst &inst) {
    uint8_t funct3 = extract<14, 12>(instWord) & 0x7;
    
    switch (funct3)
    {
    case 0b000: return _decode_storev(instWord, inst); // vsb
    case 0b001: break; // fsh
    case 0b010: break; // fsw
    case 0b011: break; // fsd
    case 0b100: break; // fsq
    case 0b101: // vsh
    case 0b110: // vsw
    case 0b111: // vsd
        return _decode_storev(instWord, inst);
    }

    // store floating-point instruction
    inst.opcode = OPCode::storefp;
    setupS(instWord, inst);
    inst.rs2Type = RegType::freg;
    switch (funct3)
    {
    case 0b001:
        inst.instName = InstName::fsh;
        break;
    case 0b010:
        inst.instName = InstName::fsw;
        break;
    case 0b011:
        inst.instName = InstName::fsd;
        break;
    case 0b100:
        inst.instName = InstName::fsq;
        break;
    default:
        return false;
    }
    inst.controlType = ControlType::store;
    return true;
}

inline void _setupOPV_no_regtype(const uint32_t &instWord, DecodedInst &inst) {
    inst.instType = InstType::V;
    inst.rd = extract<11, 7>(instWord) & 0x1f;
    inst.rs1 = extract<19, 15>(instWord) & 0x1f;
    inst.rs2 = extract<24, 20>(instWord) & 0x1f;
    inst.funct3 = extract<14, 12>(instWord) & 0x7;
    inst.funct7 = extract<31, 25>(instWord) & 0x7f;
}


bool _decodev_opivv(const uint32_t &instWord, DecodedInst &inst) {

    uint8_t funct6 = extract<31, 26>(instWord) & 0x3f;
    bool vmflag = (instWord & (1U << 25)) != 0;

    _setupOPV_no_regtype(instWord, inst);
    inst.rdType = RegType::vreg;
    inst.rs1Type = RegType::vreg;
    inst.rs2Type = ((funct6 == 0x17 && vmflag) ? (RegType::none) : (RegType::vreg)); // vmv.v.v

    switch (funct6)
    {
    case 0x00: inst.instName = InstName::vadd_vv; break;
    case 0x02: inst.instName = InstName::vsub_vv; break;
    case 0x04: inst.instName = InstName::vminu_vv; break;
    case 0x05: inst.instName = InstName::vmin_vv; break;
    case 0x06: inst.instName = InstName::vmaxu_vv; break;
    case 0x07: inst.instName = InstName::vmax_vv; break;
    case 0x09: inst.instName = InstName::vand_vv; break;
    case 0x0a: inst.instName = InstName::vor_vv; break;
    case 0x0b: inst.instName = InstName::vxor_vv; break;
    case 0x0c: inst.instName = InstName::vrgather_vv; break;
    case 0x0e: inst.instName = InstName::vrgatherei16_vv; inst.vrs1View = VRegView::ei16; break;
    case 0x10: inst.instName = InstName::vadc_vvm; if (vmflag) return false; break;
    case 0x11: inst.instName = (vmflag ? (InstName::vmadc_vv) : (InstName::vmadc_vvm)); inst.vrdView = VRegView::mask; break;
    case 0x12: inst.instName = InstName::vsbc_vvm; if (vmflag) return false; break;
    case 0x13: inst.instName = (vmflag ? (InstName::vmsbc_vv) : (InstName::vmsbc_vvm)); inst.vrdView = VRegView::mask; break;
    case 0x17: inst.instName = (vmflag ? (InstName::vmv_v_v) : (InstName::vmerge_vvm)); break;
    case 0x18: inst.instName = InstName::vmseq_vv; inst.vrdView = VRegView::mask; break;
    case 0x19: inst.instName = InstName::vmsne_vv; inst.vrdView = VRegView::mask; break;
    case 0x1a: inst.instName = InstName::vmsltu_vv; inst.vrdView = VRegView::mask; break;
    case 0x1b: inst.instName = InstName::vmslt_vv; inst.vrdView = VRegView::mask; break;
    case 0x1c: inst.instName = InstName::vmsleu_vv; inst.vrdView = VRegView::mask; break;
    case 0x1d: inst.instName = InstName::vmsle_vv; inst.vrdView = VRegView::mask; break;
    case 0x20: inst.instName = InstName::vsaddu_vv; break;
    case 0x21: inst.instName = InstName::vsadd_vv; break;
    case 0x22: inst.instName = InstName::vssubu_vv; break;
    case 0x23: inst.instName = InstName::vssub_vv; break;
    case 0x25: inst.instName = InstName::vsll_vv; break;
    case 0x27: inst.instName = InstName::vsmul_vv; break;
    case 0x28: inst.instName = InstName::vsrl_vv; break;
    case 0x29: inst.instName = InstName::vsra_vv; break;
    case 0x2a: inst.instName = InstName::vssrl_vv; break;
    case 0x2b: inst.instName = InstName::vssra_vv; break;
    case 0x2c: inst.instName = InstName::vnsrl_wv; inst.vrs2View = VRegView::m2; break;
    case 0x2d: inst.instName = InstName::vnsra_wv; inst.vrs2View = VRegView::m2; break;
    case 0x2e: inst.instName = InstName::vnclipu_wv; inst.vrs2View = VRegView::m2; break;
    case 0x2f: inst.instName = InstName::vnclip_wv; inst.vrs2View = VRegView::m2; break;
    case 0x30: inst.instName = InstName::vwredsumu_vs; inst.vrs1View = inst.vrdView = VRegView::m2; break;
    case 0x31: inst.instName = InstName::vwredsum_vs; inst.vrs1View = inst.vrdView = VRegView::m2; break;
    default: return false;
    }

    return true;
}

bool _decodev_opfvv(const uint32_t &instWord, DecodedInst &inst) {
    uint8_t funct6 = extract<31, 26>(instWord) & 0x3f;
    bool vmflag = (instWord & (1U << 25)) != 0;

    _setupOPV_no_regtype(instWord, inst);
    inst.rdType = ((funct6 == 0x10) ? (RegType::freg) : (RegType::vreg));
    inst.rs2Type = RegType::vreg;
    switch (funct6)
    {
    case 0x10:
    case 0x12:
    case 0x13: inst.rs1Type = RegType::none; break;
    default: inst.rs1Type = RegType::vreg; break;
    }

    switch (funct6)
    {
    case 0x00: inst.instName = InstName::vfadd_vv; break;
    case 0x01: inst.instName = InstName::vfredusum_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x02: inst.instName = InstName::vfsub_vv; break;
    case 0x03: inst.instName = InstName::vfredosum_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x04: inst.instName = InstName::vfmin_vv; break;
    case 0x05: inst.instName = InstName::vfredmin_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x06: inst.instName = InstName::vfmax_vv; break;
    case 0x07: inst.instName = InstName::vfredmax_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x08: inst.instName = InstName::vfsgnj_vv; break;
    case 0x09: inst.instName = InstName::vfsgnjn_vv; break;
    case 0x0a: inst.instName = InstName::vfsgnjx_vv; break;
    case 0x10: inst.instName = InstName::vfmv_f_s; if (!vmflag) return false; break;
    case 0x12:
    {
        switch (extract<19, 15>(instWord))
        {
        case 0x00: inst.instName = InstName::vfcvt_xu_f_v; break;
        case 0x01: inst.instName = InstName::vfcvt_x_f_v; break;
        case 0x02: inst.instName = InstName::vfcvt_f_xu_v; break;
        case 0x03: inst.instName = InstName::vfcvt_f_x_v; break;
        case 0x06: inst.instName = InstName::vfcvt_rtz_xu_f_v; break;
        case 0x07: inst.instName = InstName::vfcvt_rtz_x_f_v; break;
        case 0x08: inst.instName = InstName::vfwcvt_xu_f_v; inst.vrdView = VRegView::m2; break;
        case 0x09: inst.instName = InstName::vfwcvt_x_f_v; inst.vrdView = VRegView::m2; break;
        case 0x0a: inst.instName = InstName::vfwcvt_f_xu_v; inst.vrdView = VRegView::m2; break;
        case 0x0b: inst.instName = InstName::vfwcvt_f_x_v; inst.vrdView = VRegView::m2; break;
        case 0x0c: inst.instName = InstName::vfwcvt_f_f_v; inst.vrdView = VRegView::m2; break;
        case 0x0e: inst.instName = InstName::vfwcvt_rtz_xu_f_v; inst.vrdView = VRegView::m2; break;
        case 0x0f: inst.instName = InstName::vfwcvt_rtz_x_f_v; inst.vrdView = VRegView::m2; break;
        case 0x10: inst.instName = InstName::vfncvt_xu_f_w; inst.vrs2View = VRegView::m2; break;
        case 0x11: inst.instName = InstName::vfncvt_x_f_w; inst.vrs2View = VRegView::m2; break;
        case 0x12: inst.instName = InstName::vfncvt_f_xu_w; inst.vrs2View = VRegView::m2; break;
        case 0x13: inst.instName = InstName::vfncvt_f_x_w; inst.vrs2View = VRegView::m2; break;
        case 0x14: inst.instName = InstName::vfncvt_f_f_w; inst.vrs2View = VRegView::m2; break;
        case 0x15: inst.instName = InstName::vfncvt_rod_f_f_w; inst.vrs2View = VRegView::m2; break;
        case 0x16: inst.instName = InstName::vfncvt_rtz_xu_f_w; inst.vrs2View = VRegView::m2; break;
        case 0x17: inst.instName = InstName::vfncvt_rtz_x_f_w; inst.vrs2View = VRegView::m2; break;
        default: return false;
        }
    } break;
    case 0x13:
    {
        switch (extract<19, 15>(instWord))
        {
        case 0x00: inst.instName = InstName::vfsqrt_v; break;
        case 0x04: inst.instName = InstName::vfrsqrt7_v; break;
        case 0x05: inst.instName = InstName::vfrec7_v; break;
        case 0x10: inst.instName = InstName::vfclass_v; break;
        default: return false;
        }
    } break;
    case 0x18: inst.instName = InstName::vmfeq_vv; inst.vrdView = VRegView::mask; break;
    case 0x19: inst.instName = InstName::vmfle_vv; inst.vrdView = VRegView::mask; break;
    case 0x1b: inst.instName = InstName::vmflt_vv; inst.vrdView = VRegView::mask; break;
    case 0x1c: inst.instName = InstName::vmfne_vv; inst.vrdView = VRegView::mask; break;
    case 0x20: inst.instName = InstName::vfdiv_vv; break;
    case 0x24: inst.instName = InstName::vfmul_vv; break;
    case 0x28: inst.instName = InstName::vfmadd_vv; break;
    case 0x29: inst.instName = InstName::vfnmadd_vv; break;
    case 0x2a: inst.instName = InstName::vfmsub_vv; break;
    case 0x2b: inst.instName = InstName::vfnmsub_vv; break;
    case 0x2c: inst.instName = InstName::vfmacc_vv; break;
    case 0x2d: inst.instName = InstName::vfnmacc_vv; break;
    case 0x2e: inst.instName = InstName::vfmsac_vv; break;
    case 0x2f: inst.instName = InstName::vfnmsac_vv; break;
    case 0x30: inst.instName = InstName::vfwadd_vv; inst.vrdView = VRegView::m2; break;
    case 0x31: inst.instName = InstName::vfwredusum_vs; inst.vrs1View = inst.vrdView = VRegView::m2; break;
    case 0x32: inst.instName = InstName::vfwsub_vv; inst.vrdView = VRegView::m2; break;
    case 0x33: inst.instName = InstName::vfwredosum_vs; inst.vrs1View = inst.vrdView = VRegView::m2; break;
    case 0x34: inst.instName = InstName::vfwadd_wv; inst.vrdView = inst.vrs2View = VRegView::m2; break;
    case 0x36: inst.instName = InstName::vfwsub_wv; inst.vrdView = inst.vrs2View = VRegView::m2; break;
    case 0x38: inst.instName = InstName::vfwmul_vv; inst.vrdView = VRegView::m2; break;
    case 0x3c: inst.instName = InstName::vfwmacc_vv; inst.vrdView = VRegView::m2; break;
    case 0x3d: inst.instName = InstName::vfwnmacc_vv; inst.vrdView = VRegView::m2; break;
    case 0x3e: inst.instName = InstName::vfwmsac_vv; inst.vrdView = VRegView::m2; break;
    case 0x3f: inst.instName = InstName::vfwnmsac_vv; inst.vrdView = VRegView::m2; break;
    default:
        return false;
    }
    return true;
}


bool _decodev_opmvv(const uint32_t &instWord, DecodedInst &inst) {
    
    uint8_t funct6 = extract<31, 26>(instWord) & 0x3f;
    bool vmflag = (instWord & (1U << 25)) != 0;

    _setupOPV_no_regtype(instWord, inst);
    inst.rdType = (funct6 == 0x10) ? (RegType::gpreg) : (RegType::vreg);
    inst.rs1Type = (funct6 == 0x10 || funct6 == 0x12 || funct6 == 0x14) ? (RegType::none) : (RegType::vreg);
    inst.rs2Type = (funct6 == 0x14 && inst.rs1 == 0x11) ? (RegType::none) : (RegType::vreg);


    switch (funct6)
    {
    case 0x00: inst.instName = InstName::vredsum_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x01: inst.instName = InstName::vredand_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x02: inst.instName = InstName::vredor_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x03: inst.instName = InstName::vredxor_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x04: inst.instName = InstName::vredminu_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x05: inst.instName = InstName::vredmin_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x06: inst.instName = InstName::vredmaxu_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x07: inst.instName = InstName::vredmax_vs; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x08: inst.instName = InstName::vaaddu_vv; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x09: inst.instName = InstName::vaadd_vv; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x0a: inst.instName = InstName::vasubu_vv; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x0b: inst.instName = InstName::vasub_vv; inst.vrs1View = inst.vrdView = VRegView::e0; break;
    case 0x10:
    {
        switch (inst.rs1)
        {
        case 0x00: inst.instName = InstName::vmv_x_s; if (!vmflag) return false; break;
        case 0x10: inst.instName = InstName::vcpop_m; break;
        case 0x11: inst.instName = InstName::vfirst_m; break;
        default: return false;
        }
    } break;
    case 0x12:
    {
        switch (inst.rs1)
        {
        case 0x02: inst.instName = InstName::vzext_vf8; inst.vrs2View = VRegView::f8; break;
        case 0x03: inst.instName = InstName::vsext_vf8; inst.vrs2View = VRegView::f8; break;
        case 0x04: inst.instName = InstName::vzext_vf4; inst.vrs2View = VRegView::f4; break;
        case 0x05: inst.instName = InstName::vsext_vf4; inst.vrs2View = VRegView::f4; break;
        case 0x06: inst.instName = InstName::vzext_vf2; inst.vrs2View = VRegView::f2; break;
        case 0x07: inst.instName = InstName::vsext_vf2; inst.vrs2View = VRegView::f2; break;
        default: return false;
        }
    } break;
    case 0x14:
    {
        switch (inst.rs1)
        {
        case 0x01: inst.instName = InstName::vmsbf_m; inst.vrs2View = inst.vrdView = VRegView::mask; break;
        case 0x02: inst.instName = InstName::vmsof_m; inst.vrs2View = inst.vrdView = VRegView::mask; break;
        case 0x03: inst.instName = InstName::vmsif_m; inst.vrs2View = inst.vrdView = VRegView::mask; break;
        case 0x10: inst.instName = InstName::viota_m; inst.vrs2View = VRegView::mask; break;
        case 0x11: inst.instName = InstName::vid_v; break;
        default: return false;
        }
    } break;
    case 0x17: inst.instName = InstName::vcompress_vm; if (!vmflag) return false; inst.vrs1View = VRegView::mask; break;
    case 0x18: inst.instName = InstName::vmandn_mm; if (!vmflag) return false; inst.vrdView = inst.vrs1View = inst.vrs2View = VRegView::mask; break;
    case 0x19: inst.instName = InstName::vmand_mm; if (!vmflag) return false; inst.vrdView = inst.vrs1View = inst.vrs2View = VRegView::mask; break;
    case 0x1a: inst.instName = InstName::vmor_mm; if (!vmflag) return false; inst.vrdView = inst.vrs1View = inst.vrs2View = VRegView::mask; break;
    case 0x1b: inst.instName = InstName::vmxor_mm; if (!vmflag) return false; inst.vrdView = inst.vrs1View = inst.vrs2View = VRegView::mask; break;
    case 0x1c: inst.instName = InstName::vmorn_mm; if (!vmflag) return false; inst.vrdView = inst.vrs1View = inst.vrs2View = VRegView::mask; break;
    case 0x1d: inst.instName = InstName::vmnand_mm; if (!vmflag) return false; inst.vrdView = inst.vrs1View = inst.vrs2View = VRegView::mask; break;
    case 0x1e: inst.instName = InstName::vmnor_mm; if (!vmflag) return false; inst.vrdView = inst.vrs1View = inst.vrs2View = VRegView::mask; break;
    case 0x1f: inst.instName = InstName::vmxnor_mm; if (!vmflag) return false; inst.vrdView = inst.vrs1View = inst.vrs2View = VRegView::mask; break;
    case 0x20: inst.instName = InstName::vdivu_vv; break;
    case 0x21: inst.instName = InstName::vdiv_vv; break;
    case 0x22: inst.instName = InstName::vremu_vv; break;
    case 0x23: inst.instName = InstName::vrem_vv; break;
    case 0x24: inst.instName = InstName::vmulhu_vv; break;
    case 0x25: inst.instName = InstName::vmul_vv; break;
    case 0x26: inst.instName = InstName::vmulhsu_vv; break;
    case 0x27: inst.instName = InstName::vmulh_vv; break;
    case 0x29: inst.instName = InstName::vmadd_vv; break;
    case 0x2b: inst.instName = InstName::vnmsub_vv; break;
    case 0x2d: inst.instName = InstName::vmacc_vv; break;
    case 0x2f: inst.instName = InstName::vnmsac_vv; break;
    case 0x30: inst.instName = InstName::vwaddu_vv; inst.vrdView = VRegView::m2; break;
    case 0x31: inst.instName = InstName::vwadd_vv; inst.vrdView = VRegView::m2; break;
    case 0x32: inst.instName = InstName::vwsubu_vv; inst.vrdView = VRegView::m2; break;
    case 0x33: inst.instName = InstName::vwsub_vv; inst.vrdView = VRegView::m2; break;
    case 0x34: inst.instName = InstName::vwaddu_wv; inst.vrdView = inst.vrs2View = VRegView::m2; break;
    case 0x35: inst.instName = InstName::vwadd_wv; inst.vrdView = inst.vrs2View = VRegView::m2; break;
    case 0x36: inst.instName = InstName::vwsubu_wv; inst.vrdView = inst.vrs2View = VRegView::m2; break;
    case 0x37: inst.instName = InstName::vwsub_wv; inst.vrdView = inst.vrs2View = VRegView::m2; break;
    case 0x38: inst.instName = InstName::vwmulu_vv; inst.vrdView = VRegView::m2; break;
    case 0x3a: inst.instName = InstName::vwmulsu_vv; inst.vrdView = VRegView::m2; break;
    case 0x3b: inst.instName = InstName::vwmul_vv; inst.vrdView = VRegView::m2; break;
    case 0x3c: inst.instName = InstName::vwmaccu_vv; inst.vrdView = VRegView::m2; break;
    case 0x3d: inst.instName = InstName::vwmacc_vv; inst.vrdView = VRegView::m2; break;
    case 0x3f: inst.instName = InstName::vwmaccsu_vv; inst.vrdView = VRegView::m2; break;
    default:
        return false;
    }
    return true;
}

bool _decodev_opivi(const uint32_t &instWord, DecodedInst &inst) {

    uint8_t funct6 = extract<31, 26>(instWord) & 0x3f;
    bool vmflag = (instWord & (1U << 25)) != 0;

    _setupOPV_no_regtype(instWord, inst);
    inst.rdType = RegType::vreg;
    inst.rs2Type = ((funct6 == 0x17 && vmflag) ? (RegType::none) : (RegType::vreg)); // vmv.v.i

    switch (funct6)
    {
    case 0x0c:
    case 0x0e:
    case 0x0f:
    case 0x25:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x2e:
    case 0x2f: inst.imm = static_cast<uint64_t>(inst.rs1); break;
    default: inst.imm = signExtend<4>(inst.rs1); break;
    }

    switch (funct6)
    {
    case 0x00: inst.instName = InstName::vadd_vi; break;
    case 0x03: inst.instName = InstName::vrsub_vi; break;
    case 0x09: inst.instName = InstName::vand_vi; break;
    case 0x0a: inst.instName = InstName::vor_vi; break;
    case 0x0b: inst.instName = InstName::vxor_vi; break;
    case 0x0c: inst.instName = InstName::vrgather_vi; break;
    case 0x0e: inst.instName = InstName::vslideup_vi; break;
    case 0x0f: inst.instName = InstName::vslidedown_vi; break;
    case 0x10: inst.instName = InstName::vadc_vim; if (vmflag) return false; break;
    case 0x11: inst.instName = (vmflag ? (InstName::vmadc_vi) : (InstName::vmadc_vim)); inst.vrdView = VRegView::mask; break;
    case 0x17: inst.instName = (vmflag ? (InstName::vmv_v_i) : (InstName::vmerge_vim)); break;
    case 0x18: inst.instName = InstName::vmseq_vi; inst.vrdView = VRegView::mask; break;
    case 0x19: inst.instName = InstName::vmsne_vi; inst.vrdView = VRegView::mask; break;
    case 0x1c: inst.instName = InstName::vmsleu_vi; inst.vrdView = VRegView::mask; break;
    case 0x1d: inst.instName = InstName::vmsle_vi; inst.vrdView = VRegView::mask; break;
    case 0x1e: inst.instName = InstName::vmsgtu_vi; inst.vrdView = VRegView::mask; break;
    case 0x1f: inst.instName = InstName::vmsgt_vi; inst.vrdView = VRegView::mask; break;
    case 0x20: inst.instName = InstName::vsaddu_vi; break;
    case 0x21: inst.instName = InstName::vsadd_vi; break;
    case 0x25: inst.instName = InstName::vsll_vi; break;
    case 0x27:
    {
        switch (inst.rs1)
        {
        case 0: inst.instName = InstName::vmv1r_v; inst.vrs2View = inst.vrdView = VRegView::reg1; break;
        case 1: inst.instName = InstName::vmv2r_v; inst.vrs2View = inst.vrdView = VRegView::reg2; break;
        case 3: inst.instName = InstName::vmv4r_v; inst.vrs2View = inst.vrdView = VRegView::reg4; break;
        case 7: inst.instName = InstName::vmv8r_v; inst.vrs2View = inst.vrdView = VRegView::reg8; break;
        default: return false;
        }
    } break;
    case 0x28: inst.instName = InstName::vsrl_vi; break;
    case 0x29: inst.instName = InstName::vsra_vi; break;
    case 0x2a: inst.instName = InstName::vssrl_vi; break;
    case 0x2b: inst.instName = InstName::vssra_vi; break;
    case 0x2c: inst.instName = InstName::vnsrl_wi; inst.vrs2View = VRegView::m2; break;
    case 0x2d: inst.instName = InstName::vnsra_wi; inst.vrs2View = VRegView::m2; break;
    case 0x2e: inst.instName = InstName::vnclipu_wi; inst.vrs2View = VRegView::m2; break;
    case 0x2f: inst.instName = InstName::vnclip_wi; inst.vrs2View = VRegView::m2; break;
    default:
        return false;
    }
    return true;
}

bool _decodev_opivx(const uint32_t &instWord, DecodedInst &inst) {

    uint8_t funct6 = extract<31, 26>(instWord) & 0x3f;
    bool vmflag = (instWord & (1U << 25)) != 0;

    _setupOPV_no_regtype(instWord, inst);

    inst.rdType = RegType::vreg;
    inst.rs1Type = RegType::gpreg;
    inst.rs2Type = ((funct6 == 0x17 && vmflag) ? (RegType::none) : (RegType::vreg)); // vmv.v.x

    switch (funct6)
    {
    case 0x00: inst.instName = InstName::vadd_vx; break;
    case 0x02: inst.instName = InstName::vsub_vx; break;
    case 0x03: inst.instName = InstName::vrsub_vx; break;
    case 0x04: inst.instName = InstName::vminu_vx; break;
    case 0x05: inst.instName = InstName::vmin_vx; break;
    case 0x06: inst.instName = InstName::vmaxu_vx; break;
    case 0x07: inst.instName = InstName::vmax_vx; break;
    case 0x09: inst.instName = InstName::vand_vx; break;
    case 0x0a: inst.instName = InstName::vor_vx; break;
    case 0x0b: inst.instName = InstName::vxor_vx; break;
    case 0x0c: inst.instName = InstName::vrgather_vx; break;
    case 0x0e: inst.instName = InstName::vslideup_vx; break;
    case 0x0f: inst.instName = InstName::vslidedown_vx; break;
    case 0x10: inst.instName = InstName::vadc_vxm; if (vmflag) return false; break;
    case 0x11: inst.instName = (vmflag ? (InstName::vmadc_vx) : (InstName::vmadc_vxm)); inst.vrdView = VRegView::mask; break;
    case 0x12: inst.instName = InstName::vsbc_vxm; if (vmflag) return false; break;
    case 0x13: inst.instName = (vmflag ? (InstName::vmsbc_vx) : (InstName::vmsbc_vxm)); inst.vrdView = VRegView::mask; break;
    case 0x17: inst.instName = (vmflag ? (InstName::vmv_v_x) : (InstName::vmerge_vxm)); break;
    case 0x18: inst.instName = InstName::vmseq_vx; inst.vrdView = VRegView::mask; break;
    case 0x19: inst.instName = InstName::vmsne_vx; inst.vrdView = VRegView::mask; break;
    case 0x1a: inst.instName = InstName::vmsltu_vx; inst.vrdView = VRegView::mask; break;
    case 0x1b: inst.instName = InstName::vmslt_vx; inst.vrdView = VRegView::mask; break;
    case 0x1c: inst.instName = InstName::vmsleu_vx; inst.vrdView = VRegView::mask; break;
    case 0x1d: inst.instName = InstName::vmsle_vx; inst.vrdView = VRegView::mask; break;
    case 0x1e: inst.instName = InstName::vmsgtu_vx; inst.vrdView = VRegView::mask; break;
    case 0x1f: inst.instName = InstName::vmsgt_vx; inst.vrdView = VRegView::mask; break;
    case 0x20: inst.instName = InstName::vsaddu_vx; break;
    case 0x21: inst.instName = InstName::vsadd_vx; break;
    case 0x22: inst.instName = InstName::vssubu_vx; break;
    case 0x23: inst.instName = InstName::vssub_vx; break;
    case 0x25: inst.instName = InstName::vsll_vx; break;
    case 0x27: inst.instName = InstName::vsmul_vx; break;
    case 0x28: inst.instName = InstName::vsrl_vx; break;
    case 0x29: inst.instName = InstName::vsra_vx; break;
    case 0x2a: inst.instName = InstName::vssrl_vx; break;
    case 0x2b: inst.instName = InstName::vssra_vx; break;
    case 0x2c: inst.instName = InstName::vnsrl_wx; inst.vrs2View = VRegView::m2; break;
    case 0x2d: inst.instName = InstName::vnsra_wx; inst.vrs2View = VRegView::m2; break;
    case 0x2e: inst.instName = InstName::vnclipu_wx; inst.vrs2View = VRegView::m2; break;
    case 0x2f: inst.instName = InstName::vnclip_wx; inst.vrs2View = VRegView::m2; break;
    default:
        return false;
    }
    return true;
}

bool _decodev_opfvf(const uint32_t &instWord, DecodedInst &inst) {

    uint8_t funct6 = extract<31, 26>(instWord) & 0x3f;
    bool vmflag = (instWord & (1U << 25)) != 0;

    _setupOPV_no_regtype(instWord, inst);
    inst.rdType = RegType::vreg;
    inst.rs1Type = RegType::freg;
    inst.rs2Type = ((funct6 == 0x10 || (funct6 == 0x17 && vmflag)) ? (RegType::none) : (RegType::vreg));


    switch (funct6) // funct6
    {
    case 0x00: inst.instName = InstName::vfadd_vf; break;
    case 0x02: inst.instName = InstName::vfsub_vf; break;
    case 0x04: inst.instName = InstName::vfmin_vf; break;
    case 0x06: inst.instName = InstName::vfmax_vf; break;
    case 0x08: inst.instName = InstName::vfsgnj_vf; break;
    case 0x09: inst.instName = InstName::vfsgnjn_vf; break;
    case 0x0a: inst.instName = InstName::vfsgnjx_vf; break;
    case 0x0e: inst.instName = InstName::vfslide1up_vf; break;
    case 0x0f: inst.instName = InstName::vfslide1down_vf; break;
    case 0x10: inst.instName = InstName::vfmv_s_f; if (!vmflag) return false; break;
    case 0x17: inst.instName = (vmflag ? (InstName::vfmerge_vfm) : (InstName::vfmv_v_f)); break;
    case 0x18: inst.instName = InstName::vmfeq_vf; inst.vrdView = VRegView::mask; break;
    case 0x19: inst.instName = InstName::vmfle_vf; inst.vrdView = VRegView::mask; break;
    case 0x1b: inst.instName = InstName::vmflt_vf; inst.vrdView = VRegView::mask; break;
    case 0x1c: inst.instName = InstName::vmfne_vf; inst.vrdView = VRegView::mask; break;
    case 0x1d: inst.instName = InstName::vmfgt_vf; inst.vrdView = VRegView::mask; break;
    case 0x1f: inst.instName = InstName::vmfge_vf; inst.vrdView = VRegView::mask; break;
    case 0x20: inst.instName = InstName::vfdiv_vf; break;
    case 0x21: inst.instName = InstName::vfrdiv_vf; break;
    case 0x24: inst.instName = InstName::vfmul_vf; break;
    case 0x27: inst.instName = InstName::vfrsub_vf; break;
    case 0x28: inst.instName = InstName::vfmadd_vf; break;
    case 0x29: inst.instName = InstName::vfnmadd_vf; break;
    case 0x2a: inst.instName = InstName::vfmsub_vf; break;
    case 0x2b: inst.instName = InstName::vfnmsub_vf; break;
    case 0x2c: inst.instName = InstName::vfmacc_vf; break;
    case 0x2d: inst.instName = InstName::vfnmacc_vf; break;
    case 0x2e: inst.instName = InstName::vfmsac_vf; break;
    case 0x2f: inst.instName = InstName::vfnmsac_vf; break;
    case 0x30: inst.instName = InstName::vfwadd_vf; inst.vrdView = VRegView::m2; break;
    case 0x32: inst.instName = InstName::vfwsub_vf; inst.vrdView = VRegView::m2; break;
    case 0x34: inst.instName = InstName::vfwadd_wf; inst.vrs2View = inst.vrdView = VRegView::m2; break;
    case 0x36: inst.instName = InstName::vfwsub_wf; inst.vrs2View = inst.vrdView = VRegView::m2; break;
    case 0x38: inst.instName = InstName::vfwmul_vf; inst.vrdView = VRegView::m2; break;
    case 0x3c: inst.instName = InstName::vfwmacc_vf; inst.vrdView = VRegView::m2; break;
    case 0x3d: inst.instName = InstName::vfwnmacc_vf; inst.vrdView = VRegView::m2; break;
    case 0x3e: inst.instName = InstName::vfwmsac_vf; inst.vrdView = VRegView::m2; break;
    case 0x3f: inst.instName = InstName::vfwnmsac_vf; inst.vrdView = VRegView::m2; break;
    default:
        return false;
    }
    return true;
}

bool _decodev_opmvx(const uint32_t &instWord, DecodedInst &inst) {

    uint8_t funct6 = extract<31, 26>(instWord) & 0x3f;
    bool vmflag = (instWord & (1U << 25)) != 0;

    _setupOPV_no_regtype(instWord, inst);

    inst.rdType = RegType::vreg;
    inst.rs1Type = RegType::gpreg;
    inst.rs2Type = ((funct6 == 0x10) ? (RegType::none) : (RegType::vreg)); // vmv.s.x

    switch (funct6)
    {
    case 0x08: inst.instName = InstName::vaaddu_vx; break;
    case 0x09: inst.instName = InstName::vaadd_vx; break;
    case 0x0a: inst.instName = InstName::vasubu_vx; break;
    case 0x0b: inst.instName = InstName::vasub_vx; break;
    case 0x0e: inst.instName = InstName::vslide1up_vx; break;
    case 0x0f: inst.instName = InstName::vslide1down_vx; break;
    case 0x10: inst.instName = InstName::vmv_s_x; if (!vmflag) return false; break;
    case 0x20: inst.instName = InstName::vdivu_vx; break;
    case 0x21: inst.instName = InstName::vdiv_vx; break;
    case 0x22: inst.instName = InstName::vremu_vx; break;
    case 0x23: inst.instName = InstName::vrem_vx; break;
    case 0x24: inst.instName = InstName::vmulhu_vx; break;
    case 0x25: inst.instName = InstName::vmul_vx; break;
    case 0x26: inst.instName = InstName::vmulhsu_vx; break;
    case 0x27: inst.instName = InstName::vmulh_vx; break;
    case 0x29: inst.instName = InstName::vmadd_vx; break;
    case 0x2b: inst.instName = InstName::vnmsub_vx; break;
    case 0x2d: inst.instName = InstName::vmacc_vx; break;
    case 0x2f: inst.instName = InstName::vnmsac_vx; break;
    case 0x30: inst.instName = InstName::vwaddu_vx; inst.vrdView = VRegView::m2; break;
    case 0x31: inst.instName = InstName::vwadd_vx; inst.vrdView = VRegView::m2; break;
    case 0x32: inst.instName = InstName::vwsubu_vx; inst.vrdView = VRegView::m2; break;
    case 0x33: inst.instName = InstName::vwsub_vx; inst.vrdView = VRegView::m2; break;
    case 0x34: inst.instName = InstName::vwaddu_wx; inst.vrs2View = inst.vrdView = VRegView::m2; break;
    case 0x35: inst.instName = InstName::vwadd_wx; inst.vrs2View = inst.vrdView = VRegView::m2; break;
    case 0x36: inst.instName = InstName::vwsubu_wx; inst.vrs2View = inst.vrdView = VRegView::m2; break;
    case 0x37: inst.instName = InstName::vwsub_wx; inst.vrs2View = inst.vrdView = VRegView::m2; break;
    case 0x38: inst.instName = InstName::vwmulu_vx; inst.vrdView = VRegView::m2; break;
    case 0x3a: inst.instName = InstName::vwmulsu_vx; inst.vrdView = VRegView::m2; break;
    case 0x3b: inst.instName = InstName::vwmul_vx; inst.vrdView = VRegView::m2; break;
    case 0x3c: inst.instName = InstName::vwmaccu_vx; inst.vrdView = VRegView::m2; break;
    case 0x3d: inst.instName = InstName::vwmacc_vx; inst.vrdView = VRegView::m2; break;
    case 0x3e: inst.instName = InstName::vwmaccus_vx; inst.vrdView = VRegView::m2; break;
    case 0x3f: inst.instName = InstName::vwmaccsu_vx; inst.vrdView = VRegView::m2; break;
    default: return false;
    }
    return true;
}

bool _decodev_opcfg(const uint32_t &instWord, DecodedInst &inst) {

    _setupOPV_no_regtype(instWord, inst);
    inst.rdType = RegType::vreg;

    if (instWord & (1U << 31)) {
        if (instWord & (1U << 30)) {
            inst.imm = extract<29, 20>(instWord);
            inst.instName = InstName::vsetivli;
        } else {
            inst.rs1Type = RegType::gpreg;
            inst.rs2Type = RegType::gpreg;
            inst.instName = InstName::vsetvl;
        }
    } else {
        inst.imm = extract<30, 20>(instWord);
        inst.rs1Type = RegType::gpreg;
        inst.instName = InstName::vsetvli;
    }
    
    return true;
}

bool decode_opv(const uint32_t &instWord, DecodedInst &inst) {
    inst.opcode = OPCode::opv;
    uint8_t funct3 = extract<14, 12>(instWord) & 0x7;
    switch (funct3)
    {
    case 0b000: return _decodev_opivv(instWord, inst);
    case 0b001: return _decodev_opfvv(instWord, inst);
    case 0b010: return _decodev_opmvv(instWord, inst);
    case 0b011: return _decodev_opivi(instWord, inst);
    case 0b100: return _decodev_opivx(instWord, inst);
    case 0b101: return _decodev_opfvf(instWord, inst);
    case 0b110: return _decodev_opmvx(instWord, inst);
    case 0b111: return _decodev_opcfg(instWord, inst);
    }
    return false;
}

}

