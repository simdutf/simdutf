internal::pair<const char *, char *>
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

  return internal::make_pair(latin_input, utf8_output);
}
