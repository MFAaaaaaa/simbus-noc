
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

#include "intcast.hpp"
#include "rvf/bitpack.hpp"
#include "softfloat/softfloat.h"
#include "softfloat/internals.h"

#include <bit>

using std::bit_cast;

namespace rv64archsem {

inline uint64_t nan_box_float_result(uint64_t value, uint32_t fp_width) {
    switch (fp_width)
    {
    case 0: // fp32
        return value | 0xffffffff00000000ULL;
    case 2: // fp16
        return value | 0xffffffffffff0000ULL;
    default:
        return value;
    }
}
inline void prepare_softfloat(const DecodedInst& inst, uint64_t &fcsr) {
    uint8_t rmmod = inst.funct3;
    if (rmmod == FRM_DYN) {
        rmmod = getFRM(fcsr);
    }
    if (rmmod > 4) {
        rmmod = FRM_RNE;
    }
    // uint8_t rmmod = getFRM(fcsr);
    // if (rmmod == FRM_DYN) rmmod = inst.funct3;
    // if (rmmod > 4) rmmod = FRM_RNE;
    softfloat_roundingMode = rmmod;
    softfloat_exceptionFlags = 0;
}

inline uint64_t fmuladd_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint_fast8_t op) {
    switch (inst.funct7 & 3)
    {
    case 0:
        return softfloat_mulAddF32(static_cast<uint32_t>(rs1), static_cast<uint32_t>(rs2), static_cast<uint32_t>(rs3), op).v;
    case 1:
        return softfloat_mulAddF64(rs1, rs2, rs3, op).v;
    case 2:
        return softfloat_mulAddF16(static_cast<uint16_t>(rs1), static_cast<uint16_t>(rs2), static_cast<uint16_t>(rs3), op).v;
    }
    return 0;
}

inline void commit_softfloat_flags(uint64_t &fcsr) {
    fcsr |= (static_cast<uint64_t>(softfloat_exceptionFlags) & 0x1f);
}

// uint64_t exec_madd(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr) {
//     prepare_softfloat(inst, fcsr);
//     uint64_t res = 0;
//     switch (inst.funct7 & 3)
//     {
//     case 0: // fp32 
//         return f32_mulAdd({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}, {static_cast<uint32_t>(rs3)}).v;
//     case 1: // fp64
//         return f64_mulAdd({rs1}, {rs2}, {rs3}).v;
//     case 2: // fp16
//         return f16_mulAdd({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}, {static_cast<uint16_t>(rs3)}).v;
//     }
//     // return 0;
//     commit_softfloat_flags(fcsr);
//     return nan_box_float_result(res, inst.funct7 & 3);
// }
// uint64_t exec_msub(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr) {
//     prepare_softfloat(inst, fcsr);
//     uint64_t res = 0;
//     switch (inst.funct7 & 3)
//     {
//     case 0: // fp32 
//         return f32_mulAdd({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}, {static_cast<uint32_t>(rs3) ^ 0x80000000U}).v;
//     case 1: // fp64
//         return f64_mulAdd({rs1}, {rs2}, {rs3 ^ 0x8000000000000000ULL}).v;
//     case 2: // fp16
//         return f16_mulAdd({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}, {static_cast<uint16_t>(rs3 ^ 0x8000U)}).v;
//     }
//     // return 0;
//     commit_softfloat_flags(fcsr);
//     return nan_box_float_result(res, inst.funct7 & 3);
// }
// uint64_t exec_nmsub(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr) {
//     prepare_softfloat(inst, fcsr);
//     uint64_t res = 0;
//     switch (inst.funct7 & 3)
//     {
//     case 0: // fp32 
//         return f32_mulAdd({static_cast<uint32_t>(rs1) ^ 0x80000000U}, {static_cast<uint32_t>(rs2)}, {static_cast<uint32_t>(rs3)}).v;
//     case 1: // fp64
//         return f64_mulAdd({rs1 ^ 0x8000000000000000ULL}, {rs2}, {rs3}).v;
//     case 2: // fp16
//         return f16_mulAdd({static_cast<uint16_t>(rs1 ^ 0x8000U)}, {static_cast<uint16_t>(rs2)}, {static_cast<uint16_t>(rs3)}).v;
//     }
//     // return 0;
//     commit_softfloat_flags(fcsr);
//     return nan_box_float_result(res, inst.funct7 & 3);
// }
uint64_t exec_madd(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr) {
    prepare_softfloat(inst, fcsr);
    uint64_t res = fmuladd_impl(inst, rs1, rs2, rs3, 0);
    commit_softfloat_flags(fcsr);
    return nan_box_float_result(res, inst.funct7 & 3);
}

