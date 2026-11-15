namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace encoding_detection {

// Checks UTF-16LE in 64-byte blocks (32 code units).
struct utf16le_checker {
  uint8x16_t error = vdupq_n_u8(0);
  // Lane 15 is set if the last code unit of the previous block was a high
  // surrogate (0xD800..0xDBFF). The other lanes are not used.
  uint8x16_t prev_high = vdupq_n_u8(0);

  // If 'overlapping' is true, the first code unit of the block has already
  // been checked together with the code unit before it.
  template <bool overlapping = false>
  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    // The odd bytes are the high bytes of the code units.
    const uint8x16_t hi0 = vuzp2q_u8(in.chunks[0], in.chunks[1]);
    const uint8x16_t hi1 = vuzp2q_u8(in.chunks[2], in.chunks[3]);
    const uint8x16_t v_fc = vdupq_n_u8(0xfc);
    const uint8x16_t m0 = vandq_u8(hi0, v_fc);
    const uint8x16_t m1 = vandq_u8(hi1, v_fc);
    const uint8x16_t high0 = vceqq_u8(m0, vdupq_n_u8(0xd8));
    const uint8x16_t high1 = vceqq_u8(m1, vdupq_n_u8(0xd8));
    const uint8x16_t low0 = vceqq_u8(m0, vdupq_n_u8(0xdc));
    const uint8x16_t low1 = vceqq_u8(m1, vdupq_n_u8(0xdc));
    // A code unit is a low surrogate if and only if the code unit before it is
    // a high surrogate. When overlapping, we pretend that the code unit before
    // the block is a high surrogate exactly when the first one is a low
    // surrogate, so that the first code unit is not checked again.
    const uint8x16_t carry = overlapping ? vdupq_laneq_u8(low0, 0) : prev_high;
    error = vorrq_u8(error, veorq_u8(low0, vextq_u8(carry, high0, 15)));
    error = vorrq_u8(error, veorq_u8(low1, vextq_u8(high0, high1, 15)));
    prev_high = high1;
  }

  // For a block without any byte >= 0x80, hence without surrogates: the only
  // possible error is a high surrogate at the end of the previous block.
  simdutf_really_inline void check_ascii() {
    error = vorrq_u8(error, vextq_u8(prev_high, vdupq_n_u8(0), 15));
    prev_high = vdupq_n_u8(0);
  }

  simdutf_really_inline bool errors() const {
    return any_lane_set(vreinterpretq_u16_u8(error));
  }

  // The input must not end with a high surrogate.
  simdutf_really_inline bool errors_at_eof() const {
    return errors() || vgetq_lane_u8(prev_high, 15) != 0;
  }
};

// Checks UTF-32LE: every value must be at most 0x10FFFF and outside
// 0xD800..0xDFFF. Adding 0xFFFF2000 maps the surrogates to the 2048 largest
// 32-bit values, so we only track two maxima.
struct utf32le_checker {
  uint32x4_t currentmax = vdupq_n_u32(0);
  uint32x4_t currentoffsetmax = vdupq_n_u32(0);

  simdutf_really_inline void check(const simd::simd8x64<uint8_t> &in) {
    const uint32x4_t offset = vdupq_n_u32(0xffff2000);
    const uint32x4_t w0 = vreinterpretq_u32_u8(in.chunks[0]);
    const uint32x4_t w1 = vreinterpretq_u32_u8(in.chunks[1]);
    const uint32x4_t w2 = vreinterpretq_u32_u8(in.chunks[2]);
    const uint32x4_t w3 = vreinterpretq_u32_u8(in.chunks[3]);
    // Reduce the block first so that the loop-carried dependency is a single
    // vmaxq_u32 per 64 bytes.
    currentmax =
        vmaxq_u32(currentmax, vmaxq_u32(vmaxq_u32(w0, w1), vmaxq_u32(w2, w3)));
    currentoffsetmax = vmaxq_u32(
        currentoffsetmax,
        vmaxq_u32(vmaxq_u32(vaddq_u32(w0, offset), vaddq_u32(w1, offset)),
                  vmaxq_u32(vaddq_u32(w2, offset), vaddq_u32(w3, offset))));
  }

  simdutf_really_inline bool errors() const {
    const uint32x4_t too_large = vcgtq_u32(currentmax, vdupq_n_u32(0x10ffff));
    const uint32x4_t surrogate =
        vcgtq_u32(currentoffsetmax, vdupq_n_u32(0xfffff7ff));
    return any_lane_set(vreinterpretq_u16_u32(vorrq_u32(too_large, surrogate)));
  }
};

} // namespace encoding_detection
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
