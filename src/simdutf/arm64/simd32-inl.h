template <typename T> struct simd32;

template <> struct simd32<uint32_t> {
  static const size_t SIZE = sizeof(uint32x4_t);
  static const size_t ELEMENTS = SIZE / sizeof(uint32_t);

  uint32x4_t value;

  simdutf_really_inline simd32(const uint32x4_t v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd32(const Pointer *ptr)
      : value(vld1q_u32(reinterpret_cast<const uint32_t *>(ptr))) {}

  simdutf_really_inline uint64_t sum() const { return vaddvq_u32(value); }

  simdutf_really_inline simd32<uint32_t> swap_bytes() const {
    return vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(value)));
  }

  template <int N> simdutf_really_inline simd32<uint32_t> shr() const {
    return vshrq_n_u32(value, N);
  }

  template <int N> simdutf_really_inline simd32<uint32_t> shl() const {
    return vshlq_n_u32(value, N);
  }

  void dump() const {
#ifdef SIMDUTF_LOGGING
    uint32_t temp[4];
    vst1q_u32(temp, value);
    printf("[%08x, %08x, %08x, %08x]\n", temp[0], temp[1], temp[2], temp[3]);
#endif // SIMDUTF_LOGGING
  }

  // operators
  simdutf_really_inline simd32 &operator+=(const simd32 other) {
    value = vaddq_u32(value, other.value);
    return *this;
  }

  // static members
  simdutf_really_inline static simd32<uint32_t> zero() {
    return vdupq_n_u32(0);
  }

  simdutf_really_inline static simd32<uint32_t> splat(uint32_t v) {
    return vdupq_n_u32(v);
  }
};

//----------------------------------------------------------------------

template <> struct simd32<bool> {
  uint32x4_t value;

  simdutf_really_inline simd32(const uint32x4_t v) : value(v) {}

  simdutf_really_inline bool any() const { return vmaxvq_u32(value) != 0; }
};

//----------------------------------------------------------------------

template <typename T>
simdutf_really_inline simd32<T> operator|(const simd32<T> a,
                                          const simd32<T> b) {
  return vorrq_u32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> min(const simd32<uint32_t> a,
                                           const simd32<uint32_t> b) {
  return vminq_u32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> max(const simd32<uint32_t> a,
                                           const simd32<uint32_t> b) {
  return vmaxq_u32(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator==(const simd32<uint32_t> a,
                                              uint32_t b) {
  return vceqq_u32(a.value, vdupq_n_u32(b));
}

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> a,
                                                 const simd32<uint32_t> b) {
  return vandq_u32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> a,
                                                 uint32_t b) {
  return vandq_u32(a.value, vdupq_n_u32(b));
}

simdutf_really_inline simd32<uint32_t> operator|(const simd32<uint32_t> a,
                                                 uint32_t b) {
  return vorrq_u32(a.value, vdupq_n_u32(b));
}

simdutf_really_inline simd32<uint32_t> operator+(const simd32<uint32_t> a,
                                                 const simd32<uint32_t> b) {
  return vaddq_u32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator-(const simd32<uint32_t> a,
                                                 uint32_t b) {
  return vsubq_u32(a.value, vdupq_n_u32(b));
}

simdutf_really_inline simd32<bool> operator>=(const simd32<uint32_t> a,
                                              const simd32<uint32_t> b) {
  return vcgeq_u32(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator!(const simd32<bool> v) {
  return vmvnq_u32(v.value);
}

simdutf_really_inline simd32<bool> operator>(const simd32<uint32_t> a,
                                             const simd32<uint32_t> b) {
  return vcgtq_u32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> select(const simd32<bool> cond,
                                              const simd32<uint32_t> v_true,
                                              const simd32<uint32_t> v_false) {
  return vbslq_u32(cond.value, v_true.value, v_false.value);
}
