namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace encoding_detection {

simdutf_really_inline uint64_t lsx_bitmask64(__m128i m0, __m128i m1, __m128i m2,
                                             __m128i m3) {
  __m128i mask = __lsx_vbsll_v(__lsx_vmsknz_b(m3), 6);
  mask = __lsx_vor_v(mask, __lsx_vbsll_v(__lsx_vmsknz_b(m2), 4));
  mask = __lsx_vor_v(mask, __lsx_vbsll_v(__lsx_vmsknz_b(m1), 2));
  mask = __lsx_vor_v(mask, __lsx_vmsknz_b(m0));
  return __lsx_vpickve2gr_du(mask, 0);
}

struct utf16le_checker {
  uint64_t error = 0;
  uint64_t prev_high = 0;

  template <bool overlapping = false>
  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const __m128i v_fc = __lsx_vrepli_b(int8_t(0xfc));
    const __m128i v_d8 = __lsx_vreplgr2vr_b(0xd8);
    const __m128i v_dc = __lsx_vreplgr2vr_b(0xdc);
    const __m128i m0 = __lsx_vand_v(in.chunks[0], v_fc);
    const __m128i m1 = __lsx_vand_v(in.chunks[1], v_fc);
    const __m128i m2 = __lsx_vand_v(in.chunks[2], v_fc);
    const __m128i m3 = __lsx_vand_v(in.chunks[3], v_fc);
    const uint64_t odd = 0xaaaaaaaaaaaaaaaa;
    const uint64_t high =
        lsx_bitmask64(__lsx_vseq_b(m0, v_d8), __lsx_vseq_b(m1, v_d8),
                      __lsx_vseq_b(m2, v_d8), __lsx_vseq_b(m3, v_d8)) &
        odd;
    const uint64_t low =
        lsx_bitmask64(__lsx_vseq_b(m0, v_dc), __lsx_vseq_b(m1, v_dc),
                      __lsx_vseq_b(m2, v_dc), __lsx_vseq_b(m3, v_dc)) &
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
  __m128i currentmax = __lsx_vreplgr2vr_w(0);
  __m128i currentoffsetmax = __lsx_vreplgr2vr_w(0);

  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const __m128i offset = __lsx_vreplgr2vr_w(int32_t(0xffff2000));
    const __m128i w0 = in.chunks[0];
    const __m128i w1 = in.chunks[1];
    const __m128i w2 = in.chunks[2];
    const __m128i w3 = in.chunks[3];
    currentmax =
        __lsx_vmax_wu(currentmax, __lsx_vmax_wu(__lsx_vmax_wu(w0, w1),
                                                __lsx_vmax_wu(w2, w3)));
    currentoffsetmax = __lsx_vmax_wu(
        currentoffsetmax,
        __lsx_vmax_wu(
            __lsx_vmax_wu(__lsx_vadd_w(w0, offset), __lsx_vadd_w(w1, offset)),
            __lsx_vmax_wu(__lsx_vadd_w(w2, offset), __lsx_vadd_w(w3, offset))));
  }

  simdutf_really_inline bool errors() const {
    const __m128i too_large =
        __lsx_vslt_wu(__lsx_vreplgr2vr_w(0x10ffff), currentmax);
    const __m128i surrogate = __lsx_vslt_wu(
        __lsx_vreplgr2vr_w(int32_t(0xfffff7ff)), currentoffsetmax);
    return __lsx_bnz_v(__lsx_vor_v(too_large, surrogate));
  }
};

} // namespace encoding_detection
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
