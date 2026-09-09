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
#include <type_traits>

namespace rvvarchsem {

// 11.1. Vector Single-Width Integer Add and Subtract

template <typename T>
void vadd_vv(const uint32_t &vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs1[i] + vs2[i]) : vd[i]);
    }
}
    
template <typename T>
void vadd_vxi(const uint32_t &vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] + rs1) : vd[i]);
    }
}

template <typename T>
void vsub_vv(const uint32_t &vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] - vs1[i]) : vd[i]);
    }
}

template <typename T>
void vsub_vxi(const uint32_t &vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] - rs1) : vd[i]);
    }
};

template <typename T>
void vrsub_vxi(const uint32_t &vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (rs1 - vs2[i]) : vd[i]);
    }
};

// 11.2. Vector Widening Integer Add/Subtract

template <typename T, typename T2>
void vwadd_vv(const uint32_t &vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (static_cast<T2>(vs1[i]) + static_cast<T2>(vs2[i])) : vd[i]);
    }
};

template <typename T, typename T2>
void vwadd_vx(const uint32_t &vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T2 &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (static_cast<T2>(vs2[i]) + rs1) : vd[i]);
    }
};

template <typename T, typename T2>
void vwsub_vv(const uint32_t &vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (static_cast<T2>(vs2[i]) - static_cast<T2>(vs1[i])) : vd[i]);
    }
};

template <typename T, typename T2>
void vwsub_vx(const uint32_t &vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T2 &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (static_cast<T2>(vs2[i]) - rs1) : vd[i]);
    }
};


template <typename T, typename T2>
void vwadd_wv(const uint32_t &vl, const uint32_t vstart, T2* __restrict vd, const T2* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (static_cast<T2>(vs1[i]) + vs2[i]) : vd[i]);
    }
};

template <typename T, typename T2>
void vwadd_wx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T2* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] + static_cast<T2>(rs1)) : vd[i]);
    }
};

template <typename T, typename T2>
void vwsub_wv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T2* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] - static_cast<T2>(vs1[i])) : vd[i]);
    }
};

template <typename T, typename T2>
void vwsub_wx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T2* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] - static_cast<T2>(rs1)) : vd[i]);
    }
};

// 11.3. Vector Integer Extension

template <typename T, typename NarrowT>
void vext_vf(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const NarrowT* __restrict vs2, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i])?static_cast<T>(vs2[i]): vd[i]);
    }
};

// 11.4. Vector Integer Add-with-Carry / Subtract-with-Borrow Instructions

template <typename T>
void vadc_vvm(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>(vs2[i] + vs1[i] + (mask[i] ? T(1) : T(0)));
    }
};

template <typename T>
void vadc_vxim(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>(vs2[i] + rs1 + (mask[i] ? T(1) : T(0)));
    }
};

template <typename T>
void vmadc_vvm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        const bool cin = mask[i];
        T sum = static_cast<T>(vs2[i] + vs1[i] + (cin ? T(1) : T(0)));
        set_vmask(vd, i, (sum < vs2[i]) || (cin && sum == vs2[i]));
    }
}
// template <typename T>
// void vmadc_vvm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         T sum = static_cast<T>(vs2[i] + vs1[i] + (mask[i] ? T(1) : T(0)));
//         set_vmask(vd, i, sum < vs2[i]);
//     }
// }

template <typename T>
void vmadc_vxim(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        const bool cin = mask[i];
        T sum = static_cast<T>(vs2[i] + rs1 + (cin ? T(1) : T(0)));
        set_vmask(vd, i, (sum < vs2[i]) || (cin && sum == vs2[i]));
    }
}

// template <typename T>
// void vmadc_vxim(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         T sum = static_cast<T>(vs2[i] + rs1 + (mask[i] ? T(1) : T(0)));
//         set_vmask(vd, i, sum < vs2[i]);
//     }
// }

template <typename T>
void vmadc_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1) {
    for (uint32_t i = vstart; i < vl; i++) {
        T sum = vs2[i] + vs1[i];
        set_vmask(vd, i, sum < vs2[i]);
    }
}

template <typename T>
void vmadc_vxi(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1) {
    for (uint32_t i = vstart; i < vl; i++) {
        T sum = vs2[i] + rs1;
        set_vmask(vd, i, sum < vs2[i]);
    }
}


