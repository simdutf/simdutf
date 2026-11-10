std::pair<const char *, char *>
avx2_convert_latin1_to_utf8(const char *latin1_input, size_t len,
                            char *utf8_output) {
  const char *end = latin1_input + len;
  // A two-byte sequence is stored in memory order inside its 16-bit lane, i.e.
  // [110xxxxx][10xxxxxx], so the low byte carries the 0xc0 prefix and the high
  // byte the 0x80 one.
  const __m256i v_80c0 = _mm256_set1_epi16((int16_t)0x80c0);
  const size_t safety_margin = 12;

  while (end - latin1_input >= std::ptrdiff_t(16 + safety_margin)) {
    const __m128i in8 = _mm_loadu_si128((__m128i *)latin1_input);
    // One bit per latin1 character, in natural order: bit i is set when
    // character i is not ASCII, i.e. when it expands to two UTF-8 bytes.
    // This single instruction both detects the ASCII fast path and provides
    // the index into the packing table.
    const uint32_t nonascii_bitmask =
        static_cast<uint32_t>(_mm_movemask_epi8(in8));
    if (nonascii_bitmask == 0) { // ASCII fast path!!!!
      // 1. store (16 bytes)
      _mm_storeu_si128((__m128i *)utf8_output, in8);
      // 2. adjust pointers
      latin1_input += 16;
      utf8_output += 16;
      continue; // we are done for this round!
    }
    // We proceed only with the first 16 bytes.
    const __m256i in = _mm256_cvtepu8_epi16(in8);
    // A non-ASCII character sign extends to 0xFFxx, so both of its bytes have
    // their most significant bit set: that is exactly the per-byte selector
    // expected by blendv_epi8.
    const __m256i nonascii_bytemask = _mm256_cvtepi8_epi16(in8);

    // 1. prepare 2-byte values
    // input 16-bit word : [0000|0000|aabb|bbbb] x 16
    // expected output   : [1100|00aa|10bb|bbbb] x 16, i.e. the low byte of the
    // lane holds the first UTF-8 byte and the high byte the second one.
    const __m256i v_003f = _mm256_set1_epi16((int16_t)0x003f);

    // t0 = [0000|0000|0000|0aaa] : aaaaa = cp >> 6, and since cp < 0x100 the
    // shift brings no garbage into the high byte
    const __m256i t0 = _mm256_srli_epi16(in, 6);
    // t1 = [0000|0000|00bb|bbbb] : bbbbbb = cp & 0x3f
    const __m256i t1 = _mm256_and_si256(in, v_003f);
    // t2 = [00bb|bbbb|0000|0000] : move the continuation byte to the high byte
    const __m256i t2 = _mm256_slli_epi16(t1, 8);
    // t3 = [00bb|bbbb|0000|0aaa]
    const __m256i t3 = _mm256_or_si256(t0, t2);
    // t4 = [10bb|bbbb|1100|0aaa]
    const __m256i t4 = _mm256_or_si256(t3, v_80c0);

    // 2. merge ASCII and 2-byte codewords
    // ASCII characters are kept verbatim (their high byte is zero), the
    // others use the two-byte sequence computed above.
    const __m256i utf8_unpacked =
        _mm256_blendv_epi8(in, t4, nonascii_bytemask);

    // 3. look up the packing masks. The table is indexed directly by the
    // movemask value: one byte per latin1 character, natural order, set bit
    // meaning "two bytes".
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
    // 5. store bytes. A half produces 8 bytes for its ASCII characters plus
    // one extra byte for each non-ASCII one, i.e. 8 + popcount(half). The
    // value does not depend on the shuffle, so the popcnt is off the
    // shuffle's latency chain.
    _mm_storeu_si128((__m128i *)utf8_output,
                     _mm256_castsi256_si128(utf8_packed));
    utf8_output += 8 + count_ones(uint8_t(nonascii_bitmask));
    _mm_storeu_si128((__m128i *)utf8_output,
                     _mm256_extractf128_si256(utf8_packed, 1));
    utf8_output += 8 + count_ones(uint8_t(nonascii_bitmask >> 8));

    // 6. adjust pointers
    latin1_input += 16;
    continue;

  } // while
  return std::make_pair(latin1_input, utf8_output);
}
