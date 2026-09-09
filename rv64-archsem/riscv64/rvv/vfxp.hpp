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

#include <cstdint>

namespace rvvarchsem {

enum class VXRM {
    rnu = 0b00, // Round to Nearest, ties to Up
    rne = 0b01, // Round to Nearest, ties to Even
    rdn = 0b10, // Round Down (towards -infinity)
    rup = 0b11, // Round Up (towards +infinity)
};

template <typename T>
T roundoff_signed(const T& a, const uint32_t& shmt, const VXRM & rm, bool &vxsat) {
    (void)vxsat;
    using ST = SIntType<T>;
    using UT = UIntType<T>;

    constexpr uint32_t bit_width = sizeof(T) * 8;
    ST sa = static_cast<ST>(a);

    if (shmt == 0) {
        return static_cast<T>(sa);
    }
    if (shmt >= bit_width) {
        return static_cast<T>(sa < 0 ? ST(-1) : ST(0));
    }

    UT ua = static_cast<UT>(sa);
    ST shifted = static_cast<ST>(sa >> shmt);

    const bool bit_d_minus_1 = ((ua >> (shmt - 1)) & UT(1)) != 0;
    const bool lower_nonzero =
        (shmt > 1) && ((ua & ((UT(1) << (shmt - 1)) - 1)) != 0);
    const bool bit_d = (static_cast<UT>(shifted) & UT(1)) != 0;
    const bool discarded_nonzero = (ua & ((UT(1) << shmt) - 1)) != 0;

    ST addend = 0;
    switch (rm) {
        case VXRM::rnu:
            addend = bit_d_minus_1 ? ST(1) : ST(0);
            break;
        case VXRM::rne:
            addend = (bit_d_minus_1 && (lower_nonzero || bit_d)) ? ST(1) : ST(0);
            break;
        case VXRM::rdn:
            addend = ST(0);
            break;
        case VXRM::rup: // RVV vxrm=3, round-to-odd
            addend = (!bit_d && discarded_nonzero) ? ST(1) : ST(0);
            break;
        default:
            addend = ST(0);
            break;
    }

    return static_cast<T>(shifted + addend);
}

// template <typename T>
// T roundoff_signed(const T& a, const uint32_t& shmt, const VXRM & rm, bool &vxsat) {
//     using ST = SIntType<T>;
//     constexpr uint32_t bit_width = sizeof(T) * 8;
//     if (shmt >= bit_width) {
//         return static_cast<T>(0);
//     }
//     ST sa = static_cast<ST>(a);
//     ST addend = 0;
//     switch (rm) {
//         case VXRM::rnu:
//             addend = static_cast<ST>(static_cast<ST>(1) << (shmt - 1));
//             break;
//         case VXRM::rne:
//             addend = ((sa & (static_cast<ST>(1) << shmt)) != 0) ?
//                       static_cast<ST>(static_cast<ST>(1) << (shmt - 1)) :
//                       static_cast<ST>((static_cast<ST>(1) << (shmt - 1)) - 1);
//             break;
//         case VXRM::rdn:
//             addend = (sa < 0) ? static_cast<ST>((static_cast<ST>(1) << shmt) - 1) : static_cast<ST>(0);
//             break;
//         case VXRM::rup:
//             addend = (sa > 0) ? static_cast<ST>((static_cast<ST>(1) << shmt) - 1) : static_cast<ST>(0);
//             break;
//         default:
//             addend = static_cast<ST>(0);
//             break;
//     }
//     if (a & ((static_cast<T>(1) << shmt) - 1)) {
//         vxsat = true;
//     }
//     ST sres = sa + addend;
//     return static_cast<T>(sres >> shmt);
// }

template <typename T>
T roundoff_unsigned(const T& a, const uint32_t& shmt, const VXRM & rm, bool &vxsat) {
    (void)vxsat;
    using UT = UIntType<T>;

    constexpr uint32_t bit_width = sizeof(T) * 8;
    UT ua = static_cast<UT>(a);

    if (shmt == 0) {
        return static_cast<T>(ua);
    }
    if (shmt >= bit_width) {
        return static_cast<T>(0);
    }

    UT shifted = ua >> shmt;

    const bool bit_d_minus_1 = ((ua >> (shmt - 1)) & UT(1)) != 0;
    const bool lower_nonzero =
        (shmt > 1) && ((ua & ((UT(1) << (shmt - 1)) - 1)) != 0);
    const bool bit_d = (shifted & UT(1)) != 0;
    const bool discarded_nonzero = (ua & ((UT(1) << shmt) - 1)) != 0;

    UT addend = 0;
    switch (rm) {
        case VXRM::rnu:
            addend = bit_d_minus_1 ? UT(1) : UT(0);
            break;
        case VXRM::rne:
            addend = (bit_d_minus_1 && (lower_nonzero || bit_d)) ? UT(1) : UT(0);
            break;
        case VXRM::rdn:
            addend = UT(0);
            break;
        case VXRM::rup: // RVV vxrm=3, round-to-odd
            addend = (!bit_d && discarded_nonzero) ? UT(1) : UT(0);
            break;
        default:
            addend = UT(0);
            break;
    }

    return static_cast<T>(shifted + addend);
}
// template <typename T>
// T roundoff_unsigned(const T& a, const uint32_t& shmt, const VXRM & rm, bool &vxsat) {
//     using UT = UIntType<T>;
//     constexpr uint32_t bit_width = sizeof(T) * 8;
//     if (shmt >= bit_width) {
//         return static_cast<T>(0);
//     }
//     UT ua = static_cast<UT>(a);
//     UT addend = 0;
//     switch (rm) {
//         case VXRM::rnu:
//             addend = static_cast<UT>(static_cast<UT>(1) << (shmt - 1));
//             break;
//         case VXRM::rne:
//             addend = ((ua & (static_cast<UT>(1) << shmt)) != 0) ?
//                       static_cast<UT>(static_cast<UT>(1) << (shmt - 1)) :
//                       static_cast<UT>((static_cast<UT>(1) << (shmt - 1)) - 1);
//             break;
//         case VXRM::rdn:
//             addend = static_cast<UT>(0);
//             break;
//         case VXRM::rup:
//             addend = (ua != 0) ? static_cast<UT>((static_cast<UT>(1) << shmt) - 1) : static_cast<UT>(0);
//             break;
//         default:
//             addend = static_cast<UT>(0);
//             break;
//     }
//     if (a & ((static_cast<T>(1) << shmt) - 1)) {
//         vxsat = true;
//     }
//     UT ures = ua + addend;
//     return static_cast<T>(ures >> shmt);
// }

// 12.1. Vector Single-Width Saturating Add and Subtract

template <typename T>
void vsaddu_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, bool &vxsat) {
    using UT = typename _uint_of_size<T>::_type;
    constexpr UT MAX_VAL = static_cast<UT>(~0ULL);
    for (uint32_t i = vstart; i < vl; i++) {
        UT res = static_cast<UT>(vs1[i]) + static_cast<UT>(vs2[i]);
        vd[i] = ((mask[i]) ? ((res < static_cast<UT>(vs1[i])) ? static_cast<T>(MAX_VAL) : static_cast<T>(res)) : vd[i]);
        if (mask[i] && res < static_cast<UT>(vs1[i])) {
            vxsat = true;
        }
    }
};

