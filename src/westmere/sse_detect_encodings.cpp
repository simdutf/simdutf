namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace encoding_detection {

struct utf16le_checker {
  uint64_t error = 0;
  uint64_t prev_high = 0;

  template <bool overlapping = false>
  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const __m128i v_fc = _mm_set1_epi8(char(0xfc));
    const __m128i v_d8 = _mm_set1_epi8(char(0xd8));
    const __m128i v_dc = _mm_set1_epi8(char(0xdc));
    const uint64_t odd = 0xaaaaaaaaaaaaaaaa;
    uint64_t high = 0;
    uint64_t low = 0;
    for (size_t i = 0; i < 4; i++) {
      const __m128i m = _mm_and_si128(in.chunks[i], v_fc);
      high |= uint64_t(uint16_t(_mm_movemask_epi8(_mm_cmpeq_epi8(m, v_d8))))
              << (16 * i);
      low |= uint64_t(uint16_t(_mm_movemask_epi8(_mm_cmpeq_epi8(m, v_dc))))
             << (16 * i);
    }
    high &= odd;
    low &= odd;
    const uint64_t carry = overlapping ? (low & 2) : prev_high;
    error |= ((high << 2) | carry) ^ low;
    prev_high = high >> 62;
  }

  simdutf_really_inline void check_ascii() {
    error |= prev_high;
    prev_high = 0;
  }

  simdutf_really_inline bool errors() const { return error != 0; }

  simdutf_really_inline bool errors_at_eof() const {
    return (error | prev_high) != 0;
  }
};

struct utf32le_checker {
  __m128i currentmax = _mm_setzero_si128();
  __m128i currentoffsetmax = _mm_setzero_si128();

  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const __m128i offset = _mm_set1_epi32(int32_t(0xffff2000));
    const __m128i w0 = in.chunks[0];
    const __m128i w1 = in.chunks[1];
    const __m128i w2 = in.chunks[2];
    const __m128i w3 = in.chunks[3];
    currentmax =
        _mm_max_epu32(currentmax, _mm_max_epu32(_mm_max_epu32(w0, w1),
                                                _mm_max_epu32(w2, w3)));
    currentoffsetmax =
        _mm_max_epu32(currentoffsetmax,
                      _mm_max_epu32(_mm_max_epu32(_mm_add_epi32(w0, offset),
                                                  _mm_add_epi32(w1, offset)),
                                    _mm_max_epu32(_mm_add_epi32(w2, offset),
                                                  _mm_add_epi32(w3, offset))));
  }

  simdutf_really_inline bool errors() const {
    const __m128i standardmax = _mm_set1_epi32(0x10ffff);
    const __m128i standardoffsetmax = _mm_set1_epi32(int32_t(0xfffff7ff));
    const __m128i too_large =
        _mm_xor_si128(_mm_max_epu32(currentmax, standardmax), standardmax);
    const __m128i surrogate = _mm_xor_si128(
        _mm_max_epu32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
    const __m128i any = _mm_or_si128(too_large, surrogate);
    return !_mm_testz_si128(any, any);
  }
};

} // namespace encoding_detection
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
