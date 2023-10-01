std::pair<const char32_t*, char*> sse_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8

    __m128i high_bytes_mask = _mm_set1_epi32(0xFFFFFF00);
    __m128i shufmask = _mm_set_epi8(
      -1, -1, -1, -1,
       -1, -1, -1, -1,
       -1, -1, -1, -1,
       12, 8, 4, 0);

    for (size_t i=0; i < rounded_len; i += 8) {
        __m128i in1 = _mm_loadu_si128((__m128i *)buf);
        __m128i in2 = _mm_loadu_si128((__m128i *)(buf + 4));

        __m128i check_combined = _mm_or_si128(in1,in2);
        if (!_mm_testz_si128(check_combined, high_bytes_mask)) {
            return std::make_pair(nullptr, latin1_output);
        }

        __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask);
        __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask);

        __m128i combined = _mm_unpacklo_epi32(shuffled1, shuffled2);
        _mm_storeu_si128((__m128i*)latin1_output, combined);

        // Update pointers
        latin1_output += 8;
        buf += 8;
    }

    return std::make_pair(buf, latin1_output);
}

std::pair<result, char*> sse_convert_utf32_to_latin1_with_errors(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* start = buf;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8

    __m128i high_bytes_mask = _mm_set1_epi32(0xFFFFFF00);
    __m128i shufmask = _mm_set_epi8(
      -1, -1, -1, -1,
       -1, -1, -1, -1,
       -1, -1, -1, -1,
       12, 8, 4, 0);

    for (size_t i=0; i < rounded_len; i += 8) {
        __m128i in1 = _mm_loadu_si128((__m128i *)buf);
        __m128i in2 = _mm_loadu_si128((__m128i *)(buf + 4));

        __m128i check_combined = _mm_or_si128(in1,in2);
        if (!_mm_testz_si128(check_combined, high_bytes_mask)) {
            // Fallback to scalar code for handling errors
            for(int k = 0; k < 8; k++) {
                char32_t codepoint = buf[k];
                if(codepoint <= 0xff) {
                    *latin1_output++ = static_cast<char>(codepoint);
                } else {
                    return std::make_pair(result(error_code::TOO_LARGE, buf - start + k), latin1_output);
                }
            }
            buf += 8;
        } else {
            __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask);
            _mm_storel_epi64((__m128i*)latin1_output, shuffled1);
            __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask);
            _mm_storel_epi64((__m128i*)(latin1_output + 4), shuffled2);

            // Update pointers
            latin1_output += 8;
            buf += 8;
        }
    }

    return std::make_pair(result(error_code::SUCCESS, buf - start), latin1_output);
}