template <typename T>
void vsbc_vvm(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>(vs2[i] - vs1[i] - (mask[i] ? T(1) : T(0)));
    }
}

template <typename T>
void vsbc_vxim(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>(vs2[i] - rs1 - (mask[i] ? T(1) : T(0)));
    }
}

template <typename T>
void vmsbc_vvm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        const bool bin = mask[i];
        T diff = static_cast<T>(vs2[i] - vs1[i] - (bin ? T(1) : T(0)));
        set_vmask(vd, i, (diff > vs2[i]) || (bin && diff == vs2[i]));
    }
}
// template <typename T>
// void vmsbc_vvm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         T diff = static_cast<T>(vs2[i] - vs1[i] - (mask[i] ? T(1) : T(0)));
//         set_vmask(vd, i, diff > vs2[i]);
//     }
// }

template <typename T>
void vmsbc_vxim(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        const bool bin = mask[i];
        T diff = static_cast<T>(vs2[i] - rs1 - (bin ? T(1) : T(0)));
        set_vmask(vd, i, (diff > vs2[i]) || (bin && diff == vs2[i]));
    }
}
// template <typename T>
// void vmsbc_vxim(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         T diff = static_cast<T>(vs2[i] - rs1 - (mask[i] ? T(1) : T(0)));
//         set_vmask(vd, i, diff > vs2[i]);
//     }
// }

template <typename T>
void vmsbc_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1) {
    for (uint32_t i = vstart; i < vl; i++) {
        T diff = vs2[i] - vs1[i];
        set_vmask(vd, i, diff > vs2[i]);
    }
}

template <typename T>
void vmsbc_vxi(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1) {
    for (uint32_t i = vstart; i < vl; i++) {
        T diff = vs2[i] - rs1;
        set_vmask(vd, i, diff > vs2[i]);
    }
}

// 11.5. Vector Bitwise Logical Instructions

template <typename T>
void vand_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = (mask[i] ? (vs2[i] & vs1[i]) : vd[i]);
    }
}

template <typename T>
void vand_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] & rs1) : vd[i]);
    }
}

template <typename T>
void vor_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] | vs1[i]) : vd[i]);
    }
}

template <typename T>
void vor_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] | rs1) : vd[i]);
    }
}

template <typename T>
void vxor_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] ^ vs1[i]) : vd[i]);
    }
}

template <typename T>
void vxor_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] ^ rs1) : vd[i]);
    }
}

// 11.6. Vector Single-Width Shift Instructions

template <typename T>
void vsl_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    constexpr uint32_t bit_width = sizeof(T) * 8;
    constexpr uint32_t shift_mask = bit_width - 1;
    for (uint32_t i = vstart; i < vl; i++) {
        uint32_t shamt = static_cast<uint32_t>(vs1[i]) & shift_mask;
        vd[i] = ((mask[i]) ? static_cast<T>(vs2[i] << shamt) : vd[i]);
    }
}

template <typename T>
void vsl_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint32_t &rs1, const ExpdMaskT * __restrict mask) {
    constexpr uint32_t bit_width = sizeof(T) * 8;
    constexpr uint32_t shift_mask = bit_width - 1;
    uint32_t shamt = rs1 & shift_mask;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(vs2[i] << shamt) : vd[i]);
    }
}

template <typename T>
void vsr_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    constexpr uint32_t bit_width = sizeof(T) * 8;
    constexpr uint32_t shift_mask = bit_width - 1;
    for (uint32_t i = vstart; i < vl; i++) {
        uint32_t shamt = static_cast<uint32_t>(vs1[i]) & shift_mask;
        vd[i] = ((mask[i]) ? static_cast<T>(vs2[i] >> shamt) : vd[i]);
    }
}

template <typename T>
void vsr_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint32_t &rs1, const ExpdMaskT * __restrict mask) {
    constexpr uint32_t bit_width = sizeof(T) * 8;
    constexpr uint32_t shift_mask = bit_width - 1;
    uint32_t shamt = rs1 & shift_mask;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(vs2[i] >> shamt) : vd[i]);
    }
}

// 11.7. Vector Narrowing Integer Right Shift Instructions

