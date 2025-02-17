template <typename T> struct simd32;

template <> struct simd32<uint32_t> {
  static const size_t SIZE = sizeof(__m256i);
  static const size_t ELEMENTS = SIZE / sizeof(uint32_t);

  __m256i value;

  simdutf_really_inline simd32(const __m256i v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd32(const Pointer *ptr)
      : value(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr))) {}

  uint64_t sum() const {
    const __m256i mask = _mm256_set1_epi64x(0xffffffff);
    const __m256i t0 = _mm256_and_si256(value, mask);
    const __m256i t1 = _mm256_srli_epi64(value, 32);
    const __m256i t2 = _mm256_add_epi64(t0, t1);

    return uint64_t(_mm256_extract_epi64(t2, 0)) +
           uint64_t(_mm256_extract_epi64(t2, 1)) +
           uint64_t(_mm256_extract_epi64(t2, 2)) +
           uint64_t(_mm256_extract_epi64(t2, 3));
  }

  // operators
  simdutf_really_inline simd32 &operator+=(const simd32 other) {
    value = _mm256_add_epi32(value, other.value);
    return *this;
  }

  // static members
  simdutf_really_inline static simd32<uint32_t> zero() {
    return _mm256_setzero_si256();
  }

  simdutf_really_inline static simd32<uint32_t> splat(uint32_t v) {
    return _mm256_set1_epi32(v);
  }
};

simdutf_really_inline simd32<uint32_t> min(const simd32<uint32_t> b,
                                           const simd32<uint32_t> a) {
  return _mm256_min_epu32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> b,
                                                 const simd32<uint32_t> a) {
  return _mm256_and_si256(a.value, b.value);
}
