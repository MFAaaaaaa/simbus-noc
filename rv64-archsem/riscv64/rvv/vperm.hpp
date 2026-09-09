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
#include "riscv64_config.hpp"

#include <cstdint>
#include <type_traits>

namespace rvvarchsem {

// 16.3. Vector Slide Instructions

template <typename T>
void vslideup_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint32_t & rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            if (i >= rs1) {
                vd[i] = vs2[i - rs1];
            }
        }
    }
}
// template <typename T>
// void vslideup_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint32_t & rs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         if (mask[i]) {
//             if (i < rs1) {
//                 vd[i] = static_cast<T>(0);
//             } else {
//                 vd[i] = vs2[i - rs1];
//             }
//         }
//     }
// }

// template <typename T>
// void vslidedown_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint32_t & rs1, const uint32_t & vlmax, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         if (mask[i]) {
//             if (rs1 >= vlmax || i >= vlmax - rs1) {
//                 vd[i] = static_cast<T>(0);
//             } else {
//                 vd[i] = vs2[i + rs1];
//             }
//         }
//     }
// }
template <typename T>
void vslidedown_vxi(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint32_t & rs1, const uint32_t & vlmax, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            if (rs1 >= vlmax || i >= vlmax - rs1) {
                vd[i] = static_cast<T>(0);
            } else {
                vd[i] = vs2[i + rs1];
            }
        }
    }
}


template <typename T>
void vslide1up_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T & rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            if (i == 0) {
                vd[i] = rs1;
            } else {
                vd[i] = vs2[i - 1];
            }
        }
    }
}

template <typename T>
void vslide1down_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T & rs1, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            if (i == vl - 1) {
                vd[i] = rs1;
            } else {
                vd[i] = vs2[i + 1];
            }
        }
    }
}

// 16.4. Vector Register Gather Instructions

// template <typename T>
// void vrgather_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T* __restrict vs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         if (mask[i]) {
//             uint32_t index = static_cast<uint32_t>(vs1[i]);
//             // vd[i] = (index < vl) ? vs2[index] : static_cast<T>(0);
//             vd[i] = (index < vlmax) ? vs2[index] : static_cast<T>(0);
//         }
//     }
// }
template <typename T>
void vrgather_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd,
                 const T* __restrict vs2, const T* __restrict vs1,
                 const uint32_t & vlmax, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            uint32_t index = static_cast<uint32_t>(vs1[i]);
            vd[i] = (index < vlmax) ? vs2[index] : static_cast<T>(0);
        }
    }
}

template <typename T>
void vrgather_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd,
                 const T* __restrict vs2, const T & rs1,
                 const uint32_t & vlmax, const ExpdMaskT * __restrict mask) {
    uint32_t index = static_cast<uint32_t>(rs1);
    T val = (index < vlmax) ? vs2[index] : static_cast<T>(0);
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            vd[i] = val;
        }
    }
}
// template <typename T>
// void vrgather_vx(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const T & rs1, const ExpdMaskT * __restrict mask) {
//     uint32_t index = static_cast<uint32_t>(rs1);
//     // T val = (index < vl) ? vs2[index] : static_cast<T>(0);
//     T val = (index < vlmax) ? vs2[index] : static_cast<T>(0);
//     for (uint32_t i = vstart; i < vl; i++) {
//         if (mask[i]) {
//             vd[i] = val;
//         }
//     }
// }

template <typename T>
void vrgatherei16_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd,
                     const T* __restrict vs2, const uint16_t* __restrict vs1,
                     const uint32_t & vlmax, const ExpdMaskT * __restrict mask) {
    for (uint32_t i = vstart; i < vl; i++) {
        if (mask[i]) {
            uint32_t index = static_cast<uint32_t>(vs1[i]);
            vd[i] = (index < vlmax) ? vs2[index] : static_cast<T>(0);
        }
    }
}
// template <typename T>
// void vrgatherei16_vv(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint16_t* __restrict vs1, const ExpdMaskT * __restrict mask) {
//     for (uint32_t i = vstart; i < vl; i++) {
//         if (mask[i]) {
//             uint32_t index = static_cast<uint32_t>(vs1[i]);
//             // vd[i] = (index < vl) ? vs2[index] : static_cast<T>(0);
//             vd[i] = (index < vlmax) ? vs2[index] : static_cast<T>(0);
//         }
//     }
// }

// 16.5. Vector Compress Instruction

template <typename T>
void vcompress_vm(const uint32_t & vl, const uint32_t vstart, T* __restrict vd, const T* __restrict vs2, const uint8_t* __restrict vs1) {
    uint32_t dest_index = 0;
    for (uint32_t i = vstart; i < vl; i++) {
        if (vs1[i/8] & (1 << (i%8))) {
            vd[dest_index++] = vs2[i];
        }
    }
}


} // namespace rvvarchsem