template <typename T, typename T2>
void vnsr_wv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T2* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    constexpr uint32_t bit_width = sizeof(T2) * 8;
    constexpr uint32_t shift_mask = bit_width - 1;
    for (uint32_t i = vstart; i < vl; i++) {
        uint32_t shamt = static_cast<uint32_t>(vs1[i]) & shift_mask;
        vd[i] = ((mask[i]) ? static_cast<T>(vs2[i] >> shamt) : vd[i]);
    }
}

template <typename T, typename T2>
void vnsr_wxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T2* __restrict vs2, const uint32_t &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    constexpr uint32_t bit_width = sizeof(T2) * 8;
    constexpr uint32_t shift_mask = bit_width - 1;
    uint32_t shamt = rs1 & shift_mask;
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? static_cast<T>(vs2[i] >> shamt) : vd[i]);
    }
}

// 11.8. Vector Integer Compare Instructions

template <typename T, typename OP>
void vmcomp_common_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask, OP comp) {
    for (uint32_t i = vstart; i < vl; i++) {
        bool res = comp(vs2[i], vs1[i]);
        if (mask[i]) set_vmask(vd, i, res);
    }
}

template <typename T, typename OP>
void vmcomp_common_vxi(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask, OP comp) {
    for (uint32_t i = vstart; i < vl; i++) {
        bool res = comp(vs2[i], rs1);
        if (mask[i]) set_vmask(vd, i, res);
    }
}

template <typename T>
void vmseq_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vv<T>(vl, vstart, vd, vs2, vs1, mask, [](T a, T b) { return a == b; });
}

template <typename T>
void vmseq_vxi(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vxi<T>(vl, vstart, vd, vs2, rs1, mask, [](T a, T b) { return a == b; });
}

template <typename T>
void vmsne_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vv<T>(vl, vstart, vd, vs2, vs1, mask, [](T a, T b) { return a != b; });
}

template <typename T>
void vmsne_vxi(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vxi<T>(vl, vstart, vd, vs2, rs1, mask, [](T a, T b) { return a != b; });
}

template <typename T>
void vmslt_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vv<T>(vl, vstart, vd, vs2, vs1, mask, [](T a, T b) { return a < b; });
}

template <typename T>
void vmslt_vxi(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vxi<T>(vl, vstart, vd, vs2, rs1, mask, [](T a, T b) { return a < b; });
}

template <typename T>
void vmsle_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vv<T>(vl, vstart, vd, vs2, vs1, mask, [](T a, T b) { return a <= b; });
}

template <typename T>
void vmsle_vxi(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vxi<T>(vl, vstart, vd, vs2, rs1, mask, [](T a, T b) { return a <= b; });
}

template <typename T>
void vmsgt_vv(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vv<T>(vl, vstart, vd, vs2, vs1, mask, [](T a, T b) { return a > b; });
}

template <typename T>
void vmsgt_vxi(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const T* __restrict vs2, const T& rs1, const ExpdMaskT * __restrict mask) {
    vmcomp_common_vxi<T>(vl, vstart, vd, vs2, rs1, mask, [](T a, T b) { return a > b; });
}

// 11.9. Vector Integer Min/Max Instructions

template <typename T>
void vmin_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? ((vs2[i] < vs1[i]) ? vs2[i] : vs1[i]) : vd[i]);
    }
}

template <typename T>
void vmin_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? ((vs2[i] < rs1) ? vs2[i] : rs1) : vd[i]);
    }
}

template <typename T>
void vmax_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? ((vs2[i] < vs1[i]) ? vs1[i] : vs2[i]) : vd[i]);
    }
}

template <typename T>
void vmax_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? ((vs2[i] < rs1) ? rs1 : vs2[i]) : vd[i]);
    }
}

// 11.10. Vector Single-Width Integer Multiply Instructions

template <typename UT2, typename T2, typename UT, typename T>
UT mulh_impl(const UT &a, const UT &b) {
    T2 aa = static_cast<T2>(static_cast<T>(a));
    T2 bb = static_cast<T2>(static_cast<T>(b));
    UT2 result = static_cast<UT2>((aa * bb) >> (sizeof(T) * 8));
    return static_cast<UT>(result);
}
template <typename UT2, typename T2, typename UT, typename T>
UT mulhu_impl(const UT &a, const UT &b) {
    UT2 aa = static_cast<UT2>((a));
    UT2 bb = static_cast<UT2>((b));
    UT2 result = static_cast<UT2>((aa * bb) >> (sizeof(T) * 8));
    return static_cast<UT>(result);
}
template <typename UT2, typename T2, typename UT, typename T>
UT mulhsu_impl(const UT &a, const UT &b) {
    T2 aa = static_cast<T2>(static_cast<T>(a));
    T2 bb = static_cast<T2>(static_cast<UT2>(b));
    UT2 result = static_cast<UT2>((aa * bb) >> (sizeof(T) * 8));
    return static_cast<UT>(result);
}

