//working on big1, doesn't pass all tests
/* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.266 ins/byte,    0.136 cycle/byte,   23.514 GB/s (1.2 %),     3.197 GHz,    1.955 ins/cycle 
   1.063 ins/char,    0.544 cycle/char,    5.879 Gc/s (1.2 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.121 cycle/byte,   25.586 GB/s (0.9 %),     3.097 GHz,    1.421 ins/cycle 
   0.688 ins/char,    0.484 cycle/char,    6.397 Gc/s (0.9 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.771 cycle/byte,    1.800 GB/s (20.3 %),     3.187 GHz,    5.649 ins/cycle 
  40.023 ins/char,    7.084 cycle/char,    0.450 Gc/s (20.3 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (3.6 %),     3.189 GHz,    5.334 ins/cycle 
  78.749 ins/char,   14.763 cycle/char,    0.216 Gc/s (3.6 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.406 ins/byte,    0.129 cycle/byte,   24.818 GB/s (1.4 %),     3.196 GHz,    3.156 ins/cycle 
   1.626 ins/char,    0.515 cycle/char,    6.204 Gc/s (1.4 %)     4.00 byte/char 
convert_utf32_to_latin1_with_errors+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   2.000 ins/byte,    0.376 cycle/byte,    8.493 GB/s (0.6 %),     3.194 GHz,    5.319 ins/cycle 
   8.001 ins/char,    1.504 cycle/char,    2.123 Gc/s (0.6 %)     4.00 byte/char 
convert_utf32_to_latin1_with_errors+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.188 ins/byte,    0.121 cycle/byte,   25.496 GB/s (1.1 %),     3.097 GHz,    1.545 ins/cycle 
   0.751 ins/char,    0.486 cycle/char,    6.374 Gc/s (1.1 %)     4.00 byte/char 
convert_utf32_to_latin1_with_errors+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   2.000 ins/byte,    0.376 cycle/byte,    8.494 GB/s (0.5 %),     3.194 GHz,    5.319 ins/cycle 
   8.001 ins/char,    1.504 cycle/char,    2.124 Gc/s (0.5 %)     4.00 byte/char  */
/* std::pair<const char32_t *, char *>
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
} */

std::pair<const char32_t *, char *>
avx2_convert_utf32_to_latin1(const char32_t *buf, size_t len,
                             char *latin1_output) {
  const size_t rounded_len =
      len & ~0x1F; // Round down to nearest multiple of 32

  __m256i high_bytes_mask = _mm256_set1_epi32(0xFFFFFF00);
/*   __m256i shufmask = _mm256_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                     -1, 12, 8, 4, 0, 12, 8, 4, 0,-1, -1, -1, -1, -1, -1,
                                     -1, -1, -1, -1, -1, -1 );
 */
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

/*     __m256i shuffled1 = _mm256_shuffle_epi8(in1, shufmask);
    _mm_storeu_si128((__m128i *)latin1_output,
                     _mm256_castsi256_si128(shuffled1));
    _mm_storeu_si128((__m128i *)(latin1_output + 4),
                     _mm256_extracti128_si256(shuffled1, 1));

    __m256i shuffled2 = _mm256_shuffle_epi8(in2, shufmask);
    _mm_storeu_si128((__m128i *)(latin1_output + 8),
                     _mm256_castsi256_si128(shuffled2));
    *reinterpret_cast<uint32_t *>(latin1_output + 12) =
        _mm_cvtsi128_si32(_mm256_extracti128_si256(shuffled2, 1)); */

    //Turn UTF32 bytes into latin 1 bytes
    __m256i shuffled1 = _mm256_shuffle_epi8(in1, shufmask);
    __m256i shuffled2 = _mm256_shuffle_epi8(in2, shufmask);

    //reshuffle UTF32 bytes to their correct spot
    __m256i idx1 = _mm256_set_epi32(0, 4,0,0,0,0,0,0); //this is likely wrong
    __m256i idx2 = _mm256_set_epi32(0, 0,0,4,0,0,0,0);

    __m256i reshuffled1 = _mm256_permutevar8x32_epi32(shuffled1, idx1);
    __m256i reshuffled2 = _mm256_permutevar8x32_epi32(shuffled2, idx2);

    __m256i result = _mm256_or_si256(reshuffled2, reshuffled2);
    _mm_storeu_si128((__m128i *)latin1_output,
                     _mm256_castsi256_si128(result));

/*     _mm256_storeu_si256((__m256i*)latin1_output, interleave1);
    _mm256_storeu_si256((__m256i*)(latin1_output + 16), interleave2); */

    latin1_output += 16;
    buf += 16;
  }

  return std::make_pair(buf, latin1_output);
}

std::pair<result, char *>
avx2_convert_utf32_to_latin1_with_errors(const char32_t *buf, size_t len,
                                         char *latin1_output) {
  const size_t rounded_len =
      len & ~0x1F; // Round down to nearest multiple of 32

  __m256i high_bytes_mask = _mm256_set1_epi32(0xFFFFFF00);
  __m256i shufmask = _mm256_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                     -1, 12, 8, 4, 0, -1, -1, -1, -1, -1, -1,
                                     -1, -1, -1, -1, -1, -1, 12, 8, 4, 0);

  const char32_t *start = buf;

  for (size_t i = 0; i < rounded_len; i += 16) {
    __m256i in1 = _mm256_loadu_si256((__m256i *)buf);
    __m256i in2 = _mm256_loadu_si256((__m256i *)(buf + 8));

    __m256i check_combined = _mm256_or_si256(in1, in2);

    if (!_mm256_testz_si256(check_combined, high_bytes_mask)) {
      // Fallback to scalar code for handling errors
      for (int k = 0; k < 8; k++) {
        char32_t codepoint = buf[k];
        if (codepoint <= 0xFF) {
          *latin1_output++ = static_cast<char>(codepoint);
        } else {
          return std::make_pair(result(error_code::TOO_LARGE, buf - start + k),
                                latin1_output);
        }
      }
      buf += 8;
    } else {
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
  }

  return std::make_pair(result(error_code::SUCCESS, buf - start),
                        latin1_output);
}