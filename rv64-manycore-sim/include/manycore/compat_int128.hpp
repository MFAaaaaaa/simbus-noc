#pragma once

#include <cstring>
#include <type_traits>

#if defined(__SIZEOF_INT128__)
namespace std {
template <> struct is_integral<__int128_t> : true_type {};
template <> struct is_integral<__uint128_t> : true_type {};
template <> struct is_signed<__int128_t> : true_type {};
template <> struct is_signed<__uint128_t> : false_type {};
template <> struct is_unsigned<__int128_t> : false_type {};
template <> struct is_unsigned<__uint128_t> : true_type {};

#if !defined(__cpp_lib_bit_cast)
template <class To, class From>
To bit_cast(const From& src) noexcept {
  static_assert(sizeof(To) == sizeof(From), "bit_cast requires same size");
  static_assert(is_trivially_copyable<To>::value, "destination must be trivially copyable");
  static_assert(is_trivially_copyable<From>::value, "source must be trivially copyable");
  To dst;
  std::memcpy(&dst, &src, sizeof(To));
  return dst;
}
#endif
}  // namespace std
#endif
