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

#include "vcommon.hpp"
#include "softfloat/softfloat.h"
#include "intcast.hpp"

#include <cstdint>
#include <type_traits>

namespace rvvarchsem {

inline void vfp_clear_exceptions() {
    softfloat_exceptionFlags = 0;
}
inline uint8_t vfp_get_exceptions() {
    return static_cast<uint8_t>(softfloat_exceptionFlags);
}
inline void vfp_set_rounding_mode(const uint8_t & rm) {
    softfloat_roundingMode = (rm <= 5) ? rm : 0;
}

template <typename T, typename OP>
void vfop_common_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = op(vs2[i], vs1[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = op(vs2[i], vs1[i]);
            }
        }
    }
}
template <typename T, typename OP>
void vfop_common_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = op(vs2[i], rs1);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = op(vs2[i], rs1);
            }
        }
    }
}

template <typename T, typename OP>
void vfop1_common_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = op(vs2[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = op(vs2[i]);
            }
        }
    }
}


// 13.2. Vector Single-Width Floating-Point Add/Subtract Instructions

template <typename T>
T fadd_impl(const T & a, const T &b) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_add({a}, {b}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_add({a}, {b}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_add({a}, {b}).v);
    }
    return T(0);
}

template <typename T>
T fsub_impl(const T & a, const T &b) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_sub({a}, {b}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_sub({a}, {b}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_sub({a}, {b}).v);
    }
    return T(0);
}

template <typename T, typename T2>
T2 fcvt_impl(const T & a) {
    if constexpr (sizeof(T) == 2 && sizeof(T2) == 4) {
        return static_cast<T2>(f16_to_f32({a}).v);
    } else if constexpr (sizeof(T) == 2 && sizeof(T2) == 8) {
        return static_cast<T2>(f16_to_f64({a}).v);
    } else if constexpr (sizeof(T) == 4 && sizeof(T2) == 2) {
        return static_cast<T2>(f32_to_f16({a}).v);
    } else if constexpr (sizeof(T) == 4 && sizeof(T2) == 8) {
        return static_cast<T2>(f32_to_f64({a}).v);
    } else if constexpr (sizeof(T) == 8 && sizeof(T2) == 2) {
        return static_cast<T2>(f64_to_f16({a}).v);
    } else if constexpr (sizeof(T) == 8 && sizeof(T2) == 4) {
        return static_cast<T2>(f64_to_f32({a}).v);
    } else {
        return static_cast<T2>(a);
    }
}

template <typename T>
void vfadd_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fadd_impl<T>);
}

template <typename T>
void vfadd_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fadd_impl<T>);
}

template <typename T>
void vfsub_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fsub_impl<T>);
}

template <typename T>
void vfsub_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fsub_impl<T>);
}

template <typename T>
void vfrsub_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b) { return fsub_impl<T>(b, a); });
}

// 13.3. Vector Widening Floating-Point Add/Subtract Instructions

template <typename T, typename T2>
void vfwadd_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fadd_impl<T2>(fcvt_impl<T, T2>(vs2[i]), fcvt_impl<T, T2>(vs1[i]));
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fadd_impl<T2>(fcvt_impl<T, T2>(vs2[i]), fcvt_impl<T, T2>(vs1[i]));
            }
        }
    }
}

template <typename T, typename T2>
void vfwadd_vf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    T2 rs1_wide = fcvt_impl<T, T2>(rs1);
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fadd_impl<T2>(fcvt_impl<T, T2>(vs2[i]), rs1_wide);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fadd_impl<T2>(fcvt_impl<T, T2>(vs2[i]), rs1_wide);
            }
        }
    }
}

template <typename T, typename T2>
void vfwsub_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fsub_impl<T2>(fcvt_impl<T, T2>(vs2[i]), fcvt_impl<T, T2>(vs1[i]));
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fsub_impl<T2>(fcvt_impl<T, T2>(vs2[i]), fcvt_impl<T, T2>(vs1[i]));
            }
        }
    }
}

template <typename T, typename T2>
void vfwsub_vf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    T2 rs1_wide = fcvt_impl<T, T2>(rs1);
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fsub_impl<T2>(fcvt_impl<T, T2>(vs2[i]), rs1_wide);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fsub_impl<T2>(fcvt_impl<T, T2>(vs2[i]), rs1_wide);
            }
        }
    }
}