uint64_t exec_msub(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr) {
    prepare_softfloat(inst, fcsr);
    uint64_t res = fmuladd_impl(inst, rs1, rs2, rs3, softfloat_mulAdd_subC);
    commit_softfloat_flags(fcsr);
    return nan_box_float_result(res, inst.funct7 & 3);
}

uint64_t exec_nmsub(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr) {
    prepare_softfloat(inst, fcsr);
    uint64_t res = fmuladd_impl(inst, rs1, rs2, rs3, softfloat_mulAdd_subProd);
    commit_softfloat_flags(fcsr);
    return nan_box_float_result(res, inst.funct7 & 3);
}
// uint64_t exec_nmadd(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr) {
//     prepare_softfloat(inst, fcsr);
//     uint64_t res = 0;
//     switch (inst.funct7 & 3)
//     {
//     case 0: // fp32 
//         return f32_mulAdd({static_cast<uint32_t>(rs1) ^ 0x80000000U}, {static_cast<uint32_t>(rs2)}, {static_cast<uint32_t>(rs3) ^ 0x80000000U}).v;
//     case 1: // fp64
//         return f64_mulAdd({rs1 ^ 0x8000000000000000ULL}, {rs2}, {rs3 ^ 0x8000000000000000ULL}).v;
//     case 2: // fp16
//         return f16_mulAdd({static_cast<uint16_t>(rs1 ^ 0x8000U)}, {static_cast<uint16_t>(rs2)}, {static_cast<uint16_t>(rs3 ^ 0x8000U)}).v;
//     }
//     // return 0;
//     commit_softfloat_flags(fcsr);
//     return nan_box_float_result(res, inst.funct7 & 3);
// }
uint64_t exec_nmadd(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, const uint64_t &rs3, uint64_t &fcsr) {
    prepare_softfloat(inst, fcsr);
    uint64_t res = 0;
    switch (inst.funct7 & 3)
    {
    case 0:
        res = fmuladd_impl(inst, rs1, rs2, static_cast<uint32_t>(rs3) ^ 0x80000000U, softfloat_mulAdd_subProd);
        break;
    case 1:
        res = fmuladd_impl(inst, rs1, rs2, rs3 ^ 0x8000000000000000ULL, softfloat_mulAdd_subProd);
        break;
    case 2:
        res = fmuladd_impl(inst, rs1, rs2, static_cast<uint16_t>(rs3) ^ 0x8000U, softfloat_mulAdd_subProd);
        break;
    }
    commit_softfloat_flags(fcsr);
    return nan_box_float_result(res, inst.funct7 & 3);
}

