namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace encoding_detection {

struct utf16le_checker {
  uint64_t error = 0;
  uint64_t prev_high = 0;

  template <bool overlapping = false>
  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const __m256i v_fc = _mm256_set1_epi8(char(0xfc));
    const __m256i v_d8 = _mm256_set1_epi8(char(0xd8));
    const __m256i v_dc = _mm256_set1_epi8(char(0xdc));
    const __m256i m0 = _mm256_and_si256(in.chunks[0], v_fc);
    const __m256i m1 = _mm256_and_si256(in.chunks[1], v_fc);
    const uint64_t odd = 0xaaaaaaaaaaaaaaaa;
    const uint64_t high =
        (uint64_t(uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi8(m0, v_d8)))) |
         (uint64_t(uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi8(m1, v_d8))))
          << 32)) &
        odd;
    const uint64_t low =
        (uint64_t(uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi8(m0, v_dc)))) |
         (uint64_t(uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi8(m1, v_dc))))
          << 32)) &
        odd;
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
  __m256i currentmax = _mm256_setzero_si256();
  __m256i currentoffsetmax = _mm256_setzero_si256();

  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const __m256i offset = _mm256_set1_epi32(int32_t(0xffff2000));
    const __m256i w0 = in.chunks[0];
    const __m256i w1 = in.chunks[1];
    currentmax = _mm256_max_epu32(currentmax, _mm256_max_epu32(w0, w1));
    currentoffsetmax = _mm256_max_epu32(
        currentoffsetmax, _mm256_max_epu32(_mm256_add_epi32(w0, offset),
                                           _mm256_add_epi32(w1, offset)));
  }

  simdutf_really_inline bool errors() const {
    const __m256i standardmax = _mm256_set1_epi32(0x10ffff);
    const __m256i standardoffsetmax = _mm256_set1_epi32(int32_t(0xfffff7ff));
    const __m256i too_large = _mm256_xor_si256(
        _mm256_max_epu32(currentmax, standardmax), standardmax);
    const __m256i surrogate =
        _mm256_xor_si256(_mm256_max_epu32(currentoffsetmax, standardoffsetmax),
                         standardoffsetmax);
    const __m256i any = _mm256_or_si256(too_large, surrogate);
    return !_mm256_testz_si256(any, any);
  }
};

} // namespace encoding_detection
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