template <typename T>
void vsaddu_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, bool &vxsat) {
    using UT = typename _uint_of_size<T>::_type;
    constexpr UT MAX_VAL = static_cast<UT>(~0ULL);
    UT rs1_u = static_cast<UT>(rs1);
    for (uint32_t i = vstart; i < vl; i++) {
        UT res = static_cast<UT>(vs2[i]) + rs1_u;
        vd[i] = ((mask[i]) ? ((res < rs1_u) ? static_cast<T>(MAX_VAL) : static_cast<T>(res)) : vd[i]);
        if (mask[i] && res < rs1_u) {
            vxsat = true;
        }
    }
};

template <typename T>
T fxpadd_impl(const T & a, const T &b, bool &vxsat) {
    using ST = SIntType<T>;
    using WT = SInt2WidType<T>;
    constexpr uint32_t bit_width = sizeof(T) * 8;
    const WT min_val = -(WT(1) << (bit_width - 1));
    const WT max_val = (WT(1) << (bit_width - 1)) - 1;
    const WT res = static_cast<WT>(static_cast<ST>(a)) + static_cast<WT>(static_cast<ST>(b));
    if (res > max_val) { vxsat = true; return static_cast<T>(static_cast<ST>(max_val)); }
    if (res < min_val) { vxsat = true; return static_cast<T>(static_cast<ST>(min_val)); }
    return static_cast<T>(static_cast<ST>(res));
}

