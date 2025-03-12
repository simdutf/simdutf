template <typename T> struct simd64;

template <> struct simd64<uint64_t> {
  // static const size_t SIZE = sizeof(__m256i);
  // static const size_t ELEMENTS = SIZE / sizeof(uint64_t);

  __m256i value;

  simdutf_really_inline simd64(const __m256i v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd64(const Pointer *ptr)
      : value(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr))) {}

  simdutf_really_inline uint64_t sum() const {
    return _mm256_extract_epi64(value, 0) + _mm256_extract_epi64(value, 1) +
           _mm256_extract_epi64(value, 2) + _mm256_extract_epi64(value, 3);
  }

  // operators
  simdutf_really_inline simd64 &operator+=(const simd64 other) {
    value = _mm256_add_epi64(value, other.value);
    return *this;
  }

  // static members
  simdutf_really_inline static simd64<uint64_t> zero() {
    return _mm256_setzero_si256();
  }

  simdutf_really_inline static simd64<uint64_t> splat(uint64_t v) {
    return _mm256_set1_epi64x(v);
  }
};