template <typename T>
T mulh_wrapper(const T &a, const T &b) {
    if constexpr (sizeof(T) == 1) {
        return static_cast<T>(mulh_impl<uint16_t, int16_t, uint8_t, int8_t>(static_cast<uint8_t>(a), static_cast<uint8_t>(b)));
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(mulh_impl<uint32_t, int32_t, uint16_t, int16_t>(static_cast<uint16_t>(a), static_cast<uint16_t>(b)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(mulh_impl<uint64_t, int64_t, uint32_t, int32_t>(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(mulh_impl<__uint128_t, __int128_t, uint64_t, int64_t>(static_cast<uint64_t>(a), static_cast<uint64_t>(b)));
    }
    return T(0);
}
template <typename T>
T mulhu_wrapper(const T &a, const T &b) {
    if constexpr (sizeof(T) == 1) {
        return static_cast<T>(mulhu_impl<uint16_t, int16_t, uint8_t, int8_t>(static_cast<uint8_t>(a), static_cast<uint8_t>(b)));
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(mulhu_impl<uint32_t, int32_t, uint16_t, int16_t>(static_cast<uint16_t>(a), static_cast<uint16_t>(b)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(mulhu_impl<uint64_t, int64_t, uint32_t, int32_t>(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(mulhu_impl<__uint128_t, __int128_t, uint64_t, int64_t>(static_cast<uint64_t>(a), static_cast<uint64_t>(b)));
    }
    return T(0);
}
template <typename T>
T mulhsu_wrapper(const T &a, const T &b) {
    if constexpr (sizeof(T) == 1) {
        return static_cast<T>(mulhsu_impl<uint16_t, int16_t, uint8_t, int8_t>(static_cast<uint8_t>(a), static_cast<uint8_t>(b)));
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(mulhsu_impl<uint32_t, int32_t, uint16_t, int16_t>(static_cast<uint16_t>(a), static_cast<uint16_t>(b)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(mulhsu_impl<uint64_t, int64_t, uint32_t, int32_t>(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(mulhsu_impl<__uint128_t, __int128_t, uint64_t, int64_t>(static_cast<uint64_t>(a), static_cast<uint64_t>(b)));
    }
    return T(0);
}

template <typename T>
void vmul_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] * vs1[i]) : vd[i]);
    }
}

template <typename T>
void vmul_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (vs2[i] * rs1) : vd[i]);
    }
};

template <typename T>
void vmulh_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? mulh_wrapper<T>(vs2[i], vs1[i]) : vd[i]);
    }
};

template <typename T>
void vmulh_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? mulh_wrapper<T>(vs2[i], rs1) : vd[i]);
    }
};

template <typename T>
void vmulhu_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? mulhu_wrapper<T>(vs2[i], vs1[i]) : vd[i]);
    }
};

template <typename T>
void vmulhu_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? mulhu_wrapper<T>(vs2[i], rs1) : vd[i]);
    }
};

template <typename T>
void vmulhsu_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? mulhsu_wrapper<T>(vs2[i], vs1[i]) : vd[i]);
    }
};

template <typename T>
void vmulhsu_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? mulhsu_wrapper<T>(vs2[i], rs1) : vd[i]);
    }
};

// 11.11. Vector Integer Divide Instructions

template <typename T>
inline T safe_vdiv(T dividend, T divisor) {
    if constexpr (std::is_unsigned_v<T>) {
        return divisor == 0 ? static_cast<T>(~static_cast<T>(0)) : static_cast<T>(dividend / divisor);
    } else {
        constexpr T min_value = static_cast<T>(T{1} << (sizeof(T) * 8 - 1));

        if (divisor == 0) {
            return static_cast<T>(-1);
        }
        if (dividend == min_value && divisor == static_cast<T>(-1)) {
            return dividend;
        }
        return static_cast<T>(dividend / divisor);
    }
}