inline uint64_t fadd_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    switch (inst.funct7 & 3)
    {
    case 0: // fp32 
        return f32_add({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}).v;
    case 1: // fp64
        return f64_add({rs1}, {rs2}).v;
    case 2: // fp16
        return f16_add({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}).v;
    }
    return 0;
}
inline uint64_t fsub_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    switch (inst.funct7 & 3)
    {
    case 0: // fp32 
        return f32_add({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2) ^ 0x80000000U}).v;
    case 1: // fp64
        return f64_add({rs1}, {rs2 ^ 0x8000000000000000ULL}).v;
    case 2: // fp16
        return f16_add({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2 ^ 0x8000U)}).v;
    }
    return 0;
}
inline uint64_t fmul_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    switch (inst.funct7 & 3)
    {
    case 0: // fp32 
        return f32_mul({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}).v;
    case 1: // fp64
        return f64_mul({rs1}, {rs2}).v;
    case 2: // fp16
        return f16_mul({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}).v;
    }
    return 0;
}
inline uint64_t fdiv_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    switch (inst.funct7 & 3)
    {
    case 0: // fp32 
        return f32_div({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}).v;
    case 1: // fp64
        return f64_div({rs1}, {rs2}).v;
    case 2: // fp16
        return f16_div({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}).v;
    }
    return 0;
}
inline uint64_t fsqrt_impl(const DecodedInst& inst, const uint64_t &rs1) {
    switch (inst.funct7 & 3)
    {
    case 0: // fp32 
        return f32_sqrt({static_cast<uint32_t>(rs1)}).v;
    case 1: // fp64
        return f64_sqrt({rs1}).v;
    case 2: // fp16
        return f16_sqrt({static_cast<uint16_t>(rs1)}).v;
    }
    return 0;
}
inline uint64_t fsgnj_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    switch (inst.funct7 & 3)
    {
    case 0: // fp32 
        {
            switch (inst.funct3)
            {
            case 0: // SGNJ
                return (rs1 & 0x7FFFFFFF) | (rs2 & 0x80000000);
            case 1: // SGNJN
                return (rs1 & 0x7FFFFFFF) | (~rs2 & 0x80000000);
            case 2: // SGNJX
                return (rs1 & 0x7FFFFFFF) | ((rs1 ^ rs2) & 0x80000000);
            }
        } break;
    case 1: // fp64
        {
            switch (inst.funct3)
            {
            case 0: // SGNJ
                return (rs1 & 0x7FFFFFFFFFFFFFFF) | (rs2 & 0x8000000000000000ULL);
            case 1: // SGNJN
                return (rs1 & 0x7FFFFFFFFFFFFFFF) | (~rs2 & 0x8000000000000000ULL);
            case 2: // SGNJX
                return (rs1 & 0x7FFFFFFFFFFFFFFF) | ((rs1 ^ rs2) & 0x8000000000000000ULL);
            }
        } break;
    case 2: // fp16
        {
            switch (inst.funct3)
            {
            case 0: // SGNJ
                return (rs1 & 0x7FFF) | (rs2 & 0x8000);
            case 1: // SGNJN
                return (rs1 & 0x7FFF) | (~rs2 & 0x8000);
            case 2: // SGNJX
                return (rs1 & 0x7FFF) | ((rs1 ^ rs2) & 0x8000);
            }
        } break;
    }
    return 0;
}
inline uint64_t fmin_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    switch (inst.funct7 & 3)
    {
    case 0: // fp32 
        {
            switch (inst.funct3)
            {
            case 0: // MIN
                return f32_min({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}).v;
            case 1: // MAX
                return f32_max({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}).v;
            }
        } break;
    case 1: // fp64
        {
            switch (inst.funct3)
            {
            case 0: // MIN
                return f64_min({rs1}, {rs2}).v;
            case 1: // MAX
                return f64_max({rs1}, {rs2}).v;
            }
        } break;
    case 2: // fp16
        {
            switch (inst.funct3)
            {
            case 0: // MIN
                return f16_min({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}).v;
            case 1: // MAX
                return f16_max({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}).v;
            }
        } break;
    }
    return 0;
}
inline uint64_t fmvf2f_impl(const DecodedInst& inst, const uint64_t &rs1) {
    uint32_t to_width = inst.funct7 & 3;
    uint32_t from_width = inst.rs2 & 3;
    uint32_t sel = (from_width << 2) | to_width;

    // uint32_t from_width = inst.funct7 & 3;
    // uint32_t to_width = inst.rs2 & 3;
    // uint32_t sel = (from_width << 2) | to_width;
    switch (sel)
    {
    case 0b0000: // f32 to f32
        return static_cast<uint64_t>(static_cast<uint32_t>(rs1));
    case 0b0001: // f32 to f64
        return f32_to_f64({static_cast<uint32_t>(rs1)}).v;
    case 0b0010: // f32 to f16
        return f32_to_f16({static_cast<uint32_t>(rs1)}).v;
    case 0b0100: // f64 to f32
        return f64_to_f32({rs1}).v;
    case 0b0101: // f64 to f64
        return rs1;
    case 0b0110: // f64 to f16
        return f64_to_f16({rs1}).v;
    case 0b1000: // f16 to f32
        return f16_to_f32({static_cast<uint16_t>(rs1)}).v;
    case 0b1001: // f16 to f64
        return f16_to_f64({static_cast<uint16_t>(rs1)}).v;
    case 0b1010: // f16 to f16
        return static_cast<uint64_t>(static_cast<uint16_t>(rs1));
    }
    return 0;
}
inline uint64_t fcmp_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
    switch (inst.funct7 & 3)
    {
    case 0: // fp32
        {
            switch (inst.funct3)
            {
            case 2: // FEQ
                return f32_eq({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}) ? 1 : 0;
            case 1: // FLT
                return f32_lt({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}) ? 1 : 0;
            case 0: // FLE
                return f32_le({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}) ? 1 : 0;
            }
        } break;

    case 1: // fp64
        {
            switch (inst.funct3)
            {
            case 2: // FEQ
                return f64_eq({rs1}, {rs2}) ? 1 : 0;
            case 1: // FLT
                return f64_lt({rs1}, {rs2}) ? 1 : 0;
            case 0: // FLE
                return f64_le({rs1}, {rs2}) ? 1 : 0;
            }
        } break;

    case 2: // fp16
        {
            switch (inst.funct3)
            {
            case 2: // FEQ
                return f16_eq({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}) ? 1 : 0;
            case 1: // FLT
                return f16_lt({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}) ? 1 : 0;
            case 0: // FLE
                return f16_le({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}) ? 1 : 0;
            }
        } break;
    }
    return 0;
}
// inline uint64_t fcmp_impl(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2) {
//     switch (inst.funct7 & 3)
//     {
//     case 0: // fp32 
//         {
//             switch (inst.funct3)
//             {
//             case 0: // FEQ
//                 return f32_eq({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}) ? 1 : 0;
//             case 1: // FLT
//                 return f32_lt({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}) ? 1 : 0;
//             case 2: // FLE
//                 return f32_le({static_cast<uint32_t>(rs1)}, {static_cast<uint32_t>(rs2)}) ? 1 : 0;
//             }
//         } break;
//     case 1: // fp64
//         {
//             switch (inst.funct3)
//             {
//             case 0: // FEQ
//                 return f64_eq({rs1}, {rs2}) ? 1 : 0;
//             case 1: // FLT
//                 return f64_lt({rs1}, {rs2}) ? 1 : 0;
//             case 2: // FLE
//                 return f64_le({rs1}, {rs2}) ? 1 : 0;
//             }
//         } break;
//     case 2: // fp16
//         {
//             switch (inst.funct3)
//             {
//             case 0: // FEQ
//                 return f16_eq({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}) ? 1 : 0;
//             case 1: // FLT
//                 return f16_lt({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}) ? 1 : 0;
//             case 2: // FLE
//                 return f16_le({static_cast<uint16_t>(rs1)}, {static_cast<uint16_t>(rs2)}) ? 1 : 0;
//             }
//         } break;
//     }
//     return 0;
// }
inline uint64_t fcvtf2i_impl(const DecodedInst& inst, const uint64_t &rs1, uint8_t rm) {
    uint32_t from_width = inst.funct7 & 3;
    uint32_t to_width = inst.rs2 & 3;
    uint32_t sel = (from_width << 2) | to_width;
    switch (sel)
    {
    case 0b0000: // f32 to int32
        return static_cast<uint64_t>(static_cast<int64_t>(f32_to_i32({static_cast<uint32_t>(rs1)}, rm, true)));
    // case 0b0001: // f32 to uint32
    //     return static_cast<uint64_t>(f32_to_ui32({static_cast<uint32_t>(rs1)}, rm, true));
    case 0b0001: // f32 to uint32
        return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(static_cast<uint32_t>(f32_to_ui32({static_cast<uint32_t>(rs1)}, rm, true)))));
    case 0b0010: // f32 to int64
        return static_cast<uint64_t>(static_cast<int64_t>(f32_to_i64({static_cast<uint32_t>(rs1)}, rm, true)));
    case 0b0011: // f32 to uint64
        return static_cast<uint64_t>(f32_to_ui64({static_cast<uint32_t>(rs1)}, rm, true));
    case 0b0100: // f64 to int32
        return static_cast<uint64_t>(static_cast<int64_t>(f64_to_i32({rs1}, rm, true)));
    // case 0b0101: // f64 to uint32
    //     return static_cast<uint64_t>(f64_to_ui32({rs1}, rm, true));
    case 0b0101: // f64 to uint32
        return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(static_cast<uint32_t>(f64_to_ui32({rs1}, rm, true)))));
    case 0b0110: // f64 to int64
        return static_cast<uint64_t>(static_cast<int64_t>(f64_to_i64({rs1}, rm, true)));
    case 0b0111: // f64 to uint64
        return static_cast<uint64_t>(f64_to_ui64({rs1}, rm, true));
    case 0b1000: // f16 to int32
        return static_cast<uint64_t>(static_cast<int64_t>(f16_to_i32({static_cast<uint16_t>(rs1)}, rm, true)));
    // case 0b1001: // f16 to uint32
    //     return static_cast<uint64_t>(f16_to_ui32({static_cast<uint16_t>(rs1)}, rm, true));
    case 0b1001: // f16 to uint32
        return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(static_cast<uint32_t>(f16_to_ui32({static_cast<uint16_t>(rs1)}, rm, true)))));
    case 0b1010: // f16 to int64
        return static_cast<uint64_t>(static_cast<int64_t>(f16_to_i64({static_cast<uint16_t>(rs1)}, rm, true)));
    case 0b1011: // f16 to uint64
        return static_cast<uint64_t>(f16_to_ui64({static_cast<uint16_t>(rs1)}, rm, true));
    }
    return 0;
}
inline uint64_t fcvti2f_impl(const DecodedInst& inst, const uint64_t &rs1) {
    uint32_t from_width = inst.rs2 & 3;
    uint32_t to_width = inst.funct7 & 3;
    uint32_t sel = (from_width << 2) | to_width;
    switch (sel)
    {
    case 0b0000: // int32 to f32
        return i32_to_f32(int_cast<int32_t, uint64_t>(rs1)).v;
    case 0b0100: // uint32 to f32
        return ui32_to_f32(int_cast<uint32_t, uint64_t>(rs1)).v;
    case 0b1000: // int64 to f32
        return i64_to_f32(int_cast<int64_t, uint64_t>(rs1)).v;
    case 0b1100: // uint64 to f32
        return ui64_to_f32(rs1).v;
    case 0b0001: // int32 to f64
        return i32_to_f64(int_cast<int32_t, uint64_t>(rs1)).v;
    case 0b0101: // uint32 to f64
        return ui32_to_f64(int_cast<uint32_t, uint64_t>(rs1)).v;
    case 0b1001: // int64 to f64
        return i64_to_f64(int_cast<int64_t, uint64_t>(rs1)).v;
    case 0b1101: // uint64 to f64
        return ui64_to_f64(rs1).v;
    case 0b0010: // int32 to f16
        return i32_to_f16(int_cast<int32_t, uint64_t>(rs1)).v;
    case 0b0110: // uint32 to f16
        return ui32_to_f16(int_cast<uint32_t, uint64_t>(rs1)).v;
    case 0b1010: // int64 to f16
        return i64_to_f16(int_cast<int64_t, uint64_t>(rs1)).v;
    case 0b1110: // uint64 to f16
        return ui64_to_f16(rs1).v;
    }
    return 0;
}
inline uint64_t fmvf2i_impl(const DecodedInst& inst, const uint64_t &rs1) {
    uint32_t fwid = inst.funct7 & 3;
    if (inst.funct3 == 1) {
        // fclass
        switch (fwid)
        {
        case 0: // f32
            return f32_classify({static_cast<uint32_t>(rs1)});
        case 1: // f64
            return f64_classify({rs1});
        case 2: // f16
            return f16_classify({static_cast<uint16_t>(rs1)});
        }
    } else {
        switch (fwid)
        {
        case 0: // f32
            return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(static_cast<uint32_t>(rs1))));
        case 1: // f64
            return rs1;
        case 2: // f16
            return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int16_t>(static_cast<uint16_t>(rs1))));

        // case 0: // f32
        //     return static_cast<uint64_t>(static_cast<uint32_t>(rs1));
        // case 1: // f64
        //     return rs1;
        // case 2: // f16
        //     return static_cast<uint64_t>(static_cast<uint16_t>(rs1));
        }
    }
    return 0;
}
inline uint64_t fmvi2f_impl(const DecodedInst& inst, const uint64_t &rs1) {
    uint32_t fwid = inst.funct7 & 3;
    switch (fwid)
    {
    case 0: // f32
        return static_cast<uint64_t>(static_cast<uint32_t>(rs1));
    case 1: // f64
        return rs1;
    case 2: // f16
        return static_cast<uint64_t>(static_cast<uint16_t>(rs1));
    }
    return 0;
}