// template <typename T>
// T fxpadd_impl(const T & a, const T &b, bool &vxsat) {
//     using ST = SIntType<T>;
//     using UT = UIntType<T>;
//     constexpr uint32_t bit_width = sizeof(T) * 8;
//     if ((a & (static_cast<T>(1) << (bit_width - 1))) == 0 && (b & (static_cast<T>(1) << (bit_width - 1))) == 0) {
//         // both positive
//         UT ua = static_cast<UT>(a);
//         UT ub = static_cast<UT>(b);
//         UT ures = ua + ub;
//         if (ures < ua) {
//             vxsat = true;
//             return static_cast<T>(~(static_cast<UT>(0) >> 1)); // max positive
//         } else {
//             return static_cast<T>(ures);
//         }
//     } else if ((a & (static_cast<T>(1) << (bit_width - 1))) != 0 && (b & (static_cast<T>(1) << (bit_width - 1))) != 0) {
//         // both negative
//         ST sa = static_cast<ST>(a);
//         ST sb = static_cast<ST>(b);
//         ST sres = sa + sb;
//         if (sres > sa) {
//             vxsat = true;
//             return static_cast<T>(static_cast<UT>(1) << (bit_width - 1)); // max negative
//         } else {
//             return static_cast<T>(sres);
//         }
//     } else {
//         // different sign
//         return static_cast<T>(static_cast<ST>(a) + static_cast<ST>(b));
//     }
// }

template <typename T>
void vsadd_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, bool &vxsat) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? fxpadd_impl<T>(vs2[i], vs1[i], vxsat) : vd[i]);
    }
};

template <typename T>
void vsadd_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, bool &vxsat) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? fxpadd_impl<T>(vs2[i], rs1, vxsat) : vd[i]);
    }
};

template <typename T>
void vssubu_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, bool &vxsat) {
    using UT = typename _uint_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (static_cast<UT>(vs2[i]) < static_cast<UT>(vs1[i]) ? static_cast<T>(0) : static_cast<T>(static_cast<UT>(vs2[i]) - static_cast<UT>(vs1[i]))) : vd[i]);
        if (mask[i] && static_cast<UT>(vs2[i]) < static_cast<UT>(vs1[i])) {
            vxsat = true;
        }
    }
};

template <typename T>
void vssubu_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, bool &vxsat) {
    using UT = typename _uint_of_size<T>::_type;
    UT rs1_u = static_cast<UT>(rs1);
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (static_cast<UT>(vs2[i]) < rs1_u ? static_cast<T>(0) : static_cast<T>(static_cast<UT>(vs2[i]) - rs1_u)) : vd[i]);
        if (mask[i] && static_cast<UT>(vs2[i]) < rs1_u) {
            vxsat = true;
        }
    }
};