template <typename T>
inline T safe_vrem(T dividend, T divisor) {
    if constexpr (std::is_unsigned_v<T>) {
        return divisor == 0 ? dividend : static_cast<T>(dividend % divisor);
    } else {
        constexpr T min_value = static_cast<T>(T{1} << (sizeof(T) * 8 - 1));

        if (divisor == 0) {
            return dividend;
        }
        if (dividend == min_value && divisor == static_cast<T>(-1)) {
            return static_cast<T>(0);
        }
        return static_cast<T>(dividend % divisor);
    }
}

template <typename T>
void vdiv_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? safe_vdiv<T>(vs2[i], vs1[i]) : vd[i]);
    }
}

template <typename T>
void vdiv_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? safe_vdiv<T>(vs2[i], rs1) : vd[i]);
    }
}

template <typename T>
void vrem_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? safe_vrem<T>(vs2[i], vs1[i]) : vd[i]);
    }
}

template <typename T>
void vrem_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? safe_vrem<T>(vs2[i], rs1) : vd[i]);
    }
}

// template <typename T>
// void vdiv_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         vd[i] = static_cast<T>((mask[i]) ? (vs2[i] / vs1[i]) : vd[i]);
//     }
// }

// template <typename T>
// void vdiv_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         vd[i] = static_cast<T>((mask[i]) ? (vs2[i] / rs1) : vd[i]);
//     }
// };

// template <typename T>
// void vrem_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         vd[i] = static_cast<T>((mask[i]) ? (vs2[i] % vs1[i]) : vd[i]);
//     }
// }

// template <typename T>
// void vrem_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         vd[i] = static_cast<T>((mask[i]) ? (vs2[i] % rs1) : vd[i]);
//     }
// };

// 11.12. Vector Widening Integer Multiply Instructions

template <typename UT2, typename T2, typename UT, typename T>
UT2 wmul_impl(const UT &a, const UT &b) {
    T2 aa = static_cast<T2>(static_cast<T>(a));
    T2 bb = static_cast<T2>(static_cast<T>(b));
    return static_cast<UT2>(aa * bb);
}
template <typename UT2, typename T2, typename UT, typename T>
UT2 wmulu_impl(const UT &a, const UT &b) {
    UT2 aa = static_cast<UT2>((a));
    UT2 bb = static_cast<UT2>((b));
    return static_cast<UT2>(aa * bb);
}
template <typename UT2, typename T2, typename UT, typename T>
UT2 wmulsu_impl(const UT &a, const UT &b) {
    T2 aa = static_cast<T2>(static_cast<T>(a));
    T2 bb = static_cast<T2>(static_cast<UT2>(b));
    return static_cast<UT2>(aa * bb);
}