template <typename T, typename T2>
void vfwadd_wv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T2* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fadd_impl<T2>(vs2[i], fcvt_impl<T, T2>(vs1[i]));
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fadd_impl<T2>(vs2[i], fcvt_impl<T, T2>(vs1[i]));
            }
        }
    }
}

template <typename T, typename T2>
void vfwadd_wf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T2* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    T2 rs1_wide = fcvt_impl<T, T2>(rs1);
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fadd_impl<T2>((vs2[i]), rs1_wide);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fadd_impl<T2>((vs2[i]), rs1_wide);
            }
        }
    }
}

template <typename T, typename T2>
void vfwsub_wv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T2* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fsub_impl<T2>((vs2[i]), fcvt_impl<T, T2>(vs1[i]));
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fsub_impl<T2>((vs2[i]), fcvt_impl<T, T2>(vs1[i]));
            }
        }
    }
}

template <typename T, typename T2>
void vfwsub_wf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T2* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    T2 rs1_wide = fcvt_impl<T, T2>(rs1);
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fsub_impl<T2>((vs2[i]), rs1_wide);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fsub_impl<T2>((vs2[i]), rs1_wide);
            }
        }
    }
}

// 13.4. Vector Single-Width Floating-Point Multiply/Divide Instructions

template <typename T>
T fmul_impl(const T & a, const T &b) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_mul({a}, {b}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_mul({a}, {b}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_mul({a}, {b}).v);
    }
    return T(0);
}

template <typename T>
T fdiv_impl(const T & a, const T &b) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_div({a}, {b}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_div({a}, {b}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_div({a}, {b}).v);
    }
    return T(0);
}

template <typename T>
void vfmul_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fmul_impl<T>);
}

template <typename T>
void vfmul_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fmul_impl<T>);
}

template <typename T>
void vfdiv_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fdiv_impl<T>);
}

template <typename T>
void vfdiv_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fdiv_impl<T>);
}

template <typename T>
void vfrdiv_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b) { return fdiv_impl<T>(b, a); });
}

// 13.5. Vector Widening Floating-Point Multiply

template <typename T, typename T2>
void vfwmul_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fmul_impl<T2>(fcvt_impl<T, T2>(vs2[i]), fcvt_impl<T, T2>(vs1[i]));
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fmul_impl<T2>(fcvt_impl<T, T2>(vs2[i]), fcvt_impl<T, T2>(vs1[i]));
            }
        }
    }
}

template <typename T, typename T2>
void vfwmul_vf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    T2 rs1_wide = fcvt_impl<T, T2>(rs1);
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fmul_impl<T2>(fcvt_impl<T, T2>(vs2[i]), rs1_wide);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fmul_impl<T2>(fcvt_impl<T, T2>(vs2[i]), rs1_wide);
            }
        }
    }
}

// 13.6. Vector Single-Width Floating-Point Fused Multiply-Add Instructions

template <typename T>
T fneg_impl(const T & a) {
    using UT = typename _uint_of_size<T>::_type;
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(static_cast<UT>(a) ^ 0x8000U);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(static_cast<UT>(a) ^ 0x80000000U);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(static_cast<UT>(a) ^ 0x8000000000000000ULL);
    }
    return a;
}

template <typename T>
T fmadd_impl(const T & a, const T & b, const T & c) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_mulAdd({a}, {b}, {c}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_mulAdd({a}, {b}, {c}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_mulAdd({a}, {b}, {c}).v);
    }
    return T(0);
}

template <typename T, typename OP>
void vfmadd_common_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = op(vs1[i], vs2[i], vd[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = op(vs1[i], vs2[i], vd[i]);
            }
        }
    }
}

template <typename T, typename OP>
void vfmadd_common_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = op(rs1, vs2[i], vd[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = op(rs1, vs2[i], vd[i]);
            }
        }
    }
}

template <typename T>
void vfmacc_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fmadd_impl<T>);
}

template <typename T>
void vfmacc_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fmadd_impl<T>);
}

template <typename T>
void vfnmacc_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fneg_impl<T>(fmadd_impl<T>(a, b, c)); });
}

