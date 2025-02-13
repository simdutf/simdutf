std::pair<const char32_t *, char *>
avx2_convert_utf32_to_latin1(const char32_t *buf, size_t len,
                             char *latin1_output) {
  const size_t rounded_len =
      len & ~0x1F; // Round down to nearest multiple of 32

  const __m256i high_bytes_mask = _mm256_set1_epi32(0xFFFFFF00);

  for (size_t i = 0; i < rounded_len; i += 4 * 8) {
    __m256i a = _mm256_loadu_si256((__m256i *)(buf + 0 * 8));
    __m256i b = _mm256_loadu_si256((__m256i *)(buf + 1 * 8));
    __m256i c = _mm256_loadu_si256((__m256i *)(buf + 2 * 8));
    __m256i d = _mm256_loadu_si256((__m256i *)(buf + 3 * 8));

    const __m256i check_combined =
        _mm256_or_si256(_mm256_or_si256(a, b), _mm256_or_si256(c, d));

    if (!_mm256_testz_si256(check_combined, high_bytes_mask)) {
      return std::make_pair(nullptr, latin1_output);
    }

    b = _mm256_slli_epi32(b, 1 * 8);
    c = _mm256_slli_epi32(c, 2 * 8);
    d = _mm256_slli_epi32(d, 3 * 8);

    // clang-format off

    // a  = [.. .. .. a7|.. .. .. a6|.. .. .. a5|.. .. .. a4||.. .. .. a3|.. .. .. a2|.. .. .. a1|.. .. .. a0]
    // b  = [.. .. b7 ..|.. .. b6 ..|.. .. b5 ..|.. .. b4 ..||.. .. b3 ..|.. .. b2 ..|.. .. b1 ..|.. .. b0 ..]
    // c  = [.. c7 .. ..|.. c6 .. ..|.. c5 .. ..|.. c4 .. ..||.. c3 .. ..|.. c2 .. ..|.. c1 .. ..|.. c0 .. ..]
    // d  = [d7 .. .. ..|d6 .. .. ..|d5 .. .. ..|d4 .. .. ..||d3 .. .. ..|d2 .. .. ..|d1 .. .. ..|d0 .. .. ..]

    // t0 = [d7 c7 b7 a7|d6 c6 b6 a6|d5 c5 b5 a5|d4 c4 b4 a4||d3 c3 b3 a3|d2 c2 b2 a2|d1 c1 b1 a1|d0 c0 b0 a0]
    const __m256i t0 =
        _mm256_or_si256(_mm256_or_si256(a, b), _mm256_or_si256(c, d));

    // shuffle bytes within 128-bit lanes
    // t1 = [d7 d6 d5 d4|c7 c6 c5 c4|b7 b6 b5 b4|a7 a6 a5 a4||d3 d2 d1 d0|c3 c2 c1 c0|b3 b2 b1 b0|a3 a2 a1 a0]
    const __m256i shuffle_bytes =
        _mm256_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15,
                         0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

    const __m256i t1 = _mm256_shuffle_epi8(t0, shuffle_bytes);

    // reshuffle dwords
    // t2 = [d7 d6 d5 d4|d3 d2 d1 d0|c7 c6 c5 c4|c3 c2 c1 c0||b7 b6 b5 b4|b3 b2 b1 b0|a7 a6 a5 a4|a3 a2 a1 a0]
    const __m256i shuffle_dwords = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);
    const __m256i t2 = _mm256_permutevar8x32_epi32(t1, shuffle_dwords);
// clang format on

    _mm256_storeu_si256((__m256i *)latin1_output, t2);

    latin1_output += 32;
    buf += 32;
  }

  return std::make_pair(buf, latin1_output);
}

std::pair<result, char *>
avx2_convert_utf32_to_latin1_with_errors(const char32_t *buf, size_t len,
                                         char *latin1_output) {
  const size_t rounded_len =
      len & ~0x1F; // Round down to nearest multiple of 32

  const char32_t *start = buf;

  const __m256i high_bytes_mask = _mm256_set1_epi32(0xFFFFFF00);

  for (size_t i = 0; i < rounded_len; i += 4 * 8) {
    __m256i a = _mm256_loadu_si256((__m256i *)(buf + 0 * 8));
    __m256i b = _mm256_loadu_si256((__m256i *)(buf + 1 * 8));
    __m256i c = _mm256_loadu_si256((__m256i *)(buf + 2 * 8));
    __m256i d = _mm256_loadu_si256((__m256i *)(buf + 3 * 8));

    const __m256i check_combined =
        _mm256_or_si256(_mm256_or_si256(a, b), _mm256_or_si256(c, d));

    if (!_mm256_testz_si256(check_combined, high_bytes_mask)) {
      // Fallback to scalar code for handling errors
      for (int k = 0; k < 4 * 8; k++) {
        char32_t codepoint = buf[k];
        if (codepoint <= 0xFF) {
          *latin1_output++ = static_cast<char>(codepoint);
        } else {
          return std::make_pair(result(error_code::TOO_LARGE, buf - start + k),
                                latin1_output);
        }
      }
    }

    b = _mm256_slli_epi32(b, 1 * 8);
    c = _mm256_slli_epi32(c, 2 * 8);
    d = _mm256_slli_epi32(d, 3 * 8);

    const __m256i t0 =
        _mm256_or_si256(_mm256_or_si256(a, b), _mm256_or_si256(c, d));

    const __m256i shuffle_bytes =
        _mm256_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15,
                         0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

    const __m256i t1 = _mm256_shuffle_epi8(t0, shuffle_bytes);

    const __m256i shuffle_dwords = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);
    const __m256i t2 = _mm256_permutevar8x32_epi32(t1, shuffle_dwords);

    _mm256_storeu_si256((__m256i *)latin1_output, t2);

    latin1_output += 32;
    buf += 32;
  }

  return std::make_pair(result(error_code::SUCCESS, buf - start),
                        latin1_output);
}