template <typename T, typename T2>
T2 wmul_wrapper(const T &a, const T &b) {
    static_assert(sizeof(T2) == sizeof(T) * 2, "T2 must be double the size of T");
    if constexpr (sizeof(T) == 1) {
        return static_cast<T2>(wmul_impl<uint16_t, int16_t, uint8_t, int8_t>(static_cast<uint8_t>(a), static_cast<uint8_t>(b)));
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T2>(wmul_impl<uint32_t, int32_t, uint16_t, int16_t>(static_cast<uint16_t>(a), static_cast<uint16_t>(b)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T2>(wmul_impl<uint64_t, int64_t, uint32_t, int32_t>(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
    }
    return T2(0);
}

template <typename T, typename T2>
T2 wmulu_wrapper(const T &a, const T &b) {
    static_assert(sizeof(T2) == sizeof(T) * 2, "T2 must be double the size of T");
    if constexpr (sizeof(T) == 1) {
        return static_cast<T2>(wmulu_impl<uint16_t, int16_t, uint8_t, int8_t>(static_cast<uint8_t>(a), static_cast<uint8_t>(b)));
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T2>(wmulu_impl<uint32_t, int32_t, uint16_t, int16_t>(static_cast<uint16_t>(a), static_cast<uint16_t>(b)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T2>(wmulu_impl<uint64_t, int64_t, uint32_t, int32_t>(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
    }
    return T2(0);
}

template <typename T, typename T2>
T2 wmulsu_wrapper(const T &a, const T &b) {
    static_assert(sizeof(T2) == sizeof(T) * 2, "T2 must be double the size of T");
    if constexpr (sizeof(T) == 1) {
        return static_cast<T2>(wmulsu_impl<uint16_t, int16_t, uint8_t, int8_t>(static_cast<uint8_t>(a), static_cast<uint8_t>(b)));
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T2>(wmulsu_impl<uint32_t, int32_t, uint16_t, int16_t>(static_cast<uint16_t>(a), static_cast<uint16_t>(b)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T2>(wmulsu_impl<uint64_t, int64_t, uint32_t, int32_t>(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
    }
    return T2(0);
}

template <typename T, typename T2>
void vwmul_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (wmul_wrapper<T, T2>(vs2[i], vs1[i])) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmul_vx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (wmul_wrapper<T, T2>(vs2[i], rs1)) : vd[i]);
    }
};

template <typename T, typename T2>
void vwmulu_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (wmulu_wrapper<T, T2>(vs2[i], vs1[i])) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmulu_vx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (wmulu_wrapper<T, T2>(vs2[i], rs1)) : vd[i]);
    }
};

template <typename T, typename T2>
void vwmulsu_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (wmulsu_wrapper<T, T2>(vs2[i], vs1[i])) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmulsu_vx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = ((mask[i]) ? (wmulsu_wrapper<T, T2>(vs2[i], rs1)) : vd[i]);
    }
};

// 11.13. Vector Single-Width Integer Multiply-Add Instructions

template <typename T>
void vmacc_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? (vd[i] + (vs2[i] * vs1[i])) : vd[i]);
    }
}

template <typename T>
void vmacc_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? (vd[i] + (vs2[i] * rs1)) : vd[i]);
    }
}

template <typename T>
void vnmsac_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? (vd[i] - (vs2[i] * vs1[i])) : vd[i]);
    }
}

template <typename T>
void vnmsac_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? (vd[i] - (vs2[i] * rs1)) : vd[i]);
    }
}

template <typename T>
void vmadd_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? (vs2[i] + (vd[i] * vs1[i])) : vd[i]);
    }
}

template <typename T>
void vmadd_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? (vs2[i] + (vd[i] * rs1)) : vd[i]);
    }
}

template <typename T>
void vnmsub_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? (vs2[i] - (vd[i] * vs1[i])) : vd[i]);
    }
}

template <typename T>
void vnmsub_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? (vs2[i] - (vd[i] * rs1)) : vd[i]);
    }
}

// 11.14. Vector Widening Integer Multiply-Add Instructions

template <typename T, typename T2>
void vwmaccu_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T2>((mask[i]) ? (vd[i] + wmulu_wrapper<T, T2>(vs2[i], vs1[i])) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmaccu_vx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T2>((mask[i]) ? (vd[i] + wmulu_wrapper<T, T2>(vs2[i], rs1)) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmacc_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T2>((mask[i]) ? (vd[i] + wmul_wrapper<T, T2>(vs2[i], vs1[i])) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmacc_vx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T2>((mask[i]) ? (vd[i] + wmul_wrapper<T, T2>(vs2[i], rs1)) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmaccsu_vv(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T2>((mask[i]) ? (vd[i] + wmulsu_wrapper<T, T2>(vs1[i], vs2[i])) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmaccsu_vx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T2>((mask[i]) ? (vd[i] + wmulsu_wrapper<T, T2>(rs1, vs2[i])) : vd[i]);
    }
}

template <typename T, typename T2>
void vwmaccus_vx(const uint32_t & vl, const uint32_t vstart, T2* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    static_assert(sizeof(T2) == 2 * sizeof(T), "T2 must be twice the size of T");
    if constexpr (sizeof(T2) > 8) {
        return; // not defined for 128-bit destination
    }
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T2>((mask[i]) ? (vd[i] + wmulsu_wrapper<T, T2>(vs2[i], rs1)) : vd[i]);
    }
}

// 11.15. Vector Integer Merge Instructions

template <typename T>
void vmerge_vvm(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? vs1[i] : vs2[i]);
    }
}

template <typename T>
void vmerge_vxim(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T &rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        vd[i] = static_cast<T>((mask[i]) ? rs1 : vs2[i]);
    }
}

// 11.16. Vector Integer Move Instructions
// These are implemented as simple memory copies




} // namespace rvvarchsem