template <typename T>
void vfnmacc_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fneg_impl<T>(fmadd_impl<T>(a, b, c)); });
}

template <typename T>
void vfmsac_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(a, b, fneg_impl<T>(c)); });
}

template <typename T>
void vfmsac_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(a, b, fneg_impl<T>(c)); });
}

template <typename T>
void vfnmsac_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(fneg_impl<T>(a), b, c); });
}

template <typename T>
void vfnmsac_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(fneg_impl<T>(a), b, c); });
}

template <typename T>
void vfmadd_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(a, c, b); });
}

template <typename T>
void vfmadd_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(a, c, b); });
}

template <typename T>
void vfnmadd_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fneg_impl<T>(fmadd_impl<T>(a, c, b)); });
}

template <typename T>
void vfnmadd_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fneg_impl<T>(fmadd_impl<T>(a, c, b)); });
}

template <typename T>
void vfmsub_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(a, c, fneg_impl<T>(b)); });
}

template <typename T>
void vfmsub_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(a, c, fneg_impl<T>(b)); });
}

template <typename T>
void vfnmsub_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(fneg_impl<T>(a), c, b); });
}

template <typename T>
void vfnmsub_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfmadd_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b, const T & c) -> T { return fmadd_impl<T>(fneg_impl<T>(a), c, b); });
}

// 13.7. Vector Widening Floating-Point Fused Multiply-Add Instructions

template <typename T, typename T2, typename OP>
void vfwmadd_common_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = op(fcvt_impl<T, T2>(vs1[i]), fcvt_impl<T, T2>(vs2[i]), vd[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = op(fcvt_impl<T, T2>(vs1[i]), fcvt_impl<T, T2>(vs2[i]), vd[i]);
            }
        }
    }
}

template <typename T, typename T2, typename OP>
void vfwmadd_common_vf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    T2 rs1_wide = fcvt_impl<T, T2>(rs1);
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = op(rs1_wide, fcvt_impl<T, T2>(vs2[i]), vd[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = op(rs1_wide, fcvt_impl<T, T2>(vs2[i]), vd[i]);
            }
        }
    }
}

template <typename T, typename T2>
void vfwmacc_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwmadd_common_vv<T, T2>(vl, vstart, vd, vs2, vs1, mask, nomask, fmadd_impl<T2>);
}

template <typename T, typename T2>
void vfwmacc_vf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwmadd_common_vf<T, T2>(vl, vstart, vd, vs2, rs1, mask, nomask, fmadd_impl<T2>);
}

template <typename T, typename T2>
void vfwnmacc_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwmadd_common_vv<T, T2>(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T2 & a, const T2 & b, const T2 & c) -> T2 { return fneg_impl<T2>(fmadd_impl<T2>(a, b, c)); });
}

template <typename T, typename T2>
void vfwnmacc_vf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwmadd_common_vf<T, T2>(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T2 & a, const T2 & b, const T2 & c) -> T2 { return fneg_impl<T2>(fmadd_impl<T2>(a, b, c)); });
}

template <typename T, typename T2>
void vfwmsac_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwmadd_common_vv<T, T2>(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T2 & a, const T2 & b, const T2 & c) -> T2 { return fmadd_impl<T2>(a, b, fneg_impl<T2>(c)); });
}

template <typename T, typename T2>
void vfwmsac_vf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwmadd_common_vf<T, T2>(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T2 & a, const T2 & b, const T2 & c) -> T2 { return fmadd_impl<T2>(a, b, fneg_impl<T2>(c)); });
}

template <typename T, typename T2>
void vfwnmsac_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwmadd_common_vv<T, T2>(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T2 & a, const T2 & b, const T2 & c) -> T2 { return fmadd_impl<T2>(fneg_impl<T2>(a), b, c); });
}

template <typename T, typename T2>
void vfwnmsac_vf(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwmadd_common_vf<T, T2>(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T2 & a, const T2 & b, const T2 & c) -> T2 { return fmadd_impl<T2>(fneg_impl<T2>(a), b, c); });
}

// 13.8. Vector Floating-Point Square-Root Instruction

template <typename T>
T fsqrt_impl(const T & a) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_sqrt({a}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_sqrt({a}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_sqrt({a}).v);
    }
    return T(0);
}

