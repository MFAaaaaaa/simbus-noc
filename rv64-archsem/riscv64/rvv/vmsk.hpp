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

template <typename OP>
inline void vmskop_common_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1, OP op) {
    uint32_t bytes = vl / 8;
    uint32_t remain_bits = vl % 8;
    for (uint32_t i = vstart; i < bytes; i++) {
        vd[i] = op(vs2[i], vs1[i]);
    }
    if (remain_bits == 0) return;
    uint8_t mask = static_cast<uint8_t>(((1 << remain_bits) - 1) & 0xFF);
    vd[bytes] = (vd[bytes] & (~mask)) | (op(vs2[bytes], vs1[bytes]) & mask);
}

// 15.1. Vector Mask-Register Logical Instructions

inline void vmand_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1) {
    vmskop_common_mm(vl, vstart, vd, vs2, vs1, [](const uint8_t & a, const uint8_t & b) -> uint8_t { return a & b; });
}

inline void vmnand_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1) {
    vmskop_common_mm(vl, vstart, vd, vs2, vs1, [](const uint8_t & a, const uint8_t & b) -> uint8_t { return ~(a & b); });
}

inline void vmandn_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1) {
    vmskop_common_mm(vl, vstart, vd, vs2, vs1, [](const uint8_t & a, const uint8_t & b) -> uint8_t { return a & (~b); });
}

inline void vmxor_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1) {
    vmskop_common_mm(vl, vstart, vd, vs2, vs1, [](const uint8_t & a, const uint8_t & b) -> uint8_t { return a ^ b; });
}

inline void vmor_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1) {
    vmskop_common_mm(vl, vstart, vd, vs2, vs1, [](const uint8_t & a, const uint8_t & b) -> uint8_t { return a | b; });
}

inline void vmnor_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1) {
    vmskop_common_mm(vl, vstart, vd, vs2, vs1, [](const uint8_t & a, const uint8_t & b) -> uint8_t { return ~(a | b); });
}

inline void vmorn_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1) {
    vmskop_common_mm(vl, vstart, vd, vs2, vs1, [](const uint8_t & a, const uint8_t & b) -> uint8_t { return a | (~b); });
}

inline void vmxnor_mm(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const uint8_t* __restrict vs1) {
    vmskop_common_mm(vl, vstart, vd, vs2, vs1, [](const uint8_t & a, const uint8_t & b) -> uint8_t { return ~(a ^ b); });
}

// 15.2. Vector count population in mask vcpop.m

inline uint64_t vcpop_m(const uint32_t & vl, const uint32_t vstart, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
    uint64_t count = 0;
    for (uint32_t i = vstart; i < vl; i++) {
        if (vs2[i/8] & (1 << (i%8)) && (mask[i])) {
            count++;
        }
    }
    return count;
}

// 15.3. vfirst  nd- rst-set mask bit

inline uint64_t vfirst_m(const uint32_t & vl, const uint32_t vstart, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
    for (uint64_t i = vstart; i < vl; i++) {
        if ((vs2[i/8] & (1 << (i%8))) && (mask[i])) {
            return static_cast<uint64_t>(i);
        }
    }
    return (~0ULL);
}

// 15.4. vmsbf.m set-before-first mask bit

inline void vmsbf_m(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
    bool found = false;
    for (uint32_t i = vstart; i < vl; i++) {
        if (!mask[i]) continue;
        const bool bit = (vs2[i/8] & (1 << (i%8))) != 0;
        set_vmask(vd, i, !found && !bit);
        if (bit) found = true;
    }
}
// inline void vmsbf_m(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
//     clear_vmask(vd, vl);
//     for (uint32_t i = vstart; i < vl; i++) {
//         if ((vs2[i/8] & (1 << (i%8))) && (mask[i])) {
//             break;
//         }
//         set_vmask(vd, i, true);
//     }
// }

// 15.5. vmsif.m set-in-first mask bit

inline void vmsif_m(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
    bool found = false;
    for (uint32_t i = vstart; i < vl; i++) {
        if (!mask[i]) continue;
        const bool bit = (vs2[i/8] & (1 << (i%8))) != 0;
        set_vmask(vd, i, !found);
        if (bit) found = true;
    }
}
// inline void vmsif_m(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
//     clear_vmask(vd, vl);
//     for (uint32_t i = vstart; i < vl; i++) {
//         set_vmask(vd, i, true);
//         if ((vs2[i/8] & (1 << (i%8))) && (mask[i])) {
//             break;
//         }
//     }
// }

// 15.6. vmsof.m set-only-first mask bit

inline void vmsof_m(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
    bool found = false;
    for (uint32_t i = vstart; i < vl; i++) {
        if (!mask[i]) continue;
        const bool bit = (vs2[i/8] & (1 << (i%8))) != 0;
        set_vmask(vd, i, !found && bit);
        if (bit) found = true;
    }
}
// inline void vmsof_m(const uint32_t & vl, const uint32_t vstart, uint8_t* __restrict vd, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
//     clear_vmask(vd, vl);
//     for (uint32_t i = vstart; i < vl; i++) {
//         if ((vs2[i/8] & (1 << (i%8))) && (mask[i])) {
//             set_vmask(vd, i, true);
//             break;
//         }
//     }
// }

// 15.8. Vector Iota Instruction

template <typename T>
void viota_m(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const uint8_t* __restrict vs2, const ExpdMaskT* __restrict mask) {
    uint64_t counter = 0;
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            vd[i] = static_cast<T>(counter);
            if (vs2[i/8] & (1 << (i%8))) {
                counter++;
            }
        }
    }
}

// 15.9. Vector Element Index Instruction

template <typename T>
void vid_v(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const ExpdMaskT* __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i/8] & (1 << (i%8))) {
            vd[i] = static_cast<T>(i);
        }
    }
}


} // namespace rvvarchsem
