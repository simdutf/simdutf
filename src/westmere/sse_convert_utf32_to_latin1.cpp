        /* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.180 ins/byte,    0.130 cycle/byte,   24.677 GB/s (0.9 %),     3.197 GHz,    1.388 ins/cycle 
   0.719 ins/char,    0.518 cycle/char,    6.169 Gc/s (0.9 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.124 cycle/byte,   24.908 GB/s (1.0 %),     3.098 GHz,    1.383 ins/cycle 
   0.688 ins/char,    0.498 cycle/char,    6.227 Gc/s (1.0 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.764 cycle/byte,    1.810 GB/s (21.4 %),     3.193 GHz,    5.673 ins/cycle 
  40.023 ins/char,    7.054 cycle/char,    0.453 Gc/s (21.4 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (3.7 %),     3.189 GHz,    5.334 ins/cycle 
  78.748 ins/char,   14.763 cycle/char,    0.216 Gc/s (3.7 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.469 ins/byte,    0.140 cycle/byte,   22.869 GB/s (0.9 %),     3.196 GHz,    3.355 ins/cycle 
   1.876 ins/char,    0.559 cycle/char,    5.717 Gc/s (0.9 %)     4.00 byte/char  */

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
        // _mm_storel_epi64((__m128i*)latin1_output, shuffled1);
        _mm_storeu_si64(latin1_output,shuffled1);
        __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask);
        // _mm_storel_epi64((__m128i*)(latin1_output +4), shuffled2);
        // _mm_storeu_si64(latin1_output + 4,shuffled2);
        *reinterpret_cast<uint32_t*>(latin1_output + 4) = _mm_cvtsi128_si32(shuffled2);

        
        latin1_output += 8;
        buf += 8;
    }

    return std::make_pair(buf, latin1_output);
}

/* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.180 ins/byte,    0.129 cycle/byte,   24.779 GB/s (0.9 %),     3.197 GHz,    1.394 ins/cycle 
   0.719 ins/char,    0.516 cycle/char,    6.195 Gc/s (0.9 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.124 cycle/byte,   25.043 GB/s (0.8 %),     3.097 GHz,    1.391 ins/cycle 
   0.688 ins/char,    0.495 cycle/char,    6.261 Gc/s (0.8 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.779 cycle/byte,    1.795 GB/s (18.6 %),     3.193 GHz,    5.625 ins/cycle 
  40.023 ins/char,    7.115 cycle/char,    0.449 Gc/s (18.6 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (3.7 %),     3.189 GHz,    5.334 ins/cycle 
  78.748 ins/char,   14.764 cycle/char,    0.216 Gc/s (3.7 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.469 ins/byte,    0.141 cycle/byte,   22.725 GB/s (1.6 %),     3.196 GHz,    3.334 ins/cycle 
   1.876 ins/char,    0.562 cycle/char,    5.681 Gc/s (1.6 %)     4.00 byte/char  */
/* std::pair<const char32_t*, char*> sse_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8

    __m128i high_bytes_mask = _mm_set1_epi32(0xFFFFFF00);

    __m128i shufmask_2 = _mm_set_epi8(
      -1, -1, -1, -1,
       -1, -1, -1, -1,
       12, 8, 4, 0,
       -1, -1, -1, -1);

    __m128i shufmask_1 = _mm_set_epi8(
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

        __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask_1);
        // _mm_storel_epi64((__m128i*)latin1_output, shuffled1);

        __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask_2);
        __m128i result = _mm_or_si128(shuffled1, shuffled2);

        _mm_storel_epi64((__m128i*)(latin1_output), result);

 */
        // _mm_storel_epi64((__m128i*)(latin1_output +4), shuffled2);
        // shuffled1 = _mm_srli_si128(shuffled1, 4);

        // uint32_t* output_ptr = reinterpret_cast<uint32_t*>(latin1_output);
        // *output_ptr = _mm_cvtsi128_si32(shuffled1);
        /* uint32_t* output_ptr = reinterpret_cast<uint32_t*>(latin1_output + 4);
        *output_ptr = _mm_cvtsi128_si32(shuffled2); */


       /*  uint32_t* output_ptr = reinterpret_cast<uint32_t*>(latin1_output);
        *output_ptr = _mm_cvtsi128_si32(shuffled1);
        output_ptr = reinterpret_cast<uint32_t*>(latin1_output + 4);
        *output_ptr = _mm_cvtsi128_si32(shuffled2);
 */
/*         latin1_output += 8;
        buf += 8;
    } */

/*     return std::make_pair(buf, latin1_output);
} */

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
