// depends on "tables/utf8_to_latin1_tables.h"

simdutf_really_inline size_t convert_1_2_byte_utf8_to_latin1_16(
    const char *input, uint16_t leading_mask, char *latin1_output) {
  const auto in = vector_u8::load(input);
  const auto next = vector_u8::load(input + 1);
  const auto combined = in.shl<6>() | (next & uint8_t(0x3f));
  const vector_u8 value =
      vec_sel(in.value, combined.value, (in >= vector_u8(0xc0)).value);
  const uint8_t lo = uint8_t(leading_mask);
  const uint8_t hi = uint8_t(leading_mask >> 8);
  uint64_t indexes[2] = {tables::utf8_to_latin1::pack_8[lo],
                         tables::utf8_to_latin1::pack_8[hi] +
                             0x0808080808080808};
#if SIMDUTF_IS_BIG_ENDIAN
  indexes[0] = __builtin_bswap64(indexes[0]);
  indexes[1] = __builtin_bswap64(indexes[1]);
#endif
  const auto shuffle = vector_u8::load(indexes);
  const auto packed = shuffle.lookup_16(value);
  uint8_t buffer[16];
  packed.store(buffer);
  const size_t lo_count = count_ones(lo);
  std::memcpy(latin1_output, buffer, 8);
  std::memcpy(latin1_output + lo_count, buffer + 8, 8);
  return lo_count + count_ones(hi);
}