template <typename T>
void vfsqrt_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, fsqrt_impl<T>);
}

// 13.9. Vector Floating-Point Reciprocal Square-Root Estimate Instruction

template <typename T>
T frsqrt7_impl(const T & a) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_rsqrte7({a}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_rsqrte7({a}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_rsqrte7({a}).v);
    }
    return T(0);
}

template <typename T>
void vfrsqrt7_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, frsqrt7_impl<T>);
}

// 13.10. Vector Floating-Point Reciprocal Estimate Instruction

template <typename T>
T frecip7_impl(const T & a) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_recip7({a}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_recip7({a}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_recip7({a}).v);
    }
    return T(0);
}

template <typename T>
void vfrec7_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, frecip7_impl<T>);
}

// 13.11. Vector Floating-Point MIN/MAX Instructions

template <typename T>
T fmin_impl(const T & a, const T & b) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_min({a}, {b}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_min({a}, {b}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_min({a}, {b}).v);
    }
    return T(0);
}

template <typename T>
T fmax_impl(const T & a, const T & b) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_max({a}, {b}).v);
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_max({a}, {b}).v);
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_max({a}, {b}).v);
    }
    return T(0);
}

template <typename T>
void vfmin_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fmin_impl<T>);
}

template <typename T>
void vfmin_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fmin_impl<T>);
}

template <typename T>
void vfmax_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fmax_impl<T>);
}

template <typename T>
void vfmax_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fmax_impl<T>);
}

// 13.12. Vector Floating-Point Sign-Injection Instructions

template <typename T>
T fsgnj_impl(const T & a, const T & b) {
    using UT = typename _uint_of_size<T>::_type;
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFU) | (static_cast<UT>(b) & 0x8000U));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFFFFFU) | (static_cast<UT>(b) & 0x80000000U));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFFFFFFFFFFFFFULL) | (static_cast<UT>(b) & 0x8000000000000000ULL));
    }
    return a;
}
 
template <typename T>
T fsgnjn_impl(const T & a, const T & b) {
    using UT = typename _uint_of_size<T>::_type;
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFU) | (static_cast<UT>(~b) & 0x8000U));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFFFFFU) | (static_cast<UT>(~b) & 0x80000000U));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFFFFFFFFFFFFFULL) | (static_cast<UT>(~b) & 0x8000000000000000ULL));
    }
    return a;
}

template <typename T>
T fsgnjx_impl(const T & a, const T & b) {
    using UT = typename _uint_of_size<T>::_type;
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFU) | (static_cast<UT>(a^b) & 0x8000U));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFFFFFU) | (static_cast<UT>(a^b) & 0x80000000U));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>((static_cast<UT>(a) & 0x7FFFFFFFFFFFFFFFULL) | (static_cast<UT>(a^b) & 0x8000000000000000ULL));
    }
    return a;
}

template <typename T>
void vfsgnj_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fsgnj_impl<T>);
}

template <typename T>
void vfsgnj_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fsgnj_impl<T>);
}

template <typename T>
void vfsgnjn_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fsgnjn_impl<T>);
}

template <typename T>
void vfsgnjn_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fsgnjn_impl<T>);
}

template <typename T>
void vfsgnjx_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fsgnjx_impl<T>);
}

template <typename T>
void vfsgnjx_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fsgnjx_impl<T>);
}

// 13.13. Vector Floating-Point Compare Instructions

template <typename T>
bool feq_impl(const T& a, const T& b) {
    if constexpr (sizeof(T) == 2) {
        return f16_eq({a}, {b});
    } else if constexpr (sizeof(T) == 4) {
        return f32_eq({a}, {b});
    } else if constexpr (sizeof(T) == 8) {
        return f64_eq({a}, {b});
    }
    return false;
}

template <typename T>
bool flt_impl(const T& a, const T& b) {
    if constexpr (sizeof(T) == 2) {
        return f16_lt({a}, {b});
    } else if constexpr (sizeof(T) == 4) {
        return f32_lt({a}, {b});
    } else if constexpr (sizeof(T) == 8) {
        return f64_lt({a}, {b});
    }
    return false;
}

