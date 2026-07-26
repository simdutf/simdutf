// Lookup table to get size of UTF-8 code point by the leading byte
const uint8_t utf8_size[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0};

uint16x4_t arm_parse_3_byte_utf8(uint8x16_t in) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  const uint8x16_t sh = simdutf_make_uint8x16_t(0, 2, 3, 5, 6, 8, 9, 11, 1, 1,
                                                4, 4, 7, 7, 10, 10);
#else
  const uint8x16_t sh = {0, 2, 3, 5, 6, 8, 9, 11, 1, 1, 4, 4, 7, 7, 10, 10};
#endif
  uint8x16_t perm = vqtbl1q_u8(in, sh);
  // Split into half vectors.
  // 10cccccc|1110aaaa
  uint8x8_t perm_low = vget_low_u8(perm); // no-op
  // 10bbbbbb|10bbbbbb
  uint8x8_t perm_high = vget_high_u8(perm);
  // xxxxxxxx 10bbbbbb
  uint16x4_t mid = vreinterpret_u16_u8(perm_high); // no-op
  // xxxxxxxx 1110aaaa
  uint16x4_t high = vreinterpret_u16_u8(perm_low); // no-op
  // Assemble with shift left insert.
  // xxxxxxaa aabbbbbb
  uint16x4_t mid_high = vsli_n_u16(mid, high, 6);
  // (perm_low << 8) | (perm_low >> 8)
  // xxxxxxxx 10cccccc
  uint16x4_t low = vreinterpret_u16_u8(vrev16_u8(perm_low));
  // Shift left insert into the low bits
  // aaaabbbb bbcccccc
  uint16x4_t composed = vsli_n_u16(low, mid_high, 6);
  return composed;
}

uint16x8_t arm_parse_2_byte_utf8(uint8x16_t in) {
  // 10bbbbbb 110aaaaa
  uint16x8_t upper = vreinterpretq_u16_u8(in);
  // (in << 8) | (in >> 8)
  // 110aaaaa 10bbbbbb
  uint16x8_t lower = vreinterpretq_u16_u8(vrev16q_u8(in));
  // 00000000 000aaaaa
  uint16x8_t upper_masked = vandq_u16(upper, vmovq_n_u16(0x1F));
  // Assemble with shift left insert.
  // 00000aaa aabbbbbb
  uint16x8_t composed = vsliq_n_u16(lower, upper_masked, 6);
  return composed;
}

uint32x4_t arm_parse_4_byte_utf8(uint8x16_t in) {
  // We want to take 3 4-byte UTF-8 code units and turn them into 3 4-byte
  // UTF-32 code units. This uses the same method as the fixed 3 byte
  // version, reversing and shift left insert. However, there is no need for
  // a shuffle mask now, just rev16 and rev32.
  //
  // This version does not use the LUT, but 4 byte sequences are less common
  // and the overhead of the extra memory access is less important than the
  // early branch overhead in shorter sequences, so it comes last.

  // Swap pairs of bytes
  // 10dddddd|10cccccc|10bbbbbb|11110aaa
  // 10cccccc 10dddddd|11110aaa 10bbbbbb
  uint16x8_t swap1 = vreinterpretq_u16_u8(vrev16q_u8(in));
  // Shift left and insert
  // xxxxcccc ccdddddd|xxxxxxxa aabbbbbb
  uint16x8_t merge1 = vsliq_n_u16(swap1, vreinterpretq_u16_u8(in), 6);
  // Swap 16-bit lanes
  // xxxxcccc ccdddddd xxxxxxxa aabbbbbb
  // xxxxxxxa aabbbbbb xxxxcccc ccdddddd
  uint32x4_t swap2 = vreinterpretq_u32_u16(vrev32q_u16(merge1));
  // Shift insert again
  // xxxxxxxx xxxaaabb bbbbcccc ccdddddd
  uint32x4_t merge2 = vsliq_n_u32(swap2, vreinterpretq_u32_u16(merge1), 12);
  // Clear the garbage
  // 00000000 000aaabb bbbbcccc ccdddddd
  uint32x4_t composed = vandq_u32(merge2, vmovq_n_u32(0x1FFFFF));
  return composed;
}

