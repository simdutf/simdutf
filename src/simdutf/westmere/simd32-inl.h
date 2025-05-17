template <typename T> struct simd32;

template <> struct simd32<uint32_t> {
  static const size_t SIZE = sizeof(__m128i);
  static const size_t ELEMENTS = SIZE / sizeof(uint32_t);

  __m128i value;

  simdutf_really_inline simd32(const __m128i v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd32(const Pointer *ptr)
      : value(_mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr))) {}

  simdutf_really_inline uint64_t sum() const {
    return uint64_t(_mm_extract_epi32(value, 0)) +
           uint64_t(_mm_extract_epi32(value, 1)) +
           uint64_t(_mm_extract_epi32(value, 2)) +
           uint64_t(_mm_extract_epi32(value, 3));
  }

  simdutf_really_inline simd32<uint32_t> swap_bytes() const {
    const __m128i shuffle =
        _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 8, 9, 10, 11, 15, 14, 13, 12);

    return _mm_shuffle_epi8(value, shuffle);
  }

  template <int N> simdutf_really_inline simd32<uint32_t> shr() const {
    return _mm_srli_epi32(value, N);
  }

  template <int N> simdutf_really_inline simd32<uint32_t> shl() const {
    return _mm_slli_epi32(value, N);
  }
  void dump() const {
#ifdef SIMDUTF_LOGGING
    printf("[%08x, %08x, %08x, %08x]\n", uint32_t(_mm_extract_epi32(value, 0)),
           uint32_t(_mm_extract_epi32(value, 1)),
           uint32_t(_mm_extract_epi32(value, 2)),
           uint32_t(_mm_extract_epi32(value, 3)));
#endif // SIMDUTF_LOGGING
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

//----------------------------------------------------------------------

template <> struct simd32<bool> {
  // static const size_t SIZE = sizeof(__m128i);
  // static const size_t ELEMENTS = SIZE / sizeof(uint32_t);

  __m128i value;

  simdutf_really_inline simd32(const __m128i v) : value(v) {}

  simdutf_really_inline bool any() const {
    return _mm_movemask_epi8(value) != 0;
  }

  simdutf_really_inline uint8_t to_4bit_bitmask() const {
    return uint8_t(_mm_movemask_ps(_mm_castsi128_ps(value)));
  }
};

//----------------------------------------------------------------------

template <typename T>
simdutf_really_inline simd32<T> operator|(const simd32<T> a,
                                          const simd32<T> b) {
  return _mm_or_si128(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> min(const simd32<uint32_t> a,
                                           const simd32<uint32_t> b) {
  return _mm_min_epu32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> max(const simd32<uint32_t> a,
                                           const simd32<uint32_t> b) {
  return _mm_max_epu32(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator==(const simd32<uint32_t> a,
                                              uint32_t b) {
  return _mm_cmpeq_epi32(a.value, _mm_set1_epi32(b));
}

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> a,
                                                 const simd32<uint32_t> b) {
  return _mm_and_si128(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> a,
                                                 uint32_t b) {
  return _mm_and_si128(a.value, _mm_set1_epi32(b));
}

simdutf_really_inline simd32<uint32_t> operator|(const simd32<uint32_t> a,
                                                 uint32_t b) {
  return _mm_or_si128(a.value, _mm_set1_epi32(b));
}

simdutf_really_inline simd32<uint32_t> operator+(const simd32<uint32_t> a,
                                                 const simd32<uint32_t> b) {
  return _mm_add_epi32(a.value, b.value);
}

simdutf_really_inline simd32<uint32_t> operator-(const simd32<uint32_t> a,
                                                 uint32_t b) {
  return _mm_sub_epi32(a.value, _mm_set1_epi32(b));
}

simdutf_really_inline simd32<bool> operator==(const simd32<uint32_t> a,
                                              const simd32<uint32_t> b) {
  return _mm_cmpeq_epi32(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator>=(const simd32<uint32_t> a,
                                              const simd32<uint32_t> b) {
  return _mm_cmpeq_epi32(_mm_max_epu32(a.value, b.value), a.value);
}

simdutf_really_inline simd32<bool> operator!(const simd32<bool> v) {
  return _mm_xor_si128(v.value, _mm_set1_epi8(-1));
}

simdutf_really_inline simd32<bool> operator>(const simd32<uint32_t> a,
                                             const simd32<uint32_t> b) {
  return !(b >= a);
}

simdutf_really_inline simd32<uint32_t> select(const simd32<bool> cond,
                                              const simd32<uint32_t> v_true,
                                              const simd32<uint32_t> v_false) {
  return _mm_blendv_epi8(v_false.value, v_true.value, cond.value);
}
