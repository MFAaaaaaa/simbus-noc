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


enum class RV64FPOP5 {
    ADD     = 0x00,
    SUB     = 0x01,
    MUL     = 0x02,
    DIV     = 0x03,
    SQRT    = 0x0b,
    SGNJ    = 0x04,
    MIN     = 0x05,
    MVF2F   = 0x08,
    CMP     = 0x14,
    CVTF2I  = 0x18,
    CVTI2F  = 0x1a,
    MVF2I   = 0x1c,
    MVI2F   = 0x1e,
};

enum class RV64FPWidth2 {
    fword   = 0,
    fdword  = 1,
    fhalf   = 2,
    fqword  = 3
};

enum class RV64FPCVTWidth5 {
    word    = 0,
    uword   = 1,
    dword   = 2,
    udword  = 3
};

enum class RV64FPMIN3 {
    MIN     = 0,
    MAX     = 1
};

enum class RV64FPSGNJ3 {
    SGNJ    = 0,
    SGNJN   = 1,
    SGNJX   = 2
};

enum class RV64FPMVI3 {
    RAW     = 0,
    FCLASS  = 1
};

enum class RV64FPCMP3 {
    FEQ     = 2,
    FLT     = 1,
    FLE     = 0
};

inline constexpr InstName rv64_fpop_inst_name(const uint32_t funct7, const uint32_t funct3, const uint8_t rs2) {
    RV64FPWidth2 fpwid = static_cast<RV64FPWidth2>(funct7 & 0x3);

    switch (static_cast<RV64FPOP5>(funct7 >> 2))
    {
    case RV64FPOP5::ADD: switch (fpwid)
        {
        case RV64FPWidth2::fword: return InstName::fadd_s; break;
        case RV64FPWidth2::fdword: return InstName::fadd_d; break;
        case RV64FPWidth2::fhalf: return InstName::fadd_h; break;
        case RV64FPWidth2::fqword: return InstName::fadd_q; break;
        default: return InstName::illegal;
        }
    case RV64FPOP5::SUB: switch (fpwid)
        {
        case RV64FPWidth2::fword: return InstName::fsub_s; break;
        case RV64FPWidth2::fdword: return InstName::fsub_d; break;
        case RV64FPWidth2::fhalf: return InstName::fsub_h; break;
        case RV64FPWidth2::fqword: return InstName::fsub_q; break;
        default: return InstName::illegal;
        }
    case RV64FPOP5::MUL: switch (fpwid)
        {
        case RV64FPWidth2::fword: return InstName::fmul_s; break;
        case RV64FPWidth2::fdword: return InstName::fmul_d; break;
        case RV64FPWidth2::fhalf: return InstName::fmul_h; break;
        case RV64FPWidth2::fqword: return InstName::fmul_q; break;
        default: return InstName::illegal;
        }
    case RV64FPOP5::DIV: switch (fpwid)
        {
        case RV64FPWidth2::fword: return InstName::fdiv_s; break;
        case RV64FPWidth2::fdword: return InstName::fdiv_d; break;
        case RV64FPWidth2::fhalf: return InstName::fdiv_h; break;
        case RV64FPWidth2::fqword: return InstName::fdiv_q; break;
        default: return InstName::illegal;
        }
    case RV64FPOP5::SQRT: switch (fpwid)
        {
        case RV64FPWidth2::fword: return InstName::fsqrt_s; break;
        case RV64FPWidth2::fdword: return InstName::fsqrt_d; break;
        case RV64FPWidth2::fhalf: return InstName::fsqrt_h; break;
        case RV64FPWidth2::fqword: return InstName::fsqrt_q; break;
        default: return InstName::illegal;
        }
    case RV64FPOP5::SGNJ: 
    {
        RV64FPSGNJ3 sgnj3 = static_cast<RV64FPSGNJ3>(funct3);
        switch (fpwid)
        {
        case RV64FPWidth2::fword: switch (sgnj3)
            {
            case RV64FPSGNJ3::SGNJ: return InstName::fsgnj_s; break;
            case RV64FPSGNJ3::SGNJN: return InstName::fsgnjn_s; break;
            case RV64FPSGNJ3::SGNJX: return InstName::fsgnjx_s; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fdword: switch (sgnj3)
            {
            case RV64FPSGNJ3::SGNJ: return InstName::fsgnj_d; break;
            case RV64FPSGNJ3::SGNJN: return InstName::fsgnjn_d; break;
            case RV64FPSGNJ3::SGNJX: return InstName::fsgnjx_d; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fhalf: switch (sgnj3)
            {
            case RV64FPSGNJ3::SGNJ: return InstName::fsgnj_h; break;
            case RV64FPSGNJ3::SGNJN: return InstName::fsgnjn_h; break;
            case RV64FPSGNJ3::SGNJX: return InstName::fsgnjx_h; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fqword: switch (sgnj3)
            {
            case RV64FPSGNJ3::SGNJ: return InstName::fsgnj_q; break;
            case RV64FPSGNJ3::SGNJN: return InstName::fsgnjn_q; break;
            case RV64FPSGNJ3::SGNJX: return InstName::fsgnjx_q; break;
            default: return InstName::illegal;
            }
        default: return InstName::illegal;
        }
    }
    case RV64FPOP5::MIN:
    {
        RV64FPMIN3 min3 = static_cast<RV64FPMIN3>(funct3);
        switch (fpwid)
        {
        case RV64FPWidth2::fword: switch (min3)
            {
            case RV64FPMIN3::MIN: return InstName::fmin_s; break;
            case RV64FPMIN3::MAX: return InstName::fmax_s; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fdword: switch (min3)
            {
            case RV64FPMIN3::MIN: return InstName::fmin_d; break;
            case RV64FPMIN3::MAX: return InstName::fmax_d; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fhalf: switch (min3)
            {
            case RV64FPMIN3::MIN: return InstName::fmin_h; break;
            case RV64FPMIN3::MAX: return InstName::fmax_h; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fqword: switch (min3)
            {
            case RV64FPMIN3::MIN: return InstName::fmin_q; break;
            case RV64FPMIN3::MAX: return InstName::fmax_q; break;
            default: return InstName::illegal;
            }
        default: return InstName::illegal;
        }
    }
    case RV64FPOP5::MVF2F:
    {
        RV64FPWidth2 fpwid_rs2 = static_cast<RV64FPWidth2>(rs2 & 0x3);
        switch (fpwid)
        {
        case RV64FPWidth2::fword: switch (fpwid_rs2)
            {
            case RV64FPWidth2::fdword: return InstName::fcvt_s_d; break;
            case RV64FPWidth2::fhalf: return InstName::fcvt_s_h; break;
            case RV64FPWidth2::fqword: return InstName::fcvt_s_q; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fdword: switch (fpwid_rs2)
            {
            case RV64FPWidth2::fword: return InstName::fcvt_d_s; break;
            case RV64FPWidth2::fhalf: return InstName::fcvt_d_h; break;
            case RV64FPWidth2::fqword: return InstName::fcvt_d_q; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fhalf: switch (fpwid_rs2)
            {
            case RV64FPWidth2::fword: return InstName::fcvt_h_s; break;
            case RV64FPWidth2::fdword: return InstName::fcvt_h_d; break;
            case RV64FPWidth2::fqword: return InstName::fcvt_h_q; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fqword: switch (fpwid_rs2)
            {
            case RV64FPWidth2::fword: return InstName::fcvt_q_s; break;
            case RV64FPWidth2::fdword: return InstName::fcvt_q_d; break;
            case RV64FPWidth2::fhalf: return InstName::fcvt_q_h; break;
            default: return InstName::illegal;
            }
        default: return InstName::illegal;
        }
    }
    case RV64FPOP5::CMP: 
    {
        RV64FPCMP3 cmp3 = static_cast<RV64FPCMP3>(funct3);
        switch (fpwid)
        {
        case RV64FPWidth2::fword: switch (cmp3)
            {
            case RV64FPCMP3::FEQ: return InstName::feq_s; break;
            case RV64FPCMP3::FLT: return InstName::flt_s; break;
            case RV64FPCMP3::FLE: return InstName::fle_s; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fdword: switch (cmp3)
            {
            case RV64FPCMP3::FEQ: return InstName::feq_d; break;
            case RV64FPCMP3::FLT: return InstName::flt_d; break;
            case RV64FPCMP3::FLE: return InstName::fle_d; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fhalf: switch (cmp3)
            {
            case RV64FPCMP3::FEQ: return InstName::feq_h; break;
            case RV64FPCMP3::FLT: return InstName::flt_h; break;
            case RV64FPCMP3::FLE: return InstName::fle_h; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fqword: switch (cmp3)
            {
            case RV64FPCMP3::FEQ: return InstName::feq_q; break;
            case RV64FPCMP3::FLT: return InstName::flt_q; break;
            case RV64FPCMP3::FLE: return InstName::fle_q; break;
            default: return InstName::illegal;
            }
        default: return InstName::illegal;
        }
    }
    case RV64FPOP5::CVTF2I:
    {
        RV64FPCVTWidth5 cvtwidth5 = static_cast<RV64FPCVTWidth5>(rs2 & 0x7);
        switch (fpwid)
        {
        case RV64FPWidth2::fword: switch (cvtwidth5)
            {
            case RV64FPCVTWidth5::word: return InstName::fcvt_w_s; break;
            case RV64FPCVTWidth5::uword: return InstName::fcvt_wu_s; break;
            case RV64FPCVTWidth5::dword: return InstName::fcvt_l_s; break;
            case RV64FPCVTWidth5::udword: return InstName::fcvt_lu_s; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fdword: switch (cvtwidth5)
            {
            case RV64FPCVTWidth5::word: return InstName::fcvt_w_d; break;
            case RV64FPCVTWidth5::uword: return InstName::fcvt_wu_d; break;
            case RV64FPCVTWidth5::dword: return InstName::fcvt_l_d; break;
            case RV64FPCVTWidth5::udword: return InstName::fcvt_lu_d; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fhalf: switch (cvtwidth5)
            {
            case RV64FPCVTWidth5::word: return InstName::fcvt_w_h; break;
            case RV64FPCVTWidth5::uword: return InstName::fcvt_wu_h; break;
            case RV64FPCVTWidth5::dword: return InstName::fcvt_l_h; break;
            case RV64FPCVTWidth5::udword: return InstName::fcvt_lu_h; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fqword: switch (cvtwidth5)
            {
            case RV64FPCVTWidth5::word: return InstName::fcvt_w_q; break;
            case RV64FPCVTWidth5::uword: return InstName::fcvt_wu_q; break;
            case RV64FPCVTWidth5::dword: return InstName::fcvt_l_q; break;
            case RV64FPCVTWidth5::udword: return InstName::fcvt_lu_q; break;
            default: return InstName::illegal;
            }
        default: return InstName::illegal;
        }
    }
    case RV64FPOP5::CVTI2F: 
    {
        RV64FPCVTWidth5 cvtwidth5 = static_cast<RV64FPCVTWidth5>(rs2 & 0x7);
        switch (fpwid)
        {
        case RV64FPWidth2::fword: switch (cvtwidth5)
            {
            case RV64FPCVTWidth5::word: return InstName::fcvt_s_w; break;
            case RV64FPCVTWidth5::uword: return InstName::fcvt_s_wu; break;
            case RV64FPCVTWidth5::dword: return InstName::fcvt_s_l; break;
            case RV64FPCVTWidth5::udword: return InstName::fcvt_s_lu; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fdword: switch (cvtwidth5)
            {
            case RV64FPCVTWidth5::word: return InstName::fcvt_d_w; break;
            case RV64FPCVTWidth5::uword: return InstName::fcvt_d_wu; break;
            case RV64FPCVTWidth5::dword: return InstName::fcvt_d_l; break;
            case RV64FPCVTWidth5::udword: return InstName::fcvt_d_lu; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fhalf: switch (cvtwidth5)
            {
            case RV64FPCVTWidth5::word: return InstName::fcvt_h_w; break;
            case RV64FPCVTWidth5::uword: return InstName::fcvt_h_wu; break;
            case RV64FPCVTWidth5::dword: return InstName::fcvt_h_l; break;
            case RV64FPCVTWidth5::udword: return InstName::fcvt_h_lu; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fqword: switch (cvtwidth5)
            {
            case RV64FPCVTWidth5::word: return InstName::fcvt_q_w; break;
            case RV64FPCVTWidth5::uword: return InstName::fcvt_q_wu; break;
            case RV64FPCVTWidth5::dword: return InstName::fcvt_q_l; break;
            case RV64FPCVTWidth5::udword: return InstName::fcvt_q_lu; break;
            default: return InstName::illegal;
            }
        default: return InstName::illegal;
        }
    }
    case RV64FPOP5::MVF2I:
    {
        RV64FPMVI3 mvf2i3 = static_cast<RV64FPMVI3>(funct3);
        switch (fpwid)
        {
        case RV64FPWidth2::fword: switch (mvf2i3)
            {
            case RV64FPMVI3::RAW: return InstName::fmv_x_w; break;
            case RV64FPMVI3::FCLASS: return InstName::fclass_s; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fdword: switch (mvf2i3)
            {
            case RV64FPMVI3::RAW: return InstName::fmv_x_d; break;
            case RV64FPMVI3::FCLASS: return InstName::fclass_d; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fhalf: switch (mvf2i3)
            {
            case RV64FPMVI3::RAW: return InstName::fmv_x_h; break;
            case RV64FPMVI3::FCLASS: return InstName::fclass_h; break;
            default: return InstName::illegal;
            }
        case RV64FPWidth2::fqword: switch (mvf2i3)
            {
            case RV64FPMVI3::FCLASS: return InstName::fclass_q; break;
            default: return InstName::illegal;
            }
        default: return InstName::illegal;
        }
    }
    case RV64FPOP5::MVI2F: switch (fpwid)
        {
        case RV64FPWidth2::fword: return InstName::fmv_w_x; break;
        case RV64FPWidth2::fdword: return InstName::fmv_d_x; break;
        case RV64FPWidth2::fhalf: return InstName::fmv_h_x; break;
        default: return InstName::illegal;
        }
    default:
        return InstName::illegal;
    }
}

inline constexpr bool rv64_fpop_is_valid(uint32_t funct7, uint32_t funct3, uint8_t rs2) {
    switch (static_cast<RV64FPOP5>(funct7 >> 2))
    {
    case RV64FPOP5::ADD:
    case RV64FPOP5::SUB:
    case RV64FPOP5::MUL:
    case RV64FPOP5::DIV:
    case RV64FPOP5::SQRT: return true; break;
    case RV64FPOP5::SGNJ: if (funct3 <= 2) return true; break;
    case RV64FPOP5::MIN: if (funct3 <= 1) return true; break;
    case RV64FPOP5::MVF2F: if (rs2 <= 3) return true; break;
    case RV64FPOP5::CMP: if (funct3 <= 2) return true; break;
    case RV64FPOP5::CVTF2I: if (rs2 <= 3) return true; break;
    case RV64FPOP5::CVTI2F: if (rs2 <= 3) return true; break;
    case RV64FPOP5::MVF2I:  if (funct3 <= 1) return true; break;
    case RV64FPOP5::MVI2F: if (funct3 == 0) return true; break;
    default: return false;
    }
    return false;
}

inline constexpr InstName rv64_fmadd_inst_name(const uint32_t funct7) {
    RV64FPWidth2 fpwid = static_cast<RV64FPWidth2>(funct7 & 0x3);
    switch (fpwid)
    {
    case RV64FPWidth2::fword: return InstName::fmadd_s; break;
    case RV64FPWidth2::fdword: return InstName::fmadd_d; break;
    case RV64FPWidth2::fhalf: return InstName::fmadd_h; break;
    case RV64FPWidth2::fqword: return InstName::fmadd_q; break;
    default: return InstName::illegal;
    }
}
inline constexpr InstName rv64_fnmadd_inst_name(const uint32_t funct7) {
    RV64FPWidth2 fpwid = static_cast<RV64FPWidth2>(funct7 & 0x3);
    switch (fpwid)
    {
    case RV64FPWidth2::fword: return InstName::fnmadd_s; break;
    case RV64FPWidth2::fdword: return InstName::fnmadd_d; break;
    case RV64FPWidth2::fhalf: return InstName::fnmadd_h; break;
    case RV64FPWidth2::fqword: return InstName::fnmadd_q; break;
    default: return InstName::illegal;
    }
}
inline constexpr InstName rv64_fmsub_inst_name(const uint32_t funct7) {
    RV64FPWidth2 fpwid = static_cast<RV64FPWidth2>(funct7 & 0x3);
    switch (fpwid)
    {
    case RV64FPWidth2::fword: return InstName::fmsub_s; break;
    case RV64FPWidth2::fdword: return InstName::fmsub_d; break;
    case RV64FPWidth2::fhalf: return InstName::fmsub_h; break;
    case RV64FPWidth2::fqword: return InstName::fmsub_q; break;
    default: return InstName::illegal;
    }
}
inline constexpr InstName rv64_fnmsub_inst_name(const uint32_t funct7) {
    RV64FPWidth2 fpwid = static_cast<RV64FPWidth2>(funct7 & 0x3);
    switch (fpwid)
    {
    case RV64FPWidth2::fword: return InstName::fnmsub_s; break;
    case RV64FPWidth2::fdword: return InstName::fnmsub_d; break;
    case RV64FPWidth2::fhalf: return InstName::fnmsub_h; break;
    case RV64FPWidth2::fqword: return InstName::fnmsub_q; break;
    default: return InstName::illegal;
    }
}


inline constexpr bool rv64_fpop_is_i_rd(RV64FPOP5 op) {
    return ((op == RV64FPOP5::CMP) || (op == RV64FPOP5::CVTF2I) || (op == RV64FPOP5::MVF2I));
}
inline constexpr bool rv64_fpop_is_i_s1(RV64FPOP5 op) {
    return ((op == RV64FPOP5::CVTI2F) || (op == RV64FPOP5::MVI2F));
}
inline constexpr bool rv64_fpop_has_s2(RV64FPOP5 op) {
    return ((op == RV64FPOP5::ADD) || (op == RV64FPOP5::SUB) ||
    (op == RV64FPOP5::MUL) || (op == RV64FPOP5::DIV) ||
    (op == RV64FPOP5::SGNJ) || (op == RV64FPOP5::MIN) ||
    (op == RV64FPOP5::CMP)
    );
}

constexpr uint32_t FFLAG_INEXACT   = 0x01;
constexpr uint32_t FFLAG_UNDERFLOW = 0x02;
constexpr uint32_t FFLAG_OVERFLOW  = 0x04;
constexpr uint32_t FFLAG_DIVBYZERO = 0x08;
constexpr uint32_t FFLAG_INVALID   = 0x10;

constexpr uint8_t FRM_RNE = 0; // Round to Nearest, ties to Even
constexpr uint8_t FRM_RTZ = 1; // Round towards Zero
constexpr uint8_t FRM_RDN = 2; // Round Down (towards -infinity)
constexpr uint8_t FRM_RUP = 3; // Round Up (towards +infinity)
constexpr uint8_t FRM_RMM = 4; // Round to Nearest, ties to Max Magnitude
constexpr uint8_t FRM_DYN = 7; // Dynamic rounding mode (in fcsr)

constexpr uint8_t getFRM(const uint64_t &fcsr) {
    return (fcsr >> 5) & 0x7;
}

constexpr uint32_t FCLASS_NEGINF   = (1U << 0);
constexpr uint32_t FCLASS_NEGNORM  = (1U << 1);
constexpr uint32_t FCLASS_NEGSUBNORM = (1U << 2);
constexpr uint32_t FCLASS_NEGZERO  = (1U << 3);
constexpr uint32_t FCLASS_POSZERO  = (1U << 4);
constexpr uint32_t FCLASS_POSSUBNORM = (1U << 5);
constexpr uint32_t FCLASS_POSNORM  = (1U << 6);
constexpr uint32_t FCLASS_POSINF   = (1U << 7);
constexpr uint32_t FCLASS_SNAN     = (1U << 8);
constexpr uint32_t FCLASS_QNAN     = (1U << 9);




} // namespace rv64archsem
