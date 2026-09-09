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
#include "vfp.hpp"

#include <cstdint>
#include <type_traits>

namespace rvvarchsem {

template <typename SrcT, typename AccT, typename OP>
void vredop_common_vs(const uint32_t & vl, const uint32_t vstart, AccT &acc, const SrcT* __restrict vs2, const AccT& init, const ExpdMaskT * __restrict mask, const bool nomask, OP op) {
    if (vstart >= vl) {
        return;
    }
    acc = init;
    if (nomask) {
        for (uint32_t i = vstart; i < vl; i++) {
            acc = op(acc, vs2[i]);
        }
    } else {
        for (uint32_t i = vstart; i < vl; i++) {
            if (mask[i]) {
                acc = op(acc, vs2[i]);
            }
        }
    }
}

// 14.1. Vector Single-Width Integer Reduction Instructions

template <typename T>
void vredsum_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T & a, const T & b) -> T { return a + b; });
}

template <typename T>
void vredmaxu_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned type");
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T & a, const T & b) -> T { return (a > b) ? a : b; });
}

template <typename T>
void vredmax_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(std::is_signed<T>::value, "T must be a signed type");
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T & a, const T & b) -> T { return (a > b) ? a : b; });
}

template <typename T>
void vredminu_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned type");
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T & a, const T & b) -> T { return (a < b) ? a : b; });
}

template <typename T>
void vredmin_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(std::is_signed<T>::value, "T must be a signed type");
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T & a, const T & b) -> T { return (a < b) ? a : b; });
}

template <typename T>
void vredand_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T & a, const T & b) -> T { return a & b; });
}

template <typename T>
void vredor_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T & a, const T & b) -> T { return a | b; });
}

template <typename T>
void vredxor_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T & a, const T & b) -> T { return a ^ b; });
}

// 14.2. Vector Widening Integer Reduction Instructions

template <typename T, typename T2>
void vwredsumu_vs(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T2* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    static_assert(std::is_unsigned<T2>::value, "T2 must be an unsigned type");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    vredop_common_vs<T, T2>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T2 & a, const T & b) -> T2 { return a + static_cast<T2>(b); });
}

template <typename T, typename T2>
void vwredsum_vs(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T2* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    static_assert(std::is_signed<T2>::value, "T2 must be a signed type");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    vredop_common_vs<T, T2>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T2 & a, const T & b) -> T2 { return a + static_cast<T2>(b); });
}

// 14.3. Vector Single-Width Floating-Point Reduction Instructions

template <typename T>
void vfredosum_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, fadd_impl<T>);
}

template <typename T>
void vfredusum_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfredosum_vs<T>(vl, vstart, vd, vs2, vs1, mask, nomask);
}

template <typename T>
void vfredmin_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, fmin_impl<T>);
}

template <typename T>
void vfredmax_vs(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vredop_common_vs<T, T>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, fmax_impl<T>);
}

// 14.4. Vector Widening Floating-Point Reduction Instructions

template <typename T, typename T2>
void vfwredosum_vs(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T2* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    vredop_common_vs<T, T2>(vl, vstart, vd[0], vs2, vs1[0], mask, nomask, [](const T2 & a, const T & b) -> T2 { return fadd_impl<T2>(a, fcvt_impl<T, T2>(b)); });
}

template <typename T, typename T2>
void vfwredusum_vs(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T2* __restrict vs1, const ExpdMaskT * __restrict mask, const bool nomask) {
    vfwredosum_vs<T, T2>(vl, vstart, vd, vs2, vs1, mask, nomask);
}



} // namespace rvvarchsem
