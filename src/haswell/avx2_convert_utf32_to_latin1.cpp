std::pair<const char32_t *, char *>
avx2_convert_utf32_to_latin1(const char32_t *buf, size_t len,
                             char *latin1_output) {
  const size_t rounded_len =
      len & ~0x1F; // Round down to nearest multiple of 32

  __m256i high_bytes_mask = _mm256_set1_epi32(0xFFFFFF00);
  __m256i shufmask = _mm256_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                     -1, 12, 8, 4, 0, -1, -1, -1, -1, -1, -1,
                                     -1, -1, -1, -1, -1, -1, 12, 8, 4, 0);

  for (size_t i = 0; i < rounded_len; i += 16) {
    __m256i in1 = _mm256_loadu_si256((__m256i *)buf);
    __m256i in2 = _mm256_loadu_si256((__m256i *)(buf + 8));

    __m256i check_combined = _mm256_or_si256(in1, in2);

    if (!_mm256_testz_si256(check_combined, high_bytes_mask)) {
      return std::make_pair(nullptr, latin1_output);
    }

    __m256i shuffled1 = _mm256_shuffle_epi8(in1, shufmask);
    _mm_storeu_si128((__m128i *)latin1_output,
                     _mm256_castsi256_si128(shuffled1));
    _mm_storeu_si128((__m128i *)(latin1_output + 4),
                     _mm256_extracti128_si256(shuffled1, 1));

    __m256i shuffled2 = _mm256_shuffle_epi8(in2, shufmask);
    _mm_storeu_si128((__m128i *)(latin1_output + 8),
                     _mm256_castsi256_si128(shuffled2));
    *reinterpret_cast<uint32_t *>(latin1_output + 12) =
        _mm_cvtsi128_si32(_mm256_extracti128_si256(shuffled2, 1));

    latin1_output += 16;
    buf += 16;
  }

  return std::make_pair(buf, latin1_output);
}