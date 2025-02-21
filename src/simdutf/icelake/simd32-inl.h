template <typename T> struct simd32;

template <> struct simd32<uint32_t> {
  static const size_t SIZE = sizeof(__m512i);
  static const size_t ELEMENTS = SIZE / sizeof(uint32_t);

  __m512i value;

  simdutf_really_inline simd32(const __m512i v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd32(const Pointer *ptr)
      : value(_mm512_loadu_si512(reinterpret_cast<const __m512i *>(ptr))) {}

  uint64_t sum() const {
    const __m512i mask = _mm512_set1_epi64(0xffffffff);
    const __m512i t0 = _mm512_and_si512(value, mask);
    const __m512i t1 = _mm512_srli_epi64(value, 32);
    const __m512i t2 = _mm512_add_epi64(t0, t1);
    return _mm512_reduce_add_epi64(t2);
  }

  // operators
  simdutf_really_inline simd32 &operator+=(const simd32 other) {
    value = _mm512_add_epi32(value, other.value);
    return *this;
  }

  // static members
  simdutf_really_inline static simd32<uint32_t> zero() {
    return _mm512_setzero_si512();
  }

  simdutf_really_inline static simd32<uint32_t> splat(uint32_t v) {
    return _mm512_set1_epi32(v);
  }
};

simdutf_really_inline simd32<uint32_t> min(const simd32<uint32_t> b,
                                           const simd32<uint32_t> a) {
  return _mm512_min_epu32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> b,
                                                 const simd32<uint32_t> a) {
  return _mm512_and_si512(a.value, b.value);
}
