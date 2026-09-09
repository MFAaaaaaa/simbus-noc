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

namespace rvvarchsem {


template <typename T>
struct _uint_of_size {
    static_assert(sizeof(T) <= 16, "Unsupported type size");
    static_assert(std::is_integral<T>::value, "T must be an integral type");
    static_assert(!std::is_same_v<T, bool>, "T must not be bool");
    using _type = std::conditional_t< sizeof(T) == 1, uint8_t,
                  std::conditional_t<sizeof(T) == 2, uint16_t,
                  std::conditional_t<sizeof(T) == 4, uint32_t,
                  std::conditional_t<sizeof(T) == 8, uint64_t,
                  __uint128_t> > > > ;
};
template <typename T>
struct _sint_of_size {
    static_assert(sizeof(T) <= 16, "Unsupported type size");
    static_assert(std::is_integral<T>::value, "T must be an integral type");
    static_assert(!std::is_same_v<T, bool>, "T must not be bool");
    using _type = std::conditional_t< sizeof(T) == 1, int8_t,
                  std::conditional_t<sizeof(T) == 2, int16_t,
                  std::conditional_t<sizeof(T) == 4, int32_t,
                  std::conditional_t<sizeof(T) == 8, int64_t,
                  __int128_t> > > > ;
};
template <typename T>
struct _uint_double_of_size {
    static_assert(sizeof(T) <= 8, "Unsupported type size");
    static_assert(std::is_integral<T>::value, "T must be an integral type");
    static_assert(!std::is_same_v<T, bool>, "T must not be bool");
    using _type = std::conditional_t< sizeof(T) == 1, uint16_t,
                  std::conditional_t<sizeof(T) == 2, uint32_t,
                  std::conditional_t<sizeof(T) == 4, uint64_t,
                  __uint128_t> > > ;
};
template <typename T>
struct _sint_double_of_size {
    static_assert(sizeof(T) <= 8, "Unsupported type size");
    static_assert(std::is_integral<T>::value, "T must be an integral type");
    static_assert(!std::is_same_v<T, bool>, "T must not be bool");
    using _type = std::conditional_t< sizeof(T) == 1, int16_t,
                  std::conditional_t<sizeof(T) == 2, int32_t,
                  std::conditional_t<sizeof(T) == 4, int64_t,
                  __int128_t> > > ;
};

template <typename T>
using UIntType = typename _uint_of_size<T>::_type;
template <typename T>
using SIntType = typename _sint_of_size<T>::_type;
template <typename T>
using UInt2WidType = typename _uint_double_of_size<T>::_type;
template <typename T>
using SInt2WidType = typename _sint_double_of_size<T>::_type;

struct alignas(1) ExpdMaskT {
    uint8_t value;

    inline operator bool() const {
        return value != 0;
    }

    inline ExpdMaskT& operator=(const bool& b) {
        value = b ? 1 : 0;
        return *this;
    }
};

inline void expand_mask(const uint32_t &vl, ExpdMaskT* __restrict expanded_mask, const uint8_t* __restrict packed_mask) {
    for (uint32_t i = 0; i < vl; i++) {
        expanded_mask[i] = (packed_mask[i / 8] >> (i % 8)) & 0x1;
    }
}

inline void set_vmask(uint8_t * packed_mask, const uint32_t idx, const bool value) {
    packed_mask[idx / 8] &= static_cast<uint8_t>(~(1 << (idx % 8)));
    packed_mask[idx / 8] |= static_cast<uint8_t>((value ? 1 : 0) << (idx % 8));
}

inline void clear_vmask(uint8_t * packed_mask, const uint32_t vl) {
    uint32_t bytes = vl / 8;  
    uint32_t remain_bits = vl % 8;
    for (uint32_t i = 0; i < bytes; i++) {
        packed_mask[i] = 0;
    }
    if (remain_bits != 0) {
        packed_mask[bytes] &= static_cast<uint8_t>(~((1 << remain_bits) - 1));
    } 
}

} // namespace rvvarchsem