template <typename T>
bool fle_impl(const T& a, const T& b) {
    if constexpr (sizeof(T) == 2) {
        return f16_le({a}, {b});
    } else if constexpr (sizeof(T) == 4) {
        return f32_le({a}, {b});
    } else if constexpr (sizeof(T) == 8) {
        return f64_le({a}, {b});
    }
    return false;
}

template <typename T, typename OP>
void vfcmp_common_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            set_vmask(vd, i, op(vs2[i], vs1[i]));
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                set_vmask(vd, i, op(vs2[i], vs1[i]));
            }
        }
    }
}

template <typename T, typename OP>
void vfcmp_common_vf(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            set_vmask(vd, i, op(vs2[i], rs1));
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                set_vmask(vd, i, op(vs2[i], rs1));
            }
        }
    }
}

template <typename T>
void vmfeq_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, feq_impl<T>);
}

template <typename T>
void vmfeq_vf(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, feq_impl<T>);
}

template <typename T>
void vmfne_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, [](const T & a, const T & b) -> bool { return !feq_impl<T>(a, b); });
}

template <typename T>
void vmfne_vf(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b) -> bool { return !feq_impl<T>(a, b); });
}

template <typename T>
void vmflt_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, flt_impl<T>);
}

template <typename T>
void vmflt_vf(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, flt_impl<T>);
}

template <typename T>
void vmfle_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vv(vl, vstart, vd, vs2, vs1, mask, nomask, fle_impl<T>);
}

template <typename T>
void vmfle_vf(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, fle_impl<T>);
}

template <typename T>
void vmfgt_vf(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b) -> bool { return flt_impl<T>(b, a); });
}

template <typename T>
void vmfge_vf(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfcmp_common_vf(vl, vstart, vd, vs2, rs1, mask, nomask, [](const T & a, const T & b) -> bool { return fle_impl<T>(b, a); });
}

// 13.14. Vector Floating-Point Classify Instruction

template <typename T>
T fclass_impl(const T & a) {
    if constexpr (sizeof(T) == 2) {
        return static_cast<T>(f16_classify({a}));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(f32_classify({a}));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(f64_classify({a}));
    }
    return T(0);
}

template <typename T>
void vfclass_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = fclass_impl<T>(vs2[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = fclass_impl<T>(vs2[i]);
            }
        }
    }
}

// 13.15. Vector Floating-Point Merge Instruction
// Using integer vmerge

// 13.16. Vector Floating-Point Move Instruction
// Using integer vmv

// 13.17. Single-Width Floating-Point/Integer Type-Convert Instructions

