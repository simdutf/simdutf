std::pair<const char32_t *, char *>
sse_convert_utf32_to_latin1(const char32_t *buf, size_t len,
                            char *latin1_output) {
  const size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 16

  __m128i high_bytes_mask = _mm_set1_epi32(0xFFFFFF00);
  __m128i shufmask_1 =
      _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0);

  for (size_t i = 0; i < rounded_len; i += 16) {
    __m128i in1 = _mm_loadu_si128((__m128i *)buf);
    __m128i in2 = _mm_loadu_si128((__m128i *)(buf + 4));
    __m128i in3 = _mm_loadu_si128((__m128i *)(buf + 8));
    __m128i in4 = _mm_loadu_si128((__m128i *)(buf + 12));

    __m128i check_combined = _mm_or_si128(in1, in2);
    check_combined = _mm_or_si128(check_combined, in3);
    check_combined = _mm_or_si128(check_combined, in4);

    if (!_mm_testz_si128(check_combined, high_bytes_mask)) {
      return std::make_pair(nullptr, latin1_output);
    }

    __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask_1);
    _mm_storeu_si64(latin1_output, shuffled1);
    __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask_1);
    _mm_storeu_si64(latin1_output + 4, shuffled2);
    __m128i shuffled3 = _mm_shuffle_epi8(in3, shufmask_1);
    _mm_storeu_si64(latin1_output + 8, shuffled3);
    __m128i shuffled4 = _mm_shuffle_epi8(in4, shufmask_1);

    *reinterpret_cast<uint32_t *>(latin1_output + 12) =
        _mm_cvtsi128_si32(shuffled4);

    latin1_output += 16;
    buf += 16;
  }

  return std::make_pair(buf, latin1_output);
}

std::pair<result, char *>
sse_convert_utf32_to_latin1_with_errors(const char32_t *buf, size_t len,
                                        char *latin1_output) {
  const char32_t *start = buf;
  const size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 16

  __m128i high_bytes_mask = _mm_set1_epi32(0xFFFFFF00);
  __m128i shufmask =
      _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0);

  for (size_t i = 0; i < rounded_len; i += 16) {
    __m128i in1 = _mm_loadu_si128((__m128i *)buf);
    __m128i in2 = _mm_loadu_si128((__m128i *)(buf + 4));
    __m128i in3 = _mm_loadu_si128((__m128i *)(buf + 8));
    __m128i in4 = _mm_loadu_si128((__m128i *)(buf + 12));

    __m128i check_combined = _mm_or_si128(in1, in2);
    check_combined = _mm_or_si128(check_combined, in3);
    check_combined = _mm_or_si128(check_combined, in4);

    if (!_mm_testz_si128(check_combined, high_bytes_mask)) {
      // Fallback to scalar code for handling errors
      for (int k = 0; k < 16; k++) {
        char32_t codepoint = buf[k];
        if (codepoint <= 0xff) {
          *latin1_output++ = char(codepoint);
        } else {
          return std::make_pair(result(error_code::TOO_LARGE, buf - start + k),
                                latin1_output);
        }
      }
      buf += 16;
      continue;
    }

    __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask);
    _mm_storeu_si64(latin1_output, shuffled1);
    __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask);
    _mm_storeu_si64(latin1_output + 4, shuffled2);
    __m128i shuffled3 = _mm_shuffle_epi8(in3, shufmask);
    _mm_storeu_si64(latin1_output + 8, shuffled3);
    __m128i shuffled4 = _mm_shuffle_epi8(in4, shufmask);
    *reinterpret_cast<uint32_t *>(latin1_output + 12) =
        _mm_cvtsi128_si32(shuffled4);

    latin1_output += 16;
    buf += 16;
  }

  return std::make_pair(result(error_code::SUCCESS, buf - start),
                        latin1_output);
}
