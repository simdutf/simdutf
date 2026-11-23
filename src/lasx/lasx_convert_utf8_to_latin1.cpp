// depends on "tables/utf8_to_latin1_tables.h"

simdutf_really_inline size_t convert_1_2_byte_utf8_to_latin1_16(
    const char *input, uint16_t leading_mask, char *latin1_output) {
  const __m128i in = __lsx_vld(reinterpret_cast<const uint8_t *>(input), 0);
  const __m128i next = __lsx_vld(reinterpret_cast<const uint8_t *>(input), 1);
  const __m128i combined =
      __lsx_vbitsel_v(__lsx_vslli_b(in, 6), next, __lsx_vldi(0x3f));
  const __m128i value =
      __lsx_vbitsel_v(in, combined, __lsx_vsle_bu(__lsx_vldi(0xc0), in));
  const uint8_t lo = uint8_t(leading_mask);
  const uint8_t hi = uint8_t(leading_mask >> 8);
  __m128i shuffle =
      __lsx_vinsgr2vr_d(__lsx_vldi(0), tables::utf8_to_latin1::pack_8[lo], 0);
  shuffle = __lsx_vinsgr2vr_d(
      shuffle, tables::utf8_to_latin1::pack_8[hi] + 0x0808080808080808, 1);
  const __m128i packed = __lsx_vshuf_b(__lsx_vldi(0), value, shuffle);
  const size_t lo_count = count_ones(lo);
  __lsx_vstelm_d(packed, reinterpret_cast<uint8_t *>(latin1_output), 0, 0);
  __lsx_vstelm_d(packed, reinterpret_cast<uint8_t *>(latin1_output + lo_count),
                 0, 1);
  return lo_count + count_ones(hi);
}
