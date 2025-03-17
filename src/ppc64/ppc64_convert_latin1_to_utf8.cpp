/*
 * reads a vector of uint16 values
 * bits after 11th are ignored
 * first 11 bits are encoded into utf8
 * !important! utf8_output must have at least 16 writable bytes
 */
simdutf_really_inline void
write_v_u16_11bits_to_utf8(const vector_u16 v_u16, char *&utf8_output,
                           const vector_u8 one_byte_bytemask,
                           const uint16_t one_byte_bitmask) {

  // 0b1100_0000_1000_0000
  const auto v_c080 = vector_u16(0xc080);
  // 0b0011_1111_0000_0000
  const auto v_1f00 = vector_u16(0x1f00);
  // 0b0000_0000_0011_1111
  const auto v_003f = vector_u16(0x003f);

  // 1. prepare 2-byte values
  // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
  // expected output   : [110a|aaaa|10bb|bbbb] x 8

  // t0 = [0000|0000|00bb|bbbb]
  const auto t0 = v_u16 & v_003f;
  // t1 = [000a|aaaa|bbbb|bb00]
  const auto t1 = v_u16.shl<2>();
  // t2 = [000a|aaaa|00bb|bbbb]
  const auto t2 = select(v_1f00, t1, t0);
  // t3 = [110a|aaaa|10bb|bbbb]
  const auto t3 = t2 | v_c080;

  // 2. merge ASCII and 2-byte codewords
  const auto utf8_unpacked1 =
      select(one_byte_bytemask, as_vector_u8(v_u16), as_vector_u8(t3));

#if SIMDUTF_IS_BIG_ENDIAN
  const auto tmp = as_vector_u16(utf8_unpacked1).swap_bytes();
#else
  const auto tmp = as_vector_u16(utf8_unpacked1);
#endif // SIMDUTF_IS_BIG_ENDIAN
  const auto utf8_unpacked = as_vector_u8(tmp);

  // 3. prepare bitmask for 8-bit lookup
  //    one_byte_bitmask = hhggffeeddccbbaa -- the bits are doubled (h - MSB, a
  //    - LSB)
  const uint16_t m0 = one_byte_bitmask & 0x5555;      // m0 = 0h0g0f0e0d0c0b0a
  const uint16_t m1 = static_cast<uint16_t>(m0 >> 7); // m1 = 00000000h0g0f0e0
  const uint8_t m2 = static_cast<uint8_t>((m0 | m1) & 0xff); // m2 = hdgcfbea
  // 4. pack the bytes
  const uint8_t *row =
      &simdutf::tables::utf16_to_utf8::pack_1_2_utf8_bytes[m2][0];
  const auto shuffle = vector_u8::load(row + 1);
  const auto utf8_packed = shuffle.lookup_16(utf8_unpacked);

  // 5. store bytes
  utf8_packed.store(utf8_output);

  // 6. adjust pointers
  utf8_output += row[0];
}

inline void write_v_u16_11bits_to_utf8(const vector_u16 v_u16,
                                       char *&utf8_output,
                                       const vector_u16 v_0000,
                                       const vector_u16 v_ff80) {
  // no bits set above 7th bit
  const auto one_byte_bytemask = (v_u16 & v_ff80) == v_0000;
  const uint16_t one_byte_bitmask = one_byte_bytemask.to_bitmask();

  write_v_u16_11bits_to_utf8(v_u16, utf8_output,
                             as_vector_u8(one_byte_bytemask), one_byte_bitmask);
}

std::pair<const char *const, char *const>
ppc64_convert_latin1_to_utf8(const char *latin_input,
                             const size_t latin_input_length,
                             char *utf8_output) {
  const char *end = latin_input + latin_input_length;

  const auto v_0000 = vector_u16::zero();
  const auto v_00 = vector_u8::zero();

  // 0b1111_1111_1000_0000
  const auto v_ff80 = vector_u16(0xff80);

#if SIMDUTF_IS_BIG_ENDIAN
  const auto latin_1_half_into_u16_byte_mask =
      vector_u8(16, 0, 16, 1, 16, 2, 16, 3, 16, 4, 16, 5, 16, 6, 16, 7);
  const auto latin_2_half_into_u16_byte_mask =
      vector_u8(16, 8, 16, 9, 16, 10, 16, 11, 16, 12, 16, 13, 16, 14, 16, 15);
#else
  const auto latin_1_half_into_u16_byte_mask =
      vector_u8(0, 16, 1, 16, 2, 16, 3, 16, 4, 16, 5, 16, 6, 16, 7, 16);
  const auto latin_2_half_into_u16_byte_mask =
      vector_u8(8, 16, 9, 16, 10, 16, 11, 16, 12, 16, 13, 16, 14, 16, 15, 16);
#endif // SIMDUTF_IS_BIG_ENDIAN

  // each latin1 takes 1-2 utf8 bytes
  // slow path writes useful 8-15 bytes twice (eagerly writes 16 bytes and then
  // adjust the pointer) so the last write can exceed the utf8_output size by
  // 8-1 bytes by reserving 8 extra input bytes, we expect the output to have
  // 8-16 bytes free
  while (end - latin_input >= 16 + 8) {
    // Load 16 Latin1 characters (16 bytes) into a 128-bit register
    const auto v_latin = vector_u8::load(latin_input);

    if (v_latin.is_ascii()) { // ASCII fast path!!!!
      v_latin.store(utf8_output);
      latin_input += 16;
      utf8_output += 16;
      continue;
    }

    // assuming a/b are bytes and A/B are uint16 of the same value
    // aaaa_aaaa_bbbb_bbbb -> AAAA_AAAA
    const vector_u16 v_u16_latin_1_half =
        as_vector_u16(latin_1_half_into_u16_byte_mask.lookup_32(v_latin, v_00));

    // aaaa_aaaa_bbbb_bbbb -> BBBB_BBBB
    const vector_u16 v_u16_latin_2_half =
        as_vector_u16(latin_2_half_into_u16_byte_mask.lookup_32(v_latin, v_00));

    write_v_u16_11bits_to_utf8(v_u16_latin_1_half, utf8_output, v_0000, v_ff80);
    write_v_u16_11bits_to_utf8(v_u16_latin_2_half, utf8_output, v_0000, v_ff80);
    latin_input += 16;
  }

  if (end - latin_input >= 16) {
    // Load 16 Latin1 characters (16 bytes) into a 128-bit register
    const auto v_latin = vector_u8::load(latin_input);

    if (v_latin.is_ascii()) { // ASCII fast path!!!!
      v_latin.store(utf8_output);
      latin_input += 16;
      utf8_output += 16;
    } else {
      // assuming a/b are bytes and A/B are uint16 of the same value
      // aaaa_aaaa_bbbb_bbbb -> AAAA_AAAA
      const auto v_u16_latin_1_half = as_vector_u16(
          latin_1_half_into_u16_byte_mask.lookup_32(v_latin, v_00));

      write_v_u16_11bits_to_utf8(v_u16_latin_1_half, utf8_output, v_0000,
                                 v_ff80);
      latin_input += 8;
    }
  }

  return std::make_pair(latin_input, utf8_output);
}
