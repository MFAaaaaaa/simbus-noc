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

#if defined(_WIN32)
  #if defined(LIB_BUILDING)
    #define RV64LIB_API __declspec(dllexport)
  #else
    #define RV64LIB_API __declspec(dllimport)
  #endif
#else
  #define RV64LIB_API __attribute__((visibility("default")))
#endif

#include <cstdint>

namespace rv64archsem {

using ExtSetT = uint64_t;

constexpr ExtSetT EXT_BASE    = 1UL << 0;  // Base Integer ISA
constexpr ExtSetT EXT_M       = 1UL << 3;  // Integer Multiplication and Division
constexpr ExtSetT EXT_A       = 1UL << 4;  // Atomic Instructions
constexpr ExtSetT EXT_F       = 1UL << 5;  // Single-Precision Floating-Point
constexpr ExtSetT EXT_D       = 1UL << 6;  // Double-Precision Floating-Point
constexpr ExtSetT EXT_C       = 1UL << 2;  // Compressed Instructions
constexpr ExtSetT EXT_V       = 1UL << 7;  // Vector Instructions
constexpr ExtSetT EXT_B       = 1UL << 8;  // Bit Manipulation
constexpr ExtSetT EXT_J       = 1UL << 9;  // Dynamic Jump Instructions
constexpr ExtSetT EXT_T       = 1UL << 10; // Transactional Memory
constexpr ExtSetT EXT_P       = 1UL << 11; // Packed SIMD Instructions

constexpr ExtSetT SUPPORT_EXTENSION = (EXT_BASE | EXT_M | EXT_A | EXT_F | EXT_D | EXT_C | EXT_V | EXT_B);

constexpr uint32_t RVV_VLEN = 256; // in bits


} // namespace rv64archsem
