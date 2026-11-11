std::pair<const char *, char *>
avx2_convert_latin1_to_utf8(const char *latin1_input, size_t len,
                            char *utf8_output) {
  const char *end = latin1_input + len;
  // A two-byte sequence is stored in memory order in its 16-bit lane, i.e.
  // [110xxxxx][10xxxxxx]: 0xc0 goes in the low byte, 0x80 in the high one.
  const __m256i v_80c0 = _mm256_set1_epi16((int16_t)0x80c0);
  const size_t safety_margin = 12;

  while (end - latin1_input >= std::ptrdiff_t(16 + safety_margin)) {
    const __m128i in8 = _mm_loadu_si128((__m128i *)latin1_input);
    // Bit i is set when character i is not ASCII, i.e. when it expands to two
    // bytes. This single instruction both tests the ASCII fast path and gives
    // the index into the packing table.
    const uint32_t nonascii_bitmask =
        static_cast<uint32_t>(_mm_movemask_epi8(in8));
    if (nonascii_bitmask == 0) { // ASCII fast path
      // 1. store (16 bytes)
      _mm_storeu_si128((__m128i *)utf8_output, in8);
      // 2. adjust pointers
      latin1_input += 16;
      utf8_output += 16;
      continue;
    }
    const __m256i in = _mm256_cvtepu8_epi16(in8);
    // A non-ASCII character sign extends to 0xFFxx, so every byte of the lane
    // has its most significant bit set: exactly the selector blendv_epi8
    // expects.
    const __m256i nonascii_bytemask = _mm256_cvtepi8_epi16(in8);

    // 1. prepare 2-byte values
    // Encode every character as a two-byte sequence; lane layout, high byte
    // first: in = [0000|0000|aabb|bbbb] -> t4 = [10bb|bbbb|1100|00aa].
    const __m256i v_003f = _mm256_set1_epi16((int16_t)0x003f);

    // aa = cp >> 6; cp < 0x100, so the high byte stays clear
    const __m256i t0 = _mm256_srli_epi16(in, 6);
    // bbbbbb = cp & 0x3f
    const __m256i t1 = _mm256_and_si256(in, v_003f);
    // move the continuation byte into the high byte
    const __m256i t2 = _mm256_slli_epi16(t1, 8);
    // [00bb|bbbb|0000|00aa]
    const __m256i t3 = _mm256_or_si256(t0, t2);
    // [10bb|bbbb|1100|00aa]
    const __m256i t4 = _mm256_or_si256(t3, v_80c0);

    // 2. merge ASCII and 2-byte codewords
    // Keep ASCII characters verbatim; t4 is used for the others only.
    const __m256i utf8_unpacked = _mm256_blendv_epi8(in, t4, nonascii_bytemask);

    // 3. look up the packing masks, indexed by the bitmask directly.
    const uint8_t *row =
        &simdutf::tables::latin1_to_utf8::pack_1_2_utf8_bytes_from_latin1
            [uint8_t(nonascii_bitmask)][0];
    const uint8_t *row_2 =
        &simdutf::tables::latin1_to_utf8::pack_1_2_utf8_bytes_from_latin1
            [uint8_t(nonascii_bitmask >> 8)][0];

    const __m128i shuffle = _mm_loadu_si128((__m128i *)row);
    const __m128i shuffle_2 = _mm_loadu_si128((__m128i *)row_2);

    // 4. pack the bytes
    const __m256i utf8_packed = _mm256_shuffle_epi8(
        utf8_unpacked, _mm256_setr_m128i(shuffle, shuffle_2));
    // 5. store bytes; a half yields 8 bytes plus one per non-ASCII character
    _mm_storeu_si128((__m128i *)utf8_output,
                     _mm256_castsi256_si128(utf8_packed));
    utf8_output += 8 + count_ones(uint8_t(nonascii_bitmask));
    _mm_storeu_si128((__m128i *)utf8_output,
                     _mm256_extractf128_si256(utf8_packed, 1));
    utf8_output += 8 + count_ones(uint8_t(nonascii_bitmask >> 8));

    // 6. adjust pointers
    latin1_input += 16;
  } // while
  return std::make_pair(latin1_input, utf8_output);
}