template <typename FromT, typename ToT>
ToT fcvt_f2i_impl(const FromT& a) {
    constexpr size_t from_size = sizeof(FromT);
    constexpr size_t to_size = sizeof(ToT);
    constexpr bool to_signed = std::is_signed<ToT>::value;
    if constexpr (from_size == 2 && to_size == 1 && to_signed) {
        return int_cast<ToT>(f16_to_i8({int_cast<uint16_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 2 && to_size == 1 && !to_signed) {
        return int_cast<ToT>(f16_to_ui8({int_cast<uint16_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 2 && to_size == 2 && to_signed) {
        return int_cast<ToT>(f16_to_i16({int_cast<uint16_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 2 && to_size == 2 && !to_signed) {
        return int_cast<ToT>(f16_to_ui16({int_cast<uint16_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 2 && to_size == 4 && to_signed) {
        return int_cast<ToT>(f16_to_i32({int_cast<uint16_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 2 && to_size == 4 && !to_signed) {
        return int_cast<ToT>(f16_to_ui32({int_cast<uint16_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 2 && to_size == 8 && to_signed) {
        return int_cast<ToT>(f16_to_i64({int_cast<uint16_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 2 && to_size == 8 && !to_signed) {
        return int_cast<ToT>(f16_to_ui64({int_cast<uint16_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 4 && to_size == 1 && to_signed) {
        return int_cast<ToT>(f32_to_i8({int_cast<uint32_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 4 && to_size == 1 && !to_signed) {
        return int_cast<ToT>(f32_to_ui8({int_cast<uint32_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 4 && to_size == 2 && to_signed) {
        return int_cast<ToT>(f32_to_i16({int_cast<uint32_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 4 && to_size == 2 && !to_signed) {
        return int_cast<ToT>(f32_to_ui16({int_cast<uint32_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 4 && to_size == 4 && to_signed) {
        return int_cast<ToT>(f32_to_i32({int_cast<uint32_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 4 && to_size == 4 && !to_signed) {
        return int_cast<ToT>(f32_to_ui32({int_cast<uint32_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 4 && to_size == 8 && to_signed) {
        return int_cast<ToT>(f32_to_i64({int_cast<uint32_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 4 && to_size == 8 && !to_signed) {
        return int_cast<ToT>(f32_to_ui64({int_cast<uint32_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 8 && to_size == 1 && to_signed) {
        return int_cast<ToT>(f64_to_i8({int_cast<uint64_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 8 && to_size == 1 && !to_signed) {
        return int_cast<ToT>(f64_to_ui8({int_cast<uint64_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 8 && to_size == 2 && to_signed) {
        return int_cast<ToT>(f64_to_i16({int_cast<uint64_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 8 && to_size == 2 && !to_signed) {
        return int_cast<ToT>(f64_to_ui16({int_cast<uint64_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 8 && to_size == 4 && to_signed) {
        return int_cast<ToT>(f64_to_i32({int_cast<uint64_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 8 && to_size == 4 && !to_signed) {
        return int_cast<ToT>(f64_to_ui32({int_cast<uint64_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 8 && to_size == 8 && to_signed) {
        return int_cast<ToT>(f64_to_i64({int_cast<uint64_t>(a)}, softfloat_roundingMode, true));
    } else if constexpr (from_size == 8 && to_size == 8 && !to_signed) {
        return int_cast<ToT>(f64_to_ui64({int_cast<uint64_t>(a)}, softfloat_roundingMode, true));
    }
    return ToT(0);
}

template <typename FromT, typename ToT>
ToT fcvt_i2f_impl(const FromT& a) {
    constexpr size_t to_size = sizeof(ToT);
    constexpr bool from_signed = std::is_signed<FromT>::value;
    using INTT = std::conditional<from_signed, int64_t, uint64_t>::type;
    INTT a_ext = int_cast<INTT>(a);
    if constexpr (to_size == 2 && from_signed) {
        return int_cast<ToT>(i64_to_f16(a_ext).v);
    } else if constexpr (to_size == 2 && !from_signed) {
        return int_cast<ToT>(ui64_to_f16(a_ext).v);
    } else if constexpr (to_size == 4 && from_signed) {
        return int_cast<ToT>(i64_to_f32(a_ext).v);
    } else if constexpr (to_size == 4 && !from_signed) {
        return int_cast<ToT>(ui64_to_f32(a_ext).v);
    } else if constexpr (to_size == 8 && from_signed) {
        return int_cast<ToT>(i64_to_f64(a_ext).v);
    } else if constexpr (to_size == 8 && !from_signed) {
        return int_cast<ToT>(ui64_to_f64(a_ext).v);
    }
    return ToT(0);
}

template <typename T>
T fcvtf2si_impl(const T & a) {
    using ST = typename _sint_of_size<T>::_type;
    return fcvt_f2i_impl<T, ST>(a);
}

template <typename T>
T fcvtf2ui_impl(const T & a) {
    using UT = typename _uint_of_size<T>::_type;
    return fcvt_f2i_impl<T, UT>(a);
}

template <typename T>
T fcvti2f_impl(const T & a) {
    using ST = typename _sint_of_size<T>::_type;
    return fcvt_i2f_impl<ST, T>(static_cast<ST>(a));
}
template <typename T>
T fcvtui2f_impl(const T & a) {
    using UT = typename _uint_of_size<T>::_type;
    return fcvt_i2f_impl<UT, T>(static_cast<UT>(a));
}

template <typename T>
void vfcvt_xu_f_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, fcvtf2ui_impl<T>);
}

template <typename T>
void vfcvt_x_f_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, fcvtf2si_impl<T>);
}

template <typename T>
void vfcvt_rtz_xu_f_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfp_set_rounding_mode(1);
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, fcvtf2ui_impl<T>);
}

template <typename T>
void vfcvt_rtz_x_f_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfp_set_rounding_mode(1);
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, fcvtf2si_impl<T>);
}

template <typename T>
void vfcvt_f_xu_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, fcvtui2f_impl<T>);
}

template <typename T>
void vfcvt_f_x_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfop1_common_v(vl, vstart, vd, vs2, mask, nomask, fcvti2f_impl<T>);
}

// 13.18. Widening Floating-Point/Integer Type-Convert Instructions

template <typename SrcT, typename DstT, typename OP>
void vfmvopw_common_vv(const uint32_t & vl, const uint32_t vstart, DstT* __restrict vd, const SrcT* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            vd[i] = op(vs2[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                vd[i] = op(vs2[i]);
            }
        }
    }
}

template <typename T, typename T2>
void vfwcvt_xu_f_v(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T2) == 2*sizeof(T), "Destination type must be twice the size of source type");
    static_assert(std::is_unsigned<T2>::value, "Destination type must be unsigned");
    if constexpr (sizeof(T2) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, T2>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2i_impl<T, T2>);
    }
}

template <typename T, typename T2>
void vfwcvt_x_f_v(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T2) == 2*sizeof(T), "Destination type must be twice the size of source type");
    static_assert(std::is_signed<T2>::value, "Destination type must be signed");
    if constexpr (sizeof(T2) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, T2>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2i_impl<T, T2>);
    }
}

template <typename T, typename T2>
void vfwcvt_rtz_xu_f_v(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T2) == 2*sizeof(T), "Destination type must be twice the size of source type");
    static_assert(std::is_unsigned<T2>::value, "Destination type must be unsigned");
    if constexpr (sizeof(T2) > 8) {
        return; // Not support SEW 128
    } else {
        vfp_set_rounding_mode(1);
        vfmvopw_common_vv<T, T2>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2i_impl<T, T2>);
    }
}

template <typename T, typename T2>
void vfwcvt_rtz_x_f_v(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T2) == 2*sizeof(T), "Destination type must be twice the size of source type");
    static_assert(std::is_signed<T2>::value, "Destination type must be signed");
    if constexpr (sizeof(T2) > 8) {
        return; // Not support SEW 128
    } else {
        vfp_set_rounding_mode(1);
        vfmvopw_common_vv<T, T2>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2i_impl<T, T2>);
    }
}

template <typename T, typename T2>
void vfwcvt_f_xu_v(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T) * 2 == sizeof(T2), "Source type must be half the size of destination type");
    static_assert(std::is_unsigned<T>::value, "Source type must be unsigned");
    if constexpr (sizeof(T2) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, T2>(vl, vstart, vd, vs2, mask, nomask, fcvt_i2f_impl<T, T2>);
    }
}

template <typename T, typename T2>
void vfwcvt_f_x_v(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T) * 2 == sizeof(T2), "Source type must be half the size of destination type");
    static_assert(std::is_signed<T>::value, "Source type must be signed");
    if constexpr (sizeof(T2) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, T2>(vl, vstart, vd, vs2, mask, nomask, fcvt_i2f_impl<T, T2>);
    }
}

template <typename SrcT, typename DstT>
DstT fcvt_f2f_impl(const SrcT& a) {
    constexpr size_t src_size = sizeof(SrcT);
    constexpr size_t dst_size = sizeof(DstT);
    if constexpr (src_size == 2 && dst_size == 4) {
        return static_cast<DstT>(f16_to_f32({a}).v);
    } else if constexpr (src_size == 2 && dst_size == 8) {
        return static_cast<DstT>(f16_to_f64({a}).v);
    } else if constexpr (src_size == 4 && dst_size == 8) {
        return static_cast<DstT>(f32_to_f64({a}).v);
    } else if constexpr (src_size == 4 && dst_size == 2) {
        return static_cast<DstT>(f32_to_f16({a}).v);
    } else if constexpr (src_size == 8 && dst_size == 4) {
        return static_cast<DstT>(f64_to_f32({a}).v);
    } else if constexpr (src_size == 8 && dst_size == 2) {
        return static_cast<DstT>(f64_to_f16({a}).v);
    }
    return DstT(0);
}

template <typename T, typename T2>
void vfwcvt_f_f_v(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T2) == 2*sizeof(T), "Destination type must be twice the size of source type");
    if constexpr (sizeof(T2) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, T2>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2f_impl<T, T2>);
    }
}

// 13.19. Narrowing Floating-Point/Integer Type-Convert Instructions

template <typename T, typename NarrowT>
void vfncvt_xu_f_w(const uint32_t & vl, const uint32_t vstart, NarrowT* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(NarrowT) * 2 == sizeof(T), "Destination type must be half the size of source type");
    static_assert(std::is_unsigned<NarrowT>::value, "Destination type must be unsigned");
    if constexpr (sizeof(T) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, NarrowT>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2i_impl<T, NarrowT>);
    }
}

template <typename T, typename NarrowT>
void vfncvt_x_f_w(const uint32_t & vl, const uint32_t vstart, NarrowT* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(NarrowT) * 2 == sizeof(T), "Destination type must be half the size of source type");
    static_assert(std::is_signed<NarrowT>::value, "Destination type must be signed");
    if constexpr (sizeof(T) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, NarrowT>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2i_impl<T, NarrowT>);
    }
}

template <typename T, typename NarrowT>
void vfncvt_rtz_xu_f_w(const uint32_t & vl, const uint32_t vstart, NarrowT* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(NarrowT) * 2 == sizeof(T), "Destination type must be half the size of source type");
    static_assert(std::is_unsigned<NarrowT>::value, "Destination type must be unsigned");
    if constexpr (sizeof(T) > 8) {
        return; // Not support SEW 128
    } else {
        vfp_set_rounding_mode(1);
        vfmvopw_common_vv<T, NarrowT>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2i_impl<T, NarrowT>);
    }
}

template <typename T, typename NarrowT>
void vfncvt_rtz_x_f_w(const uint32_t & vl, const uint32_t vstart, NarrowT* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(NarrowT) * 2 == sizeof(T), "Destination type must be half the size of source type");
    static_assert(std::is_signed<NarrowT>::value, "Destination type must be signed");
    if constexpr (sizeof(T) > 8) {
        return; // Not support SEW 128
    } else {
        vfp_set_rounding_mode(1);
        vfmvopw_common_vv<T, NarrowT>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2i_impl<T, NarrowT>);
    }
}

template <typename T, typename NarrowT>
void vfncvt_f_xu_w(const uint32_t & vl, const uint32_t vstart, NarrowT* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(NarrowT) * 2 == sizeof(T), "Source type must be twice the size of destination type");
    static_assert(std::is_unsigned<T>::value, "Source type must be unsigned");
    if constexpr (sizeof(T) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, NarrowT>(vl, vstart, vd, vs2, mask, nomask, fcvt_i2f_impl<T, NarrowT>);
    }
}

template <typename T, typename NarrowT>
void vfncvt_f_x_w(const uint32_t & vl, const uint32_t vstart, NarrowT* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(NarrowT) * 2 == sizeof(T), "Source type must be twice the size of destination type");
    static_assert(std::is_signed<T>::value, "Source type must be signed");
    if constexpr (sizeof(T) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, NarrowT>(vl, vstart, vd, vs2, mask, nomask, fcvt_i2f_impl<T, NarrowT>);
    }
}

template <typename T, typename NarrowT>
void vfncvt_f_f_w(const uint32_t & vl, const uint32_t vstart, NarrowT* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(NarrowT) * 2 == sizeof(T), "Destination type must be half the size of source type");
    if constexpr (sizeof(T) > 8) {
        return; // Not support SEW 128
    } else {
        vfmvopw_common_vv<T, NarrowT>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2f_impl<T, NarrowT>);
    }
}

template <typename T, typename NarrowT>
void vfncvt_rod_f_f_w(const uint32_t & vl, const uint32_t vstart, NarrowT* __restrict vd, const T* __restrict vs2, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(NarrowT) * 2 == sizeof(T), "Destination type must be half the size of source type");
    if constexpr (sizeof(T) > 8) {
        return; // Not support SEW 128
    } else {
        vfp_set_rounding_mode(5);
        vfmvopw_common_vv<T, NarrowT>(vl, vstart, vd, vs2, mask, nomask, fcvt_f2f_impl<T, NarrowT>);
    }
}






} // namespace rvvarchsem