template <typename T>
T fxpsub_impl(const T & a, const T &b, bool &vxsat) {
    using ST = SIntType<T>;
    using WT = SInt2WidType<T>;
    constexpr uint32_t bit_width = sizeof(T) * 8;
    const WT min_val = -(WT(1) << (bit_width - 1));
    const WT max_val = (WT(1) << (bit_width - 1)) - 1;
    const WT res = static_cast<WT>(static_cast<ST>(a)) - static_cast<WT>(static_cast<ST>(b));
    if (res > max_val) { vxsat = true; return static_cast<T>(static_cast<ST>(max_val)); }
    if (res < min_val) { vxsat = true; return static_cast<T>(static_cast<ST>(min_val)); }
    return static_cast<T>(static_cast<ST>(res));
}
// template <typename T>
// T fxpsub_impl(const T & a, const T &b, bool &vxsat) {
//     using ST = SIntType<T>;
//     T negb = static_cast<T>(-static_cast<ST>(b));
//     return fxpadd_impl<T>(a, negb, vxsat);
// }

template <typename T>
void vssub_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, bool &vxsat) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? fxpsub_impl<T>(vs2[i], vs1[i], vxsat) : vd[i]);
    }
};

template <typename T>
void vssub_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, bool &vxsat) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? fxpsub_impl<T>(vs2[i], rs1, vxsat) : vd[i]);
    }
};

// 12.2. Vector Single-Width Averaging Add and Subtract

template <typename T>
void vaaddu_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using UT = typename _uint_double_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_unsigned<UT>((static_cast<UT>(vs2[i]) + static_cast<UT>(vs1[i])), 1, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vaaddu_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using UT = typename _uint_double_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_unsigned<UT>((static_cast<UT>(vs2[i]) + static_cast<UT>(rs1)), 1, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vaadd_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using ST = typename _sint_double_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_unsigned<ST>((static_cast<ST>(vs2[i]) + static_cast<ST>(vs1[i])), 1, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vaadd_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using ST = typename _sint_double_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_signed<ST>((static_cast<ST>(vs2[i]) + static_cast<ST>(rs1)), 1, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vasubu_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using UT = typename _uint_double_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_unsigned<UT>((static_cast<UT>(vs2[i]) - static_cast<UT>(vs1[i])), 1, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vasubu_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using UT = typename _uint_double_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_unsigned<UT>((static_cast<UT>(vs2[i]) - static_cast<UT>(rs1)), 1, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vasub_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using ST = typename _sint_double_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_signed<ST>((static_cast<ST>(vs2[i]) - static_cast<ST>(vs1[i])), 1, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vasub_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using ST = typename _sint_double_of_size<T>::_type;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_signed<ST>((static_cast<ST>(vs2[i]) - static_cast<ST>(rs1)), 1, rm, vxsat)) : vd[i]);
    }
};

// 12.3. Vector Single-Width Fractional Multiply with Rounding and Saturation

template <typename T>
void vsmul_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using ST = typename _sint_double_of_size<T>::_type;
    using BaseST = SIntType<T>;
    using BaseUT = UIntType<T>;
    constexpr uint32_t bit_width = sizeof(T) * 8;
    constexpr BaseST minv = static_cast<BaseST>(BaseUT(1) << (bit_width - 1));
    constexpr BaseST maxv = static_cast<BaseST>((BaseUT(1) << (bit_width - 1)) - 1);
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            const BaseST a = static_cast<BaseST>(vs2[i]);
            const BaseST b = static_cast<BaseST>(vs1[i]);
            if (a == minv && b == minv) {
                vd[i] = static_cast<T>(maxv);
                vxsat = true;
            } else {
                vd[i] = static_cast<T>(roundoff_signed<ST>(static_cast<ST>(a) * static_cast<ST>(b), bit_width - 1, rm, vxsat));
            }
        }
    }
};

template <typename T>
void vsmul_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T & rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using ST = typename _sint_double_of_size<T>::_type;
    using BaseST = SIntType<T>;
    using BaseUT = UIntType<T>;
    constexpr uint32_t bit_width = sizeof(T) * 8;
    constexpr BaseST minv = static_cast<BaseST>(BaseUT(1) << (bit_width - 1));
    constexpr BaseST maxv = static_cast<BaseST>((BaseUT(1) << (bit_width - 1)) - 1);
    const BaseST b = static_cast<BaseST>(rs1);
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            const BaseST a = static_cast<BaseST>(vs2[i]);
            if (a == minv && b == minv) {
                vd[i] = static_cast<T>(maxv);
                vxsat = true;
            } else {
                vd[i] = static_cast<T>(roundoff_signed<ST>(static_cast<ST>(a) * static_cast<ST>(b), bit_width - 1, rm, vxsat));
            }
        }
    }
};