uint32x4_t arm_parse_3_1234_utf8(uint8x16_t in, size_t idx) {
  // Unlike UTF-16, doing a fast codepath doesn't have nearly as much benefit
  // due to surrogates no longer being involved.
  uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t *>(
      simdutf::tables::utf8_to_utf16::shufutf8[idx]));
  // 1 byte: 00000000 00000000 00000000 0ddddddd
  // 2 byte: 00000000 00000000 110ccccc 10dddddd
  // 3 byte: 00000000 1110bbbb 10cccccc 10dddddd
  // 4 byte: 11110aaa 10bbbbbb 10cccccc 10dddddd
  uint32x4_t perm = vreinterpretq_u32_u8(vqtbl1q_u8(in, sh));
  // Ascii
  uint32x4_t ascii = vandq_u32(perm, vmovq_n_u32(0x7F));
  uint32x4_t middle = vandq_u32(perm, vmovq_n_u32(0x3f00));
  // When converting the way we do, the 3 byte prefix will be interpreted as
  // the 18th bit being set, since the code would interpret the lead byte
  // (0b1110bbbb) as a continuation byte (0b10bbbbbb). To fix this, we can
  // either xor or do an 8 bit add of the 6th bit shifted right by 1. Since
  // NEON has shift right accumulate, we use that.
  //  4 byte   3 byte
  // 10bbbbbb 1110bbbb
  // 00000000 01000000 6th bit
  // 00000000 00100000 shift right
  // 10bbbbbb 0000bbbb add
  // 00bbbbbb 0000bbbb mask
  uint8x16_t correction =
      vreinterpretq_u8_u32(vandq_u32(perm, vmovq_n_u32(0x00400000)));
  uint32x4_t corrected = vreinterpretq_u32_u8(
      vsraq_n_u8(vreinterpretq_u8_u32(perm), correction, 1));
  // 00000000 00000000 0000cccc ccdddddd
  uint32x4_t cd = vsraq_n_u32(ascii, middle, 2);
  // Insert twice
  // xxxxxxxx xxxaaabb bbbbxxxx xxxxxxxx
  uint32x4_t ab = vbslq_u32(vmovq_n_u32(0x01C0000), vshrq_n_u32(corrected, 6),
                            vshrq_n_u32(corrected, 4));
  // 00000000 000aaabb bbbbcccc ccdddddd
  uint32x4_t composed = vbslq_u32(vmovq_n_u32(0xFFE00FFF), cd, ab);
  return composed;
}

uint16x8_t arm_parse_6_12_utf8(uint8x16_t in, size_t idx) {
  uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t *>(
      simdutf::tables::utf8_to_utf16::shufutf8[idx]));
  // Shuffle
  // 1 byte: 00000000 0bbbbbbb
  // 2 byte: 110aaaaa 10bbbbbb
  uint16x8_t perm = vreinterpretq_u16_u8(vqtbl1q_u8(in, sh));
  // Mask
  // 1 byte: 00000000 0bbbbbbb
  // 2 byte: 00000000 00bbbbbb
  uint16x8_t ascii = vandq_u16(perm, vmovq_n_u16(0x7f)); // 6 or 7 bits
  // 1 byte: 00000000 00000000
  // 2 byte: 000aaaaa 00000000
  uint16x8_t highbyte = vandq_u16(perm, vmovq_n_u16(0x1f00)); // 5 bits
  // Combine with a shift right accumulate
  // 1 byte: 00000000 0bbbbbbb
  // 2 byte: 00000aaa aabbbbbb
  uint16x8_t composed = vsraq_n_u16(ascii, highbyte, 2);
  return composed;
}

uint16x4_t arm_parse_4_123_utf8(uint8x16_t in, size_t idx) {
  // UTF-16 and UTF-32 use similar algorithms, but UTF-32 skips the narrowing.
  uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t *>(
      simdutf::tables::utf8_to_utf16::shufutf8[idx]));
  // XXX: depending on the system scalar instructions might be faster.
  // 1 byte: 00000000 00000000 0ccccccc
  // 2 byte: 00000000 110bbbbb 10cccccc
  // 3 byte: 1110aaaa 10bbbbbb 10cccccc
  uint32x4_t perm = vreinterpretq_u32_u8(vqtbl1q_u8(in, sh));
  // 1 byte: 00000000 0ccccccc
  // 2 byte: xx0bbbbb x0cccccc
  // 3 byte: xxbbbbbb x0cccccc
  uint16x4_t lowperm = vmovn_u32(perm);
  // Partially mask with bic (doesn't require a temporary register unlike and)
  // The shift left insert below will clear the top bits.
  // 1 byte: 00000000 00000000
  // 2 byte: xx0bbbbb 00000000
  // 3 byte: xxbbbbbb 00000000
  uint16x4_t middlebyte = vbic_u16(lowperm, vmov_n_u16(0x00FF));
  // ASCII
  // 1 byte: 00000000 0ccccccc
  // 2+byte: 00000000 00cccccc
  uint16x4_t ascii = vand_u16(lowperm, vmov_n_u16(0x7F));
  // Split into narrow vectors.
  // 2 byte: 00000000 00000000
  // 3 byte: 00000000 xxxxaaaa
  uint16x4_t highperm = vshrn_n_u32(perm, 16);
  // Shift right accumulate the middle byte
  // 1 byte: 00000000 0ccccccc
  // 2 byte: 00xx0bbb bbcccccc
  // 3 byte: 00xxbbbb bbcccccc
  uint16x4_t middlelow = vsra_n_u16(ascii, middlebyte, 2);
  // Shift left and insert the top 4 bits, overwriting the garbage
  // 1 byte: 00000000 0ccccccc
  // 2 byte: 00000bbb bbcccccc
  // 3 byte: aaaabbbb bbcccccc
  uint16x4_t composed = vsli_n_u16(middlelow, highperm, 12);
  return composed;
}

// memcpy for up to 64 bytes. Does no length checking. Make sure 64 bytes of
// space exist in the destination before calling this function.
simdutf_really_inline void arm_memcpy_small(uint8_t *dst, const uint8_t *src) {
  vst1q_u8(dst, vld1q_u8(src));
  vst1q_u8(dst + 16, vld1q_u8(src + 16));
  vst1q_u8(dst + 32, vld1q_u8(src + 32));
  vst1q_u8(dst + 48, vld1q_u8(src + 48));
}
