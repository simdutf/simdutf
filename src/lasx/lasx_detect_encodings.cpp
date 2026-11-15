namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace encoding_detection {

struct utf16le_checker {
  uint64_t error = 0;
  uint64_t prev_high = 0;

  template <bool overlapping = false>
  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const simd::simd8<uint8_t> v_fc = simd::simd8<uint8_t>::splat(0xfc);
    const simd::simd8<uint8_t> v_d8 = simd::simd8<uint8_t>::splat(0xd8);
    const simd::simd8<uint8_t> v_dc = simd::simd8<uint8_t>::splat(0xdc);
    const simd::simd8<uint8_t> m0 = in.chunks[0] & v_fc;
    const simd::simd8<uint8_t> m1 = in.chunks[1] & v_fc;
    const uint64_t odd = 0xaaaaaaaaaaaaaaaa;
    const uint64_t high =
        simd::simd8x64<bool>(m0 == v_d8, m1 == v_d8).to_bitmask() & odd;
    const uint64_t low =
        simd::simd8x64<bool>(m0 == v_dc, m1 == v_dc).to_bitmask() & odd;
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
  __m256i currentmax = __lasx_xvreplgr2vr_w(0);
  __m256i currentoffsetmax = __lasx_xvreplgr2vr_w(0);

  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const __m256i offset = __lasx_xvreplgr2vr_w(int32_t(0xffff2000));
    const __m256i w0 = in.chunks[0];
    const __m256i w1 = in.chunks[1];
    currentmax = __lasx_xvmax_wu(currentmax, __lasx_xvmax_wu(w0, w1));
    currentoffsetmax = __lasx_xvmax_wu(
        currentoffsetmax, __lasx_xvmax_wu(__lasx_xvadd_w(w0, offset),
                                          __lasx_xvadd_w(w1, offset)));
  }

  simdutf_really_inline bool errors() const {
    const __m256i too_large =
        __lasx_xvslt_wu(__lasx_xvreplgr2vr_w(0x10ffff), currentmax);
    const __m256i surrogate = __lasx_xvslt_wu(
        __lasx_xvreplgr2vr_w(int32_t(0xfffff7ff)), currentoffsetmax);
    return __lasx_xbnz_v(__lasx_xvor_v(too_large, surrogate));
  }
};

} // namespace encoding_detection
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