// 12.4. Vector Single-Width Scaling Shift Instructions

template <typename T>
void vssrl_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using UT = UIntType<T>;
    constexpr uint32_t shmask = sizeof(T) * 8 - 1;
    for (uint32_t i = vstart; i < vl; i++) {
        uint32_t shmt = static_cast<uint32_t>(vs1[i]) & shmask;
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_unsigned<UT>(static_cast<UT>(vs2[i]), shmt, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vssrl_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint32_t & rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using UT = UIntType<T>;
    constexpr uint32_t shmask = sizeof(T) * 8 - 1;
    const uint32_t shmt = rs1 & shmask;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_unsigned<UT>(static_cast<UT>(vs2[i]), shmt, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vssra_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using ST = SIntType<T>;
    constexpr uint32_t shmask = sizeof(T) * 8 - 1;
    for (uint32_t i = vstart; i < vl; i++) {
        uint32_t shmt = static_cast<uint32_t>(vs1[i]) & shmask;
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_signed<ST>(static_cast<ST>(vs2[i]), shmt, rm, vxsat)) : vd[i]);
    }
};

template <typename T>
void vssra_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint32_t & rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    using ST = SIntType<T>;
    constexpr uint32_t shmask = sizeof(T) * 8 - 1;
    const uint32_t shmt = rs1 & shmask;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(roundoff_signed<ST>(static_cast<ST>(vs2[i]), shmt, rm, vxsat)) : vd[i]);
    }
};

// 12.5. Vector Narrowing Fixed-Point Clip Instructions

template <typename T, typename T2>
inline T sat_unsigned_narrow(const T2& value, bool& vxsat) {
    constexpr uint32_t bits = sizeof(T) * 8;
    const T2 maxv = (static_cast<T2>(1) << bits) - 1;
    if (value > maxv) {
        vxsat = true;
        return static_cast<T>(maxv);
    }
    return static_cast<T>(value);
}

template <typename T, typename T2>
inline T sat_signed_narrow(const T2& value, bool& vxsat) {
    constexpr uint32_t bits = sizeof(T) * 8;
    const T2 maxv = (static_cast<T2>(1) << (bits - 1)) - 1;
    const T2 minv = -(static_cast<T2>(1) << (bits - 1));
    if (value > maxv) {
        vxsat = true;
        return static_cast<T>(maxv);
    }
    if (value < minv) {
        vxsat = true;
        return static_cast<T>(minv);
    }
    return static_cast<T>(value);
}

template <typename T, typename T2>
void vnclipu_wv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd,
                const T2* __restrict vs2, const T* __restrict vs1,
                const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    constexpr uint32_t shmask = sizeof(T2) * 8 - 1;
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            const uint32_t shmt = static_cast<uint32_t>(vs1[i]) & shmask;
            T2 rounded = roundoff_unsigned<T2>(vs2[i], shmt, rm, vxsat);
            vd[i] = sat_unsigned_narrow<T, T2>(rounded, vxsat);
        }
    }
}

template <typename T, typename T2>
void vnclipu_wxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd,
                 const T2* __restrict vs2, const uint32_t & rs1,
                 const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    constexpr uint32_t shmask = sizeof(T2) * 8 - 1;
    const uint32_t shmt = rs1 & shmask;
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            T2 rounded = roundoff_unsigned<T2>(vs2[i], shmt, rm, vxsat);
            vd[i] = sat_unsigned_narrow<T, T2>(rounded, vxsat);
        }
    }
}

