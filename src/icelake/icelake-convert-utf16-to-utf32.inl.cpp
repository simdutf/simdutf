// file included directly

/*
  Returns a pair: the first unprocessed byte from buf and utf32_output
  A scalar routing should carry on the conversion of the tail.
*/
std::pair<const char16_t*, char32_t*> convert_utf16_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) {
  const char16_t* end = buf + len;
  const __m512i v_f800 = _mm512_set1_epi16((uint16_t)0xf800);
  const __m512i v_d800 = _mm512_set1_epi16((uint16_t)0xd800);

  while (buf + 32 <= end) {
    __m512i in = _mm512_loadu_si512((__m512i*)buf);

    // S - bitmask for surrogates
    const __mmask32 S = _mm512_cmpeq_epi16_mask(_mm512_and_si512(in, v_f800), v_d800);

    if (S == 0x00000000) {
    // extend all thirty-two 16-bit words to thirty-two 32-bit words
      _mm512_storeu_si512((__m512i *)(utf32_output), _mm512_cvtepu16_epi32(_mm512_castsi512_si256(in)));
      _mm512_storeu_si512((__m512i *)(utf32_output + 16), _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(in,1)));
      utf32_output += 32;
      buf += 32;
    // surrogate pair(s) in a register
    } else {
      const __m512i v_fc00 = _mm512_set1_epi16((uint16_t)0xfc00);
      // L - bitmask for low surrogates
      const __mmask32 L = _mm512_cmpeq_epi16_mask(_mm512_and_si512(in, v_fc00), v_d800);
      // H - bitmask for high surrogates
      const __mmask32 H = ~L & S;
      const __mmask32 V = (H ^ (L << 1));   // A low surrogate must be followed by high one and a high one must be preceded by a low one.
                                            // If valid, V should be equal to 0. We must also handle when the last word of the chunk is a
                                            // low surrogate.

      if(V == 0) {
        // valid case
        /*
            Input surrogate pair:
            |1101.10aa.aaaa.aaaa|1101.10bb.bbbb.bbbb|
                high surrogate      low surrogate
        */
        /*  1. Expand all words to 32-bit words
            in  |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0000.0000.0000.1101.10bb.bbbb.bbbb|
        */
        const __m512i first = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(in));
        const __m512i second = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(in,1));

        /*  2. Shift by one 16-bit word to align high surrogates with low surrogates
            in      |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0000.0000.0000.1101.10bb.bbbb.bbbb|
            shifted |????.????.????.????.????.????.????.????|0000.0000.0000.0000.1101.11aa.aaaa.aaaa|
        */
        const __m512i shifted_first = _mm512_alignr_epi32(second, first, 1);
        const __m512i shifted_second = _mm512_alignr_epi32(_mm512_setzero_si512(), second, 1);

        /*  3. Align all low surrogates in first and second by shifting to the left by 10 bits
            |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0011.0110.bbbb.bbbb.bb00.0000.0000|
        */
        const __m512i aligned_first = _mm512_mask_slli_epi32(first, (__mmask16)L, first, 10);
        const __m512i aligned_second = _mm512_mask_slli_epi32(second, (__mmask16)(L>>16), second, 10);

        /*  4. Remove surrogate prefixes and add offset 0x10000 by adding in, shifted and constant
            in      |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0011.0110.bbbb.bbbb.bb00.0000.0000|
            shifted |????.????.????.????.????.????.????.????|0000.0000.0000.0000.1101.11aa.aaaa.aaaa|
            constant|1111.1100.1010.0000.0010.0100.0000.0000|1111.1100.1010.0000.0010.0100.0000.0000|
        */
        const __m512i constant = _mm512_set1_epi32((uint32_t)0xfca02400);
        const __m512i added_first = _mm512_mask_add_epi32(aligned_first, (__mmask16)L, aligned_first, shifted_first);
        const __m512i utf32_first = _mm512_mask_add_epi32(added_first, (__mmask16)L, added_first, constant);

        const __m512i added_second = _mm512_mask_add_epi32(aligned_second, (__mmask16)(L>>16), aligned_second, shifted_second);
        const __m512i utf32_second = _mm512_mask_add_epi32(added_second, (__mmask16)(L>>16), added_second, constant);

        //  5. Store all valid UTF-32 words (only high surrogate positions are invalid)
        _mm512_mask_compressstoreu_epi32((__m512i *)utf32_output, (__mmask16)(~H), utf32_first);
        utf32_output += count_ones((uint16_t)(~H));
        _mm512_mask_compressstoreu_epi32((__m512i *)utf32_output, (__mmask16)((~H)>>16), utf32_second);
        utf32_output += count_ones((~H)>>16);

        buf += 32;
        // If last word in the chunk is a low surrogate, we process it in the next iteration to check if valid
        if ((L & 0x80000000) == 0x80000000) {
          utf32_output--;
          buf--;
        }
      } else {
        // invalid case
        return std::make_pair(nullptr, utf32_output);
      }
    }
  } // while
  return std::make_pair(buf, utf32_output);
}