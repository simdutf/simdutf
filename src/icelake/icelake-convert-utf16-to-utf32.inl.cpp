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
      const __mmask32 V = (L ^ (H >> 1)) | (H ^ (L << 1));
                                                                    // A low surrogate must be followed by high one and a high one must be preceded by a low one.
                                                                    // If valid, V should be equal to 0, except if a surrogate is the last word of the chunk.
                                                                    // This is special (valid) case that we need to handle.

      if(V == 0 || V == 0x80000000) {
        // valid case
        /*
            Input surrogate pair:
            |1101.10aa.aaaa.aaaa|1101.10bb.bbbb.bbbb|
                high surrogate      low surrogate
        */

        /*  0. Remove all surrogate prefixes
            |0000.00aa.aaaa.aaaa|0000.00bb.bbbb.bbbb|
        */
        const __m512i v_0000_03ff = _mm512_set1_epi16((uint16_t)0x3ff);
        const __m512i no_prefix = _mm512_mask_blend_epi16(S, in, _mm512_and_si512(in, v_0000_03ff));

        /*  1. Shift whole register by 16 bits (one word) to the right
            |????.????.????.????|0000.00aa.aaaa.aaaa|
        */
        const __m512i shifted = _mm512_and_si512(v_0000_03ff, _mm512_loadu_si512((__m512i*)(buf+1)));

        /*  2. Expand all words in no_prefix and shifted to 32-bit words
            no_prefix |0000.0000.0000.0000.0000.00aa.aaaa.aaaa|0000.0000.0000.0000.0000.00bb.bbbb.bbbb|
            shifted   |????.????.????.????.????.????.????.????|0000.0000.0000.0000.0000.00aa.aaaa.aaaa|
        */
        const __m512i first = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(no_prefix));
        const __m512i second = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(no_prefix,1));
        const __m512i shifted_first = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(shifted));
        const __m512i shifted_second = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(shifted,1));

        /*  3. Align all low surrogates in first and second by shifting to the left by 10 bits
            |0000.0000.0000.0000.0000.00aa.aaaa.aaaa|0000.0000.0000.bbbb.bbbb.bb00.0000.0000|
        */
        const __m512i aligned_first = _mm512_mask_slli_epi32(first, (__mmask16)L, first, 10);
        const __m512i aligned_second = _mm512_mask_slli_epi32(second, (__mmask16)(L>>16), second, 10);

        /*  4. Join fields A and B in lower 32-bit word (lower surrogate position)
            |0000.0000.0000.0000.0000.00aa.aaaa.aaaa|0000.0000.0000.bbbb.bbbb.bbaa.aaaa.aaaa|
        */
        const __m512i joined_first = _mm512_mask_or_epi32(aligned_first, (__mmask16)L, shifted_first, aligned_first);
        const __m512i joined_second = _mm512_mask_or_epi32(aligned_second, (__mmask16)(L>>16), shifted_second, aligned_second);

        //  5. Add offset 0x10000 to lower 32-bit word to obtain valid UTF-32
        const __m512i v_0001_0000 = _mm512_set1_epi32((uint32_t)0x10000);
        const __m512i utf32_first = _mm512_mask_add_epi32(joined_first, (__mmask16)L, joined_first, v_0001_0000);
        const __m512i utf32_second = _mm512_mask_add_epi32(joined_second, (__mmask16)(L>>16), joined_second, v_0001_0000);

        //  6. Store all valid UTF-32 words (only high surrogate positions are invalid)
        _mm512_mask_compressstoreu_epi32((__m512i *)utf32_output, (__mmask16)(~H), utf32_first);
        utf32_output += count_ones((uint16_t)(~H));
        _mm512_mask_compressstoreu_epi32((__m512i *)utf32_output, (__mmask16)((~H)>>16), utf32_second);
        utf32_output += count_ones((~H)>>16);

        buf += 32;
        // If last word in the chunk is a lone a surrogate, we process it in the next iteration to check if valid
        if (V == 0x80000000) {
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