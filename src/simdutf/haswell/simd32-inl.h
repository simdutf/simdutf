template <typename T> struct simd32;

template <> struct simd32<uint32_t> {
  static const size_t SIZE = sizeof(__m256i);
  static const size_t ELEMENTS = SIZE / sizeof(uint32_t);

  __m256i value;

  simdutf_really_inline simd32(const __m256i v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd32(const Pointer *ptr)
      : value(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr))) {}

  simdutf_really_inline uint64_t sum() const {
    const __m256i mask = _mm256_set1_epi64x(0xffffffff);
    const __m256i t0 = _mm256_and_si256(value, mask);
    const __m256i t1 = _mm256_srli_epi64(value, 32);
    const __m256i t2 = _mm256_add_epi64(t0, t1);

    return uint64_t(_mm256_extract_epi64(t2, 0)) +
           uint64_t(_mm256_extract_epi64(t2, 1)) +
           uint64_t(_mm256_extract_epi64(t2, 2)) +
           uint64_t(_mm256_extract_epi64(t2, 3));
  }

  simdutf_really_inline simd32<uint32_t> swap_bytes() const {
    const __m256i shuffle =
        _mm256_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 8, 9, 10, 11, 15, 14, 13, 12,
                         3, 2, 1, 0, 7, 6, 5, 4, 8, 9, 10, 11, 15, 14, 13, 12);

    return _mm256_shuffle_epi8(value, shuffle);
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

//----------------------------------------------------------------------

template <> struct simd32<bool> {
  // static const size_t SIZE = sizeof(__m128i);
  // static const size_t ELEMENTS = SIZE / sizeof(uint32_t);

  __m256i value;

  simdutf_really_inline simd32(const __m256i v) : value(v) {}

  simdutf_really_inline bool any() const {
    return _mm256_movemask_epi8(value) != 0;
  }
};

//----------------------------------------------------------------------

template <typename T>
simdutf_really_inline simd32<T> operator|(const simd32<T> a,
                                          const simd32<T> b) {
  return _mm256_or_si256(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> min(const simd32<uint32_t> b,
                                           const simd32<uint32_t> a) {
  return _mm256_min_epu32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> max(const simd32<uint32_t> a,
                                           const simd32<uint32_t> b) {
  return _mm256_max_epu32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> b,
                                                 const simd32<uint32_t> a) {
  return _mm256_and_si256(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator+(const simd32<uint32_t> a,
                                                 const simd32<uint32_t> b) {
  return _mm256_add_epi32(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator==(const simd32<uint32_t> a,
                                              const simd32<uint32_t> b) {
  return _mm256_cmpeq_epi32(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator>=(const simd32<uint32_t> a,
                                              const simd32<uint32_t> b) {
  return _mm256_cmpeq_epi32(_mm256_max_epu32(a.value, b.value), a.value);
}

simdutf_really_inline simd32<bool> operator!(const simd32<bool> v) {
  return _mm256_xor_si256(v.value, _mm256_set1_epi8(-1));
}

simdutf_really_inline simd32<bool> operator>(const simd32<uint32_t> a,
                                             const simd32<uint32_t> b) {
  return !(b >= a);
}