template <typename T, typename T2>
void vnclip_wv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd,
               const T2* __restrict vs2, const T* __restrict vs1,
               const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    constexpr uint32_t shmask = sizeof(T2) * 8 - 1;
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            const uint32_t shmt = static_cast<uint32_t>(vs1[i]) & shmask;
            T2 rounded = roundoff_signed<T2>(vs2[i], shmt, rm, vxsat);
            vd[i] = sat_signed_narrow<T, T2>(rounded, vxsat);
        }
    }
}

template <typename T, typename T2>
void vnclip_wxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd,
                const T2* __restrict vs2, const uint32_t & rs1,
                const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
    constexpr uint32_t shmask = sizeof(T2) * 8 - 1;
    const uint32_t shmt = rs1 & shmask;
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            T2 rounded = roundoff_signed<T2>(vs2[i], shmt, rm, vxsat);
            vd[i] = sat_signed_narrow<T, T2>(rounded, vxsat);
        }
    }
}
// template <typename T, typename T2>
// void vnclipu_wv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T2* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
//     static_assert(sizeof(T2) == sizeof(T) * 2, "T2 must be double the size of T");
//     if constexpr (sizeof(T2) > 8) {
//         return; // not defined for 128-bit destination
//     }
//     using UT2 = typename _uint_of_size<T2>::_type;
//     using UT = typename _uint_of_size<T>::_type;
//     for (uint32_t i = vstart; i < vl; i++) {
//         uint32_t shmt = static_cast<uint32_t>(vs1[i]);
//         vd[i] = ((mask[i]) ? static_cast<UT>( roundoff_unsigned<UT2>(static_cast<UT2>(vs2[i]), shmt, rm, vxsat) ) : vd[i]);
//     }
// }

// template <typename T, typename T2>
// void vnclipu_wxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T2* __restrict vs2, const uint32_t & rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
//     static_assert(sizeof(T2) == sizeof(T) * 2, "T2 must be double the size of T");
//     if constexpr (sizeof(T2) > 8) {
//         return; // not defined for 128-bit destination
//     }
//     using UT2 = typename _uint_of_size<T2>::_type;
//     using UT = typename _uint_of_size<T>::_type;
//     for (uint32_t i = vstart; i < vl; i++) {
//         vd[i] = ((mask[i]) ? static_cast<UT>( roundoff_unsigned<UT2>(static_cast<UT2>(vs2[i]), rs1, rm, vxsat) ) : vd[i]);
//     }
// }

// template <typename T, typename T2>
// void vnclip_wv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T2* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
//     static_assert(sizeof(T2) == sizeof(T) * 2, "T2 must be double the size of T");
//     if constexpr (sizeof(T2) > 8) {
//         return; // not defined for 128-bit destination
//     }
//     using ST2 = typename _sint_of_size<T2>::_type;
//     using ST = typename _sint_of_size<T>::_type;
//     for (uint32_t i = vstart; i < vl; i++) {
//         uint32_t shmt = static_cast<uint32_t>(vs1[i]);
//         vd[i] = ((mask[i]) ? static_cast<ST>( roundoff_signed<ST2>(static_cast<ST2>(vs2[i]), shmt, rm, vxsat) ) : vd[i]);
//     }
// }

// template <typename T, typename T2>
// void vnclip_wxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T2* __restrict vs2, const uint32_t & rs1, const ExpdMaskT * __restrict mask, const VXRM & rm, bool &vxsat) {
//     static_assert(sizeof(T2) == sizeof(T) * 2, "T2 must be double the size of T");
//     if constexpr (sizeof(T2) > 8) {
//         return; // not defined for 128-bit destination
//     }
//     using ST2 = typename _sint_of_size<T2>::_type;
//     using ST = typename _sint_of_size<T>::_type;
//     for (uint32_t i = vstart; i < vl; i++) {
//         vd[i] = ((mask[i]) ? static_cast<ST>( roundoff_signed<ST2>(static_cast<ST2>(vs2[i]), rs1, rm, vxsat) ) : vd[i]);
//     }
// }





} // namespace rvvarchsem