uint64_t exec_opfp(const DecodedInst& inst, const uint64_t &rs1, const uint64_t &rs2, uint64_t &fcsr) {
    uint8_t rmmod = inst.funct3;
    if (rmmod == FRM_DYN) {
        rmmod = getFRM(fcsr);
    }
    if (rmmod > 4) {
        rmmod = FRM_RNE;
    }
    // uint8_t rmmod = getFRM(fcsr);
    // if (rmmod == FRM_DYN) {
    //     rmmod = inst.funct3;
    // }
    // if (rmmod > 4) {
    //     rmmod = FRM_RNE;
    // }
    softfloat_roundingMode = rmmod;
    softfloat_exceptionFlags = 0;
    uint64_t res = 0;

    switch (static_cast<RV64FPOP5>(inst.funct7 >> 2))
    {
    case RV64FPOP5::ADD: res = fadd_impl(inst, rs1, rs2); break;
    case RV64FPOP5::SUB: res = fsub_impl(inst, rs1, rs2); break;
    case RV64FPOP5::MUL: res = fmul_impl(inst, rs1, rs2); break;
    case RV64FPOP5::DIV: res = fdiv_impl(inst, rs1, rs2); break;
    case RV64FPOP5::SQRT: res = fsqrt_impl(inst, rs1); break;
    case RV64FPOP5::SGNJ: res = fsgnj_impl(inst, rs1, rs2); break;
    case RV64FPOP5::MIN: res = fmin_impl(inst, rs1, rs2); break;
    case RV64FPOP5::MVF2F: res = fmvf2f_impl(inst, rs1); break;
    case RV64FPOP5::CMP: res = fcmp_impl(inst, rs1, rs2); break;
    case RV64FPOP5::CVTF2I: res = fcvtf2i_impl(inst, rs1, rmmod); break;
    case RV64FPOP5::CVTI2F: res = fcvti2f_impl(inst, rs1); break;
    case RV64FPOP5::MVF2I: res = fmvf2i_impl(inst, rs1); break;
    case RV64FPOP5::MVI2F: res = fmvi2f_impl(inst, rs1); break;
    default:
        return 0;
    }
    fcsr |= (static_cast<uint64_t>(softfloat_exceptionFlags) & 0x1f);
    
    switch (static_cast<RV64FPOP5>(inst.funct7 >> 2))
    {
    case RV64FPOP5::ADD:
    case RV64FPOP5::SUB:
    case RV64FPOP5::MUL:
    case RV64FPOP5::DIV:
    case RV64FPOP5::SQRT:
    case RV64FPOP5::SGNJ:
    case RV64FPOP5::MIN:
    case RV64FPOP5::MVF2F:
    case RV64FPOP5::CVTI2F:
    case RV64FPOP5::MVI2F:
        if (inst.rdType == RegType::freg) {
            res = nan_box_float_result(res, inst.funct7 & 3);
        }
        break;
    default:
        break;
    }

    return res;
}



} // namespace rv64archsem
