#ifndef SIMDUTF_ARM64_SIMD_H
#define SIMDUTF_ARM64_SIMD_H

#include "simdutf.h"
#include "simdutf/arm64/bitmanipulation.h"
#include <type_traits>


namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace simd {

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
namespace {
// Start of private section with Visual Studio workaround


/**
 * make_uint8x16_t initializes a SIMD register (uint8x16_t).
 * This is needed because, incredibly, the syntax uint8x16_t x = {1,2,3...}
 * is not recognized under Visual Studio! This is a workaround.
 * Using a std::initializer_list<uint8_t>  as a parameter resulted in
 * inefficient code. With the current approach, if the parameters are
 * compile-time constants,
 * GNU GCC compiles it to ldr, the same as uint8x16_t x = {1,2,3...}.
 * You should not use this function except for compile-time constants:
 * it is not efficient.
 */
simdutf_really_inline uint8x16_t make_uint8x16_t(uint8_t x1,  uint8_t x2,  uint8_t x3,  uint8_t x4,
                                         uint8_t x5,  uint8_t x6,  uint8_t x7,  uint8_t x8,
                                         uint8_t x9,  uint8_t x10, uint8_t x11, uint8_t x12,
                                         uint8_t x13, uint8_t x14, uint8_t x15, uint8_t x16) {
  // Doing a load like so end ups generating worse code.
  // uint8_t array[16] = {x1, x2, x3, x4, x5, x6, x7, x8,
  //                     x9, x10,x11,x12,x13,x14,x15,x16};
  // return vld1q_u8(array);
  uint8x16_t x{};
  // incredibly, Visual Studio does not allow x[0] = x1
  x = vsetq_lane_u8(x1, x, 0);
  x = vsetq_lane_u8(x2, x, 1);
  x = vsetq_lane_u8(x3, x, 2);
  x = vsetq_lane_u8(x4, x, 3);
  x = vsetq_lane_u8(x5, x, 4);
  x = vsetq_lane_u8(x6, x, 5);
  x = vsetq_lane_u8(x7, x, 6);
  x = vsetq_lane_u8(x8, x, 7);
  x = vsetq_lane_u8(x9, x, 8);
  x = vsetq_lane_u8(x10, x, 9);
  x = vsetq_lane_u8(x11, x, 10);
  x = vsetq_lane_u8(x12, x, 11);
  x = vsetq_lane_u8(x13, x, 12);
  x = vsetq_lane_u8(x14, x, 13);
  x = vsetq_lane_u8(x15, x, 14);
  x = vsetq_lane_u8(x16, x, 15);
  return x;
}

// We have to do the same work for make_int8x16_t
simdutf_really_inline int8x16_t make_int8x16_t(int8_t x1,  int8_t x2,  int8_t x3,  int8_t x4,
                                       int8_t x5,  int8_t x6,  int8_t x7,  int8_t x8,
                                       int8_t x9,  int8_t x10, int8_t x11, int8_t x12,
                                       int8_t x13, int8_t x14, int8_t x15, int8_t x16) {
  // Doing a load like so end ups generating worse code.
  // int8_t array[16] = {x1, x2, x3, x4, x5, x6, x7, x8,
  //                     x9, x10,x11,x12,x13,x14,x15,x16};
  // return vld1q_s8(array);
  int8x16_t x{};
  // incredibly, Visual Studio does not allow x[0] = x1
  x = vsetq_lane_s8(x1, x, 0);
  x = vsetq_lane_s8(x2, x, 1);
  x = vsetq_lane_s8(x3, x, 2);
  x = vsetq_lane_s8(x4, x, 3);
  x = vsetq_lane_s8(x5, x, 4);
  x = vsetq_lane_s8(x6, x, 5);
  x = vsetq_lane_s8(x7, x, 6);
  x = vsetq_lane_s8(x8, x, 7);
  x = vsetq_lane_s8(x9, x, 8);
  x = vsetq_lane_s8(x10, x, 9);
  x = vsetq_lane_s8(x11, x, 10);
  x = vsetq_lane_s8(x12, x, 11);
  x = vsetq_lane_s8(x13, x, 12);
  x = vsetq_lane_s8(x14, x, 13);
  x = vsetq_lane_s8(x15, x, 14);
  x = vsetq_lane_s8(x16, x, 15);
  return x;
}

simdutf_really_inline uint8x8_t make_uint8x8_t(uint8_t x1,  uint8_t x2,  uint8_t x3,  uint8_t x4,
                                       uint8_t x5,  uint8_t x6,  uint8_t x7,  uint8_t x8) {
  uint8x8_t x{};
  x = vsetq_lane_u8(x1, x, 0);
  x = vsetq_lane_u8(x2, x, 1);
  x = vsetq_lane_u8(x3, x, 2);
  x = vsetq_lane_u8(x4, x, 3);
  x = vsetq_lane_u8(x5, x, 4);
  x = vsetq_lane_u8(x6, x, 5);
  x = vsetq_lane_u8(x7, x, 6);
  x = vsetq_lane_u8(x8, x, 7);;
  return x;
}

simdutf_really_inline uint16x8_t make_uint16x8_t(uint16_t x1,  uint16_t x2,  uint16_t x3,  uint16_t x4,
                                       uint16_t x5,  uint16_t x6,  uint16_t x7,  uint16_t x8) {
  uint16x8_t x{};
  x = vsetq_lane_u16(x1, x, 0);
  x = vsetq_lane_u16(x2, x, 1);
  x = vsetq_lane_u16(x3, x, 2);
  x = vsetq_lane_u16(x4, x, 3);
  x = vsetq_lane_u16(x5, x, 4);
  x = vsetq_lane_u16(x6, x, 5);
  x = vsetq_lane_u16(x7, x, 6);
  x = vsetq_lane_u16(x8, x, 7);;
  return x;
}

simdutf_really_inline int16x8_t make_int16x8_t(int16_t x1,  int16_t x2,  int16_t x3,  int16_t x4,
                                       int16_t x5,  int16_t x6,  int16_t x7,  int16_t x8) {
  uint16x8_t x{};
  x = vsetq_lane_s16(x1, x, 0);
  x = vsetq_lane_s16(x2, x, 1);
  x = vsetq_lane_s16(x3, x, 2);
  x = vsetq_lane_s16(x4, x, 3);
  x = vsetq_lane_s16(x5, x, 4);
  x = vsetq_lane_s16(x6, x, 5);
  x = vsetq_lane_s16(x7, x, 6);
  x = vsetq_lane_s16(x8, x, 7);;
  return x;
}


// End of private section with Visual Studio workaround
} // namespace
#endif // SIMDUTF_REGULAR_VISUAL_STUDIO


  template<typename T>
  struct simd8;

  //
  // Base class of simd8<uint8_t> and simd8<bool>, both of which use uint8x16_t internally.
  //
  template<typename T, typename Mask=simd8<bool>>
  struct base_u8 {
    uint8x16_t value;
    static const int SIZE = sizeof(value);

    // Conversion from/to SIMD register
    simdutf_really_inline base_u8(const uint8x16_t _value) : value(_value) {}
    simdutf_really_inline operator const uint8x16_t&() const { return this->value; }
    simdutf_really_inline operator uint8x16_t&() { return this->value; }
    simdutf_really_inline T first() const { return vgetq_lane_u8(*this,0); }
    simdutf_really_inline T last() const { return vgetq_lane_u8(*this,15); }

    // Bit operations
    simdutf_really_inline simd8<T> operator|(const simd8<T> other) const { return vorrq_u8(*this, other); }
    simdutf_really_inline simd8<T> operator&(const simd8<T> other) const { return vandq_u8(*this, other); }
    simdutf_really_inline simd8<T> operator^(const simd8<T> other) const { return veorq_u8(*this, other); }
    simdutf_really_inline simd8<T> bit_andnot(const simd8<T> other) const { return vbicq_u8(*this, other); }
    simdutf_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }
    simdutf_really_inline simd8<T>& operator|=(const simd8<T> other) { auto this_cast = static_cast<simd8<T>*>(this); *this_cast = *this_cast | other; return *this_cast; }
    simdutf_really_inline simd8<T>& operator&=(const simd8<T> other) { auto this_cast = static_cast<simd8<T>*>(this); *this_cast = *this_cast & other; return *this_cast; }
    simdutf_really_inline simd8<T>& operator^=(const simd8<T> other) { auto this_cast = static_cast<simd8<T>*>(this); *this_cast = *this_cast ^ other; return *this_cast; }

    simdutf_really_inline Mask operator==(const simd8<T> other) const { return vceqq_u8(*this, other); }

    template<int N=1>
    simdutf_really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return vextq_u8(prev_chunk, *this, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base_u8<bool> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    static simdutf_really_inline simd8<bool> splat(bool _value) { return vmovq_n_u8(uint8_t(-(!!_value))); }

    simdutf_really_inline simd8(const uint8x16_t _value) : base_u8<bool>(_value) {}
    // False constructor
    simdutf_really_inline simd8() : simd8(vdupq_n_u8(0)) {}
    // Splat constructor
    simdutf_really_inline simd8(bool _value) : simd8(splat(_value)) {}
    simdutf_really_inline void store(uint8_t dst[16]) const { return vst1q_u8(dst, *this); }

    // We return uint32_t instead of uint16_t because that seems to be more efficient for most
    // purposes (cutting it down to uint16_t costs performance in some compilers).
    simdutf_really_inline uint32_t to_bitmask() const {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
      const uint8x16_t bit_mask =  make_uint8x16_t(0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                                   0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80);
#else
      const uint8x16_t bit_mask =  {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                    0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
#endif
      auto minput = *this & bit_mask;
      uint8x16_t tmp = vpaddq_u8(minput, minput);
      tmp = vpaddq_u8(tmp, tmp);
      tmp = vpaddq_u8(tmp, tmp);
      return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
    }

    // Returns 4-bit out of each byte, alternating between the high 4 bits and low bits
    // result it is 64 bit.
    // This method is expected to be faster than none() and is equivalent
    // when the vector register is the result of a comparison, with byte
    // values 0xff and 0x00.
    simdutf_really_inline uint64_t to_bitmask64() const {
      return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(*this), 4)), 0);
    }

    simdutf_really_inline bool any() const { return vmaxvq_u8(*this) != 0; }
    simdutf_really_inline bool none() const { return vmaxvq_u8(*this) == 0; }
    simdutf_really_inline bool all() const { return vminvq_u8(*this) == 0xFF; }


  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base_u8<uint8_t> {
    static simdutf_really_inline simd8<uint8_t> splat(uint8_t _value) { return vmovq_n_u8(_value); }
    static simdutf_really_inline simd8<uint8_t> zero() { return vdupq_n_u8(0); }
    static simdutf_really_inline simd8<uint8_t> load(const uint8_t* values) { return vld1q_u8(values); }
    simdutf_really_inline simd8(const uint8x16_t _value) : base_u8<uint8_t>(_value) {}
    // Zero constructor
    simdutf_really_inline simd8() : simd8(zero()) {}
    // Array constructor
    simdutf_really_inline simd8(const uint8_t values[16]) : simd8(load(values)) {}
    // Splat constructor
    simdutf_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Member-by-member initialization
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    simdutf_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(make_uint8x16_t(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
#else
    simdutf_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(uint8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
#endif

    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdutf_really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Store to array
    simdutf_really_inline void store(uint8_t dst[16]) const { return vst1q_u8(dst, *this); }

    // Saturated math
    simdutf_really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return vqaddq_u8(*this, other); }
    simdutf_really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return vqsubq_u8(*this, other); }

    // Addition/subtraction are the same for signed and unsigned
    simdutf_really_inline simd8<uint8_t> operator+(const simd8<uint8_t> other) const { return vaddq_u8(*this, other); }
    simdutf_really_inline simd8<uint8_t> operator-(const simd8<uint8_t> other) const { return vsubq_u8(*this, other); }
    simdutf_really_inline simd8<uint8_t>& operator+=(const simd8<uint8_t> other) { *this = *this + other; return *this; }
    simdutf_really_inline simd8<uint8_t>& operator-=(const simd8<uint8_t> other) { *this = *this - other; return *this; }

    // Order-specific operations
    simdutf_really_inline uint8_t max_val() const { return vmaxvq_u8(*this); }
    simdutf_really_inline uint8_t min_val() const { return vminvq_u8(*this); }
    simdutf_really_inline simd8<uint8_t> max_val(const simd8<uint8_t> other) const { return vmaxq_u8(*this, other); }
    simdutf_really_inline simd8<uint8_t> min_val(const simd8<uint8_t> other) const { return vminq_u8(*this, other); }
    simdutf_really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return vcleq_u8(*this, other); }
    simdutf_really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return vcgeq_u8(*this, other); }
    simdutf_really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return vcltq_u8(*this, other); }
    simdutf_really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return vcgtq_u8(*this, other); }
    // Same as >, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    simdutf_really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this > other); }
    // Same as <, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    simdutf_really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this < other); }

    // Bit-specific operations
    simdutf_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return vtstq_u8(*this, bits); }
    simdutf_really_inline bool is_ascii() const { return this->max_val() < 0b10000000u; }

    simdutf_really_inline bool any_bits_set_anywhere() const { return this->max_val() != 0; }
    simdutf_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return (*this & bits).any_bits_set_anywhere(); }
    template<int N>
    simdutf_really_inline simd8<uint8_t> shr() const { return vshrq_n_u8(*this, N); }
    template<int N>
    simdutf_really_inline simd8<uint8_t> shl() const { return vshlq_n_u8(*this, N); }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdutf_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }


    template<typename L>
    simdutf_really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }

    template<typename T>
    simdutf_really_inline simd8<uint8_t> apply_lookup_16_to(const simd8<T> original) const {
      return vqtbl1q_u8(*this, simd8<uint8_t>(original));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> {
    int8x16_t value;

    static simdutf_really_inline simd8<int8_t> splat(int8_t _value) { return vmovq_n_s8(_value); }
    static simdutf_really_inline simd8<int8_t> zero() { return vdupq_n_s8(0); }
    static simdutf_really_inline simd8<int8_t> load(const int8_t values[16]) { return vld1q_s8(values); }
    simdutf_really_inline void store_ascii_as_utf16(char16_t * p) const {
      vst1q_u16(reinterpret_cast<uint16_t*>(p), vmovl_u8(vget_low_u8 (vreinterpretq_u8_s8(this->value))));
      vst1q_u16(reinterpret_cast<uint16_t*>(p + 8), vmovl_high_u8(vreinterpretq_u8_s8(this->value)));
    }
    simdutf_really_inline void store_ascii_as_utf32(char32_t * p) const {
      vst1q_u32(reinterpret_cast<uint32_t*>(p), vmovl_u16(vget_low_u16(vmovl_u8(vget_low_u8 (vreinterpretq_u8_s8(this->value))))));
      vst1q_u32(reinterpret_cast<uint32_t*>(p + 4), vmovl_high_u16(vmovl_u8(vget_low_u8 (vreinterpretq_u8_s8(this->value)))));
      vst1q_u32(reinterpret_cast<uint32_t*>(p + 8), vmovl_u16(vget_low_u16(vmovl_high_u8(vreinterpretq_u8_s8(this->value)))));
      vst1q_u32(reinterpret_cast<uint32_t*>(p + 12), vmovl_high_u16(vmovl_high_u8(vreinterpretq_u8_s8(this->value))));
    }
    // Conversion from/to SIMD register
    simdutf_really_inline simd8(const int8x16_t _value) : value{_value} {}
    simdutf_really_inline operator const int8x16_t&() const { return this->value; }
    simdutf_really_inline operator const uint8x16_t() const { return vreinterpretq_u8_s8(this->value); }
    simdutf_really_inline operator int8x16_t&() { return this->value; }

    // Zero constructor
    simdutf_really_inline simd8() : simd8(zero()) {}
    // Splat constructor
    simdutf_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdutf_really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    simdutf_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3, int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(make_int8x16_t(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
#else
    simdutf_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3, int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(int8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
#endif
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdutf_really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Store to array
    simdutf_really_inline void store(int8_t dst[16]) const { return vst1q_s8(dst, value); }
    // Explicit conversion to/from unsigned
    //
    // Under Visual Studio/ARM64 uint8x16_t and int8x16_t are apparently the same type.
    // In theory, we could check this occurrence with std::same_as and std::enabled_if but it is C++14
    // and relatively ugly and hard to read.
#ifndef SIMDUTF_REGULAR_VISUAL_STUDIO
    simdutf_really_inline explicit simd8(const uint8x16_t other): simd8(vreinterpretq_s8_u8(other)) {}
#endif
    simdutf_really_inline operator simd8<uint8_t>() const { return vreinterpretq_u8_s8(this->value); }

    simdutf_really_inline simd8<int8_t> operator|(const simd8<int8_t> other) const { return vorrq_s8(value, other.value); }
    simdutf_really_inline simd8<int8_t> operator&(const simd8<int8_t> other) const { return vandq_s8(value, other.value); }
    simdutf_really_inline simd8<int8_t> operator^(const simd8<int8_t> other) const { return veorq_s8(value, other.value); }
    simdutf_really_inline simd8<int8_t> bit_andnot(const simd8<int8_t> other) const { return vbicq_s8(value, other.value); }

    // Math
    simdutf_really_inline simd8<int8_t> operator+(const simd8<int8_t> other) const { return vaddq_s8(value, other.value); }
    simdutf_really_inline simd8<int8_t> operator-(const simd8<int8_t> other) const { return vsubq_s8(value, other.value); }
    simdutf_really_inline simd8<int8_t>& operator+=(const simd8<int8_t> other) { *this = *this + other; return *this; }
    simdutf_really_inline simd8<int8_t>& operator-=(const simd8<int8_t> other) { *this = *this - other; return *this; }

    simdutf_really_inline int8_t max_val() const { return vmaxvq_s8(value); }
    simdutf_really_inline int8_t min_val() const { return vminvq_s8(value); }
    simdutf_really_inline bool is_ascii() const { return this->min_val() >= 0; }

    // Order-sensitive comparisons
    simdutf_really_inline simd8<int8_t> max_val(const simd8<int8_t> other) const { return vmaxq_s8(value, other.value); }
    simdutf_really_inline simd8<int8_t> min_val(const simd8<int8_t> other) const { return vminq_s8(value, other.value); }
    simdutf_really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return vcgtq_s8(value, other.value); }
    simdutf_really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return vcltq_s8(value, other.value); }
    simdutf_really_inline simd8<bool> operator==(const simd8<int8_t> other) const { return vceqq_s8(value, other.value); }

    template<int N=1>
    simdutf_really_inline simd8<int8_t> prev(const simd8<int8_t> prev_chunk) const {
      return vextq_s8(prev_chunk, *this, 16 - N);
    }

    // Perform a lookup assuming no value is larger than 16
    template<typename L>
    simdutf_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }
    template<typename L>
    simdutf_really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }

    template<typename T>
    simdutf_really_inline simd8<int8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_s8(*this, simd8<uint8_t>(original));
    }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 4, "ARM kernel should use four registers per 64-byte block.");
    simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T> other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

    simdutf_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    simdutf_really_inline simd8x64(const T* ptr) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+sizeof(simd8<T>)/sizeof(T)), simd8<T>::load(ptr+2*sizeof(simd8<T>)/sizeof(T)), simd8<T>::load(ptr+3*sizeof(simd8<T>)/sizeof(T))} {}

    simdutf_really_inline void store(T* ptr) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0/sizeof(T));
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1/sizeof(T));
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2/sizeof(T));
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3/sizeof(T));
    }


    simdutf_really_inline simd8x64<T>& operator |=(const simd8x64<T> &other) {
      this->chunks[0] |= other.chunks[0];
      this->chunks[1] |= other.chunks[1];
      this->chunks[2] |= other.chunks[2];
      this->chunks[3] |= other.chunks[3];
      return *this;
    }

    simdutf_really_inline simd8<T> reduce_or() const {
      return (this->chunks[0] | this->chunks[1]) | (this->chunks[2] | this->chunks[3]);
    }

    simdutf_really_inline bool is_ascii() const {
      return reduce_or().is_ascii();
    }

    simdutf_really_inline void store_ascii_as_utf16(char16_t * ptr) const {
      this->chunks[0].store_ascii_as_utf16(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store_ascii_as_utf16(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store_ascii_as_utf16(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store_ascii_as_utf16(ptr+sizeof(simd8<T>)*3);
    }

    simdutf_really_inline void store_ascii_as_utf32(char32_t * ptr) const {
      this->chunks[0].store_ascii_as_utf32(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store_ascii_as_utf32(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store_ascii_as_utf32(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store_ascii_as_utf32(ptr+sizeof(simd8<T>)*3);
    }

    simdutf_really_inline uint64_t to_bitmask() const {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
      const uint8x16_t bit_mask = make_uint8x16_t(
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      );
#else
      const uint8x16_t bit_mask = {
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      };
#endif
      // Add each of the elements next to each other, successively, to stuff each 8 byte mask into one.
      uint8x16_t sum0 = vpaddq_u8(vandq_u8(uint8x16_t(this->chunks[0]), bit_mask), vandq_u8(uint8x16_t(this->chunks[1]), bit_mask));
      uint8x16_t sum1 = vpaddq_u8(vandq_u8(uint8x16_t(this->chunks[2]), bit_mask), vandq_u8(uint8x16_t(this->chunks[3]), bit_mask));
      sum0 = vpaddq_u8(sum0, sum1);
      sum0 = vpaddq_u8(sum0, sum0);
      return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
    }

    simdutf_really_inline uint64_t eq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return  simd8x64<bool>(
      this->chunks[0] == mask,
      this->chunks[1] == mask,
      this->chunks[2] == mask,
      this->chunks[3] == mask
    ).to_bitmask();
  }

  simdutf_really_inline uint64_t lteq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return  simd8x64<bool>(
      this->chunks[0] <= mask,
      this->chunks[1] <= mask,
      this->chunks[2] <= mask,
      this->chunks[3] <= mask
    ).to_bitmask();
  }

    simdutf_really_inline uint64_t in_range(const T low, const T high) const {
      const simd8<T> mask_low = simd8<T>::splat(low);
      const simd8<T> mask_high = simd8<T>::splat(high);

      return  simd8x64<bool>(
        (this->chunks[0] <= mask_high) & (this->chunks[0] >= mask_low),
        (this->chunks[1] <= mask_high) & (this->chunks[1] >= mask_low),
        (this->chunks[2] <= mask_high) & (this->chunks[2] >= mask_low),
        (this->chunks[3] <= mask_high) & (this->chunks[3] >= mask_low)
      ).to_bitmask();
    }
    simdutf_really_inline uint64_t not_in_range(const T low, const T high) const {
      const simd8<T> mask_low = simd8<T>::splat(low);
      const simd8<T> mask_high = simd8<T>::splat(high);
      return  simd8x64<bool>(
        (this->chunks[0] > mask_high) | (this->chunks[0] < mask_low),
        (this->chunks[1] > mask_high) | (this->chunks[1] < mask_low),
        (this->chunks[2] > mask_high) | (this->chunks[2] < mask_low),
        (this->chunks[3] > mask_high) | (this->chunks[3] < mask_low)
      ).to_bitmask();
    }
    simdutf_really_inline uint64_t lt(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] < mask,
        this->chunks[1] < mask,
        this->chunks[2] < mask,
        this->chunks[3] < mask
      ).to_bitmask();
    }
    simdutf_really_inline uint64_t gt(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] > mask,
        this->chunks[1] > mask,
        this->chunks[2] > mask,
        this->chunks[3] > mask
      ).to_bitmask();
    }
    simdutf_really_inline uint64_t gteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] >= mask,
        this->chunks[1] >= mask,
        this->chunks[2] >= mask,
        this->chunks[3] >= mask
      ).to_bitmask();
    }
    simdutf_really_inline uint64_t gteq_unsigned(const uint8_t m) const {
      const simd8<uint8_t> mask = simd8<uint8_t>::splat(m);
      return  simd8x64<bool>(
        simd8<uint8_t>(uint8x16_t(this->chunks[0])) >= mask,
        simd8<uint8_t>(uint8x16_t(this->chunks[1])) >= mask,
        simd8<uint8_t>(uint8x16_t(this->chunks[2])) >= mask,
        simd8<uint8_t>(uint8x16_t(this->chunks[3])) >= mask
      ).to_bitmask();
    }
  }; // struct simd8x64<T>
  #include "simdutf/arm64/simd16-inl.h"
} // namespace simd
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#endif // SIMDUTF_ARM64_SIMD_H
