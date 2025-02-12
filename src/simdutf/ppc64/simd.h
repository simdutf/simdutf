#ifndef SIMDUTF_PPC64_SIMD_H
#define SIMDUTF_PPC64_SIMD_H

#include "simdutf.h"
#include "simdutf/ppc64/bitmanipulation.h"
#include <type_traits>

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace simd {

using vec_bool_t = __vector __bool char;
using vec_bool16_t = __vector __bool short;
using vec_bool32_t = __vector __bool int;
using vec_u8_t = __vector unsigned char;
using vec_i8_t = __vector signed char;
using vec_u16_t = __vector unsigned short;
using vec_i16_t = __vector signed short;
using vec_u32_t = __vector unsigned int;
using vec_i32_t = __vector signed int;
using vec_u64_t = __vector unsigned long long;
using vec_i64_t = __vector signed long long;

// clang-format off
template <typename T> struct vector_u8_type_for_element_aux {
  using type = typename std::conditional<std::is_same<T, bool>::value,    vec_bool_t,
               typename std::conditional<std::is_same<T, uint8_t>::value, vec_u8_t,
               typename std::conditional<std::is_same<T, int8_t>::value,  vec_i8_t, void>::type>::type>::type;

  static_assert(not std::is_same<type, void>::value,
                "accepted element types are 8 bit integers or bool");
};

template <typename T> struct vector_u16_type_for_element_aux {
  using type = typename std::conditional<std::is_same<T, bool>::value,     vec_bool16_t,
               typename std::conditional<std::is_same<T, uint16_t>::value, vec_u16_t,
               typename std::conditional<std::is_same<T, int16_t>::value,  vec_i16_t, void>::type>::type>::type;

  static_assert(not std::is_same<type, void>::value,
                "accepted element types are 16 bit integers or bool");
};

template <typename T> struct vector_u32_type_for_element_aux {
  using type = typename std::conditional<std::is_same<T, bool>::value,     vec_bool32_t,
               typename std::conditional<std::is_same<T, uint32_t>::value, vec_u32_t,
               typename std::conditional<std::is_same<T, int32_t>::value,  vec_i32_t, void>::type>::type>::type;

  static_assert(not std::is_same<type, void>::value,
                "accepted element types are 32 bit integers or bool");
};
// clang-format on

template <typename T>
using vector_u8_type_for_element =
    typename vector_u8_type_for_element_aux<T>::type;

template <typename T>
using vector_u16_type_for_element =
    typename vector_u16_type_for_element_aux<T>::type;

template <typename T>
using vector_u32_type_for_element =
    typename vector_u32_type_for_element_aux<T>::type;

template <typename T> bool move_mask_u8(T vec) {
  const vec_u8_t perm_mask = {15 * 8, 14 * 8, 13 * 8, 12 * 8, 11 * 8, 10 * 8,
                              9 * 8,  8 * 8,  7 * 8,  6 * 8,  5 * 8,  4 * 8,
                              3 * 8,  2 * 8,  1 * 8,  0 * 8};

  const auto result = (vec_u64_t)vec_vbpermq((vec_u8_t)vec, perm_mask);
#ifdef __LITTLE_ENDIAN__
  return static_cast<int>(result[1]);
#else
  return static_cast<int>(result[0]);
#endif
}

template <typename T>
T select(const T cond, const T val_true, const T val_false) {
  return vec_sel(val_false.value, val_true.value, cond.value);
}

#include "simd8-inl.h"
#include "simd16-inl.h"
#include "simd32-inl.h"

using vector_u8 = simd8<uint8_t>;
using vector_u16 = simd16<uint16_t>;
using vector_u32 = simd32<uint32_t>;

simdutf_really_inline vector_u16 as_vector_u16(const vector_u8 v) {
  return vector_u16::vector_type(v.value);
}

simdutf_really_inline vector_u32 as_vector_u32(const vector_u8 v) {
  return vector_u32::vector_type(v.value);
}

} // namespace simd
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#endif // SIMDUTF_PPC64_SIMD_INPUT_H
