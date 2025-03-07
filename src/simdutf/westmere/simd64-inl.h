template <typename T> struct simd64;

template <> struct simd64<uint64_t> {
  // static const size_t SIZE = sizeof(__m128i);
  // static const size_t ELEMENTS = SIZE / sizeof(uint64_t);

  __m128i value;

  simdutf_really_inline simd64(const __m128i v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd64(const Pointer *ptr)
      : value(_mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr))) {}

  simdutf_really_inline uint64_t sum() const {
    return _mm_extract_epi64(value, 0) + _mm_extract_epi64(value, 1);
  }

  // operators
  simdutf_really_inline simd64 &operator+=(const simd64 other) {
    value = _mm_add_epi64(value, other.value);
    return *this;
  }

  // static members
  simdutf_really_inline static simd64<uint64_t> zero() {
    return _mm_setzero_si128();
  }

  simdutf_really_inline static simd64<uint64_t> splat(uint64_t v) {
    return _mm_set1_epi64x(v);
  }
};
