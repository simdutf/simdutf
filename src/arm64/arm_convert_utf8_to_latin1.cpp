// depends on "tables/utf8_to_latin1_tables.h"

simdutf_really_inline size_t convert_1_2_byte_utf8_to_latin1_16(
    const char *input, uint16_t leading_mask, char *latin1_output) {
  const uint8x16_t in = vld1q_u8(reinterpret_cast<const uint8_t *>(input));
  const uint8x16_t next =
      vld1q_u8(reinterpret_cast<const uint8_t *>(input + 1));
  const uint8x16_t combined = vsliq_n_u8(next, in, 6);
  const uint8x16_t value =
      vbslq_u8(vcgeq_u8(in, vdupq_n_u8(0xc0)), combined, in);
  const uint8_t lo = uint8_t(leading_mask);
  const uint8_t hi = uint8_t(leading_mask >> 8);
  const uint8x16_t shuffle = vreinterpretq_u8_u64(vcombine_u64(
      vcreate_u64(tables::utf8_to_latin1::pack_8[lo]),
      vcreate_u64(tables::utf8_to_latin1::pack_8[hi] + 0x0808080808080808)));
  const uint8x16_t packed = vqtbl1q_u8(value, shuffle);
  const size_t lo_count = count_ones(lo);
  vst1_u8(reinterpret_cast<uint8_t *>(latin1_output), vget_low_u8(packed));
  vst1_u8(reinterpret_cast<uint8_t *>(latin1_output + lo_count),
          vget_high_u8(packed));
  return lo_count + count_ones(hi);
}
