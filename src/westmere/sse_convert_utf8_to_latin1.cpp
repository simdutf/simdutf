// depends on "tables/utf8_to_latin1_tables.h"

simdutf_really_inline size_t convert_1_2_byte_utf8_to_latin1_16(
    const char *input, uint16_t leading_mask, char *latin1_output) {
  const __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
  const __m128i next =
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + 1));
  const __m128i combined = _mm_or_si128(
      _mm_and_si128(_mm_slli_epi16(in, 6), _mm_set1_epi8(char(0xc0))),
      _mm_and_si128(next, _mm_set1_epi8(0x3f)));
  const __m128i value =
      _mm_blendv_epi8(in, combined, _mm_and_si128(in, _mm_add_epi8(in, in)));
  const uint8_t lo = uint8_t(leading_mask);
  const uint8_t hi = uint8_t(leading_mask >> 8);
  const __m128i shuffle = _mm_set_epi64x(
      int64_t(tables::utf8_to_latin1::pack_8[hi] + 0x0808080808080808),
      int64_t(tables::utf8_to_latin1::pack_8[lo]));
  const __m128i packed = _mm_shuffle_epi8(value, shuffle);
  const size_t lo_count = count_ones(lo);
  _mm_storel_epi64(reinterpret_cast<__m128i *>(latin1_output), packed);
  _mm_storel_epi64(reinterpret_cast<__m128i *>(latin1_output + lo_count),
                   _mm_unpackhi_epi64(packed, packed));
  return lo_count + count_ones(hi);
}
