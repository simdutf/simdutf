template <typename T> struct simd16;

template <> struct simd16<uint16_t> {
  static const size_t SIZE = sizeof(__m512i);
  static const size_t ELEMENTS = SIZE / sizeof(uint16_t);

  template <typename Pointer>
  static simdutf_really_inline simd16<uint16_t> load(const Pointer *ptr) {
    return simd16<uint16_t>(ptr);
  }

  __m512i value;

  simdutf_really_inline simd16(const __m512i v) : value(v) {}

  template <typename Pointer>
  simdutf_really_inline simd16(const Pointer *ptr)
      : value(_mm512_loadu_si512(reinterpret_cast<const __m512i *>(ptr))) {}

  // operators
  simdutf_really_inline simd16 &operator+=(const simd16 other) {
    value = _mm512_add_epi32(value, other.value);
    return *this;
  }

  simdutf_really_inline simd16 &operator-=(const simd16 other) {
    value = _mm512_sub_epi32(value, other.value);
    return *this;
  }

  // methods
  simdutf_really_inline simd16 swap_bytes() const {
    const __m512i byteflip = _mm512_setr_epi64(
        0x0607040502030001, 0x0e0f0c0d0a0b0809, 0x0607040502030001,
        0x0e0f0c0d0a0b0809, 0x0607040502030001, 0x0e0f0c0d0a0b0809,
        0x0607040502030001, 0x0e0f0c0d0a0b0809);

    return _mm512_shuffle_epi8(value, byteflip);
  }

  simdutf_really_inline uint64_t sum() const {
    const auto lo = _mm512_and_si512(value, _mm512_set1_epi32(0xffff));
    const auto hi = _mm512_srli_epi32(value, 16);
    const auto sum32 = _mm512_add_epi32(lo, hi);

    return _mm512_reduce_add_epi32(sum32);
  }

  // static members
  simdutf_really_inline static simd16<uint16_t> zero() {
    return _mm512_setzero_si512();
  }

  simdutf_really_inline static simd16<uint16_t> splat(uint16_t v) {
    return _mm512_set1_epi16(v);
  }
};

template <> struct simd16<bool> {
  __mmask32 value;

  simdutf_really_inline simd16(const __mmask32 v) : value(v) {}
};

// ------------------------------------------------------------

simdutf_really_inline simd16<uint16_t> min(const simd16<uint16_t> b,
                                           const simd16<uint16_t> a) {
  return _mm512_min_epu16(a.value, b.value);
}

simdutf_really_inline simd16<uint16_t> operator&(const simd16<uint16_t> a,
                                                 uint16_t b) {
  return _mm512_and_si512(a.value, _mm512_set1_epi16(b));
}

simdutf_really_inline simd16<uint16_t> operator^(const simd16<uint16_t> a,
                                                 uint16_t b) {
  return _mm512_xor_si512(a.value, _mm512_set1_epi16(b));
}

simdutf_really_inline simd16<uint16_t> operator^(const simd16<uint16_t> a,
                                                 const simd16<uint16_t> b) {
  return _mm512_xor_si512(a.value, b.value);
}

simdutf_really_inline simd16<bool> operator==(const simd16<uint16_t> a,
                                              uint16_t b) {
  return _mm512_cmpeq_epi16_mask(a.value, _mm512_set1_epi16(b));
}
