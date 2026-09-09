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

#include <cstdint>
#include <type_traits>
#include <limits>

template <typename To, typename From>
inline To int_cast(From value) {
    static_assert(std::is_integral<From>::value, "From must be integral");
    static_assert(std::is_integral<To>::value, "To must be integral");
    static_assert(sizeof(From) <= sizeof(uint64_t), "From size too large");
    static_assert(sizeof(To) <= sizeof(uint64_t), "To size too large");

    constexpr size_t from_bits = sizeof(From) * 8;
    constexpr size_t to_bits   = sizeof(To) * 8;

    if constexpr (to_bits < from_bits) {
        // 长转短：按低位截断
        return static_cast<To>(value & static_cast<From>((1ULL << to_bits) - 1));
    } else if constexpr (to_bits > from_bits) {
        // 短转长
        if constexpr (std::is_signed<From>::value) {
            // 符号扩展
            return static_cast<To>(static_cast<std::make_signed_t<To>>(value));
        } else {
            // 无符号扩展（零扩展）
            return static_cast<To>(value);
        }
    } else {
        // 等宽直接 cast
        return static_cast<To>(value);
    }
}

