template <typename T> struct simd32;

template <> struct simd32<uint32_t> {
  static const size_t SIZE = sizeof(__m128i);
  static const size_t ELEMENTS = SIZE / sizeof(uint32_t);

  __m128i value;

  simdutf_really_inline simd32(const __m128i v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd32(const Pointer *ptr)
      : value(_mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr))) {}

  uint64_t sum() const {
    return uint64_t(_mm_extract_epi32(value, 0)) +
           uint64_t(_mm_extract_epi32(value, 1)) +
           uint64_t(_mm_extract_epi32(value, 2)) +
           uint64_t(_mm_extract_epi32(value, 3));
  }

  void dump() const {
    printf("[%08x, %08x, %08x, %08x]\n", uint32_t(_mm_extract_epi32(value, 0)),
           uint32_t(_mm_extract_epi32(value, 1)),
           uint32_t(_mm_extract_epi32(value, 2)),
           uint32_t(_mm_extract_epi32(value, 3)));
  }

  // operators
  simdutf_really_inline simd32 &operator+=(const simd32 other) {
    value = _mm_add_epi32(value, other.value);
    return *this;
  }

  // static members
  simdutf_really_inline static simd32<uint32_t> zero() {
    return _mm_setzero_si128();
  }

  simdutf_really_inline static simd32<uint32_t> splat(uint32_t v) {
    return _mm_set1_epi32(v);
  }
};

simdutf_really_inline simd32<uint32_t> min(const simd32<uint32_t> b,
                                           const simd32<uint32_t> a) {
  return _mm_min_epu32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> b,
                                                 const simd32<uint32_t> a) {
  return _mm_and_si128(a.value, b.value);
}
