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

template <typename T> uint16_t move_mask_u8(T vec) {
  const vec_u8_t perm_mask = {15 * 8, 14 * 8, 13 * 8, 12 * 8, 11 * 8, 10 * 8,
                              9 * 8,  8 * 8,  7 * 8,  6 * 8,  5 * 8,  4 * 8,
                              3 * 8,  2 * 8,  1 * 8,  0 * 8};

  const auto result = (vec_u64_t)vec_vbpermq((vec_u8_t)vec, perm_mask);
#if SIMDUTF_IS_BIG_ENDIAN
  return static_cast<uint16_t>(result[0]);
#else
  return static_cast<uint16_t>(result[1]);
#endif
}

#include "simdutf/ppc64/simd8-inl.h"
#include "simdutf/ppc64/simd16-inl.h"
#include "simdutf/ppc64/simd32-inl.h"

template <typename T>
simd8<T> select(const simd8<T> cond, const simd8<T> val_true,
                const simd8<T> val_false) {
  return vec_sel(val_false.value, val_true.value, cond.value);
}

template <typename T>
simd8<T> select(const T cond, const simd8<T> val_true,
                const simd8<T> val_false) {
  return vec_sel(val_false.value, val_true.value, vec_splats(cond));
}

template <typename T>
simd16<T> select(const simd16<T> cond, const simd16<T> val_true,
                 const simd16<T> val_false) {
  return vec_sel(val_false.value, val_true.value, cond.value);
}

template <typename T>
simd16<T> select(const T cond, const simd16<T> val_true,
                 const simd16<T> val_false) {
  return vec_sel(val_false.value, val_true.value, vec_splats(cond));
}

template <typename T>
simd32<T> select(const simd32<T> cond, const simd32<T> val_true,
                 const simd32<T> val_false) {
  return vec_sel(val_false.value, val_true.value, cond.value);
}

template <typename T>
simd32<T> select(const T cond, const simd32<T> val_true,
                 const simd32<T> val_false) {
  return vec_sel(val_false.value, val_true.value, vec_splats(cond));
}

using vector_u8 = simd8<uint8_t>;
using vector_u16 = simd16<uint16_t>;
using vector_u32 = simd32<uint32_t>;
using vector_i8 = simd8<int8_t>;

simdutf_really_inline vector_u8 as_vector_u8(const vector_u16 v) {
  return vector_u8::vector_type(v.value);
}

simdutf_really_inline vector_u8 as_vector_u8(const vector_u32 v) {
  return vector_u8::vector_type(v.value);
}

simdutf_really_inline vector_u8 as_vector_u8(const vector_i8 v) {
  return vector_u8::vector_type(v.value);
}

simdutf_really_inline vector_u8 as_vector_u8(const simd16<bool> v) {
  return vector_u8::vector_type(v.value);
}

simdutf_really_inline vector_i8 as_vector_i8(const vector_u8 v) {
  return vector_i8::vector_type(v.value);
}

simdutf_really_inline vector_u16 as_vector_u16(const vector_u8 v) {
  return vector_u16::vector_type(v.value);
}

simdutf_really_inline vector_u16 as_vector_u16(const simd16<bool> v) {
  return vector_u16::vector_type(v.value);
}

simdutf_really_inline vector_u32 as_vector_u32(const vector_u8 v) {
  return vector_u32::vector_type(v.value);
}

simdutf_really_inline vector_u32 as_vector_u32(const vector_u16 v) {
  return vector_u32::vector_type(v.value);
}

simdutf_really_inline vector_u32 max(vector_u32 a, vector_u32 b) {
  return vec_max(a.value, b.value);
}

simdutf_really_inline vector_u32 max(vector_u32 a, vector_u32 b, vector_u32 c) {
  return max(max(a, b), c);
}

simdutf_really_inline vector_u32 sum4bytes(vector_u8 bytes, vector_u32 acc) {
  return vec_sum4s(bytes.value, acc.value);
}

} // namespace simd
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#endif // SIMDUTF_PPC64_SIMD_INPUT_H
