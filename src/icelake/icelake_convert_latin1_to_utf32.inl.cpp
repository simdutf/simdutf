void avx512_convert_latin1_to_utf32(const char *buf, size_t len,
                                    char32_t *utf32_output) {
  while (len >= 16) {
    // Load 16 Latin1 characters into a 128-bit register
    __m128i in = _mm_loadu_si128((__m128i *)buf);

    // Zero extend each set of 8 Latin1 characters to 16 32-bit integers using
    // vpmovzxbd
    __m512i out = _mm512_cvtepu8_epi32(in);

    // Store the results back to memory
    _mm512_storeu_si512((__m512i *)utf32_output, out);

    len -= 16;
    buf += 16;
    utf32_output += 16;
  }

  __mmask16 mask = __mmask16((1 << len) - 1);
  __m128i in = _mm_maskz_loadu_epi8(mask, buf);
  __m512i out = _mm512_cvtepu8_epi32(in);
  _mm512_mask_storeu_epi32((__m512i *)utf32_output, mask, out);
}
