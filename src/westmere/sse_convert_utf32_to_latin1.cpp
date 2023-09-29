/* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.180 ins/byte,    0.127 cycle/byte,   25.153 GB/s (1.1 %),     3.197 GHz,    1.415 ins/cycle 
   0.719 ins/char,    0.508 cycle/char,    6.288 Gc/s (1.1 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.122 cycle/byte,   25.366 GB/s (0.9 %),     3.097 GHz,    1.409 ins/cycle 
   0.688 ins/char,    0.488 cycle/char,    6.341 Gc/s (0.9 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.759 cycle/byte,    1.815 GB/s (20.6 %),     3.193 GHz,    5.689 ins/cycle 
  40.023 ins/char,    7.035 cycle/char,    0.454 Gc/s (20.6 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (3.8 %),     3.189 GHz,    5.334 ins/cycle 
  78.748 ins/char,   14.764 cycle/char,    0.216 Gc/s (3.8 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.750 ins/byte,    0.193 cycle/byte,   16.559 GB/s (1.0 %),     3.195 GHz,    3.888 ins/cycle 
   3.001 ins/char,    0.772 cycle/char,    4.140 Gc/s (1.0 %)     4.00 byte/char  */
/* std::pair<const char32_t*, char*> sse_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x3;  // Round down to nearest multiple of 4

    __m128i v_0xFF = _mm_set1_epi32(0xff);
    __m128i shufmask = _mm_set_epi8(
      -1, -1, -1, -1,
       -1, -1, -1, -1,
       -1, -1, -1, -1,
       12, 8, 4, 0);

    // Ensure the loop condition is correct
    while (buf + 4 <= end) {
        __m128i in = _mm_loadu_si128((__m128i *)buf);

        // Treat the comparisons as unsigned using _mm_cmpgt_epu32
        if (!_mm_testz_si128(_mm_and_si128(in, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output);
        }

        __m128i shuffled = _mm_shuffle_epi8(in, shufmask);
        _mm_storel_epi64((__m128i*)latin1_output, shuffled);

        latin1_output += 4;
        buf += 4;
    }

    return std::make_pair(buf, latin1_output);
} */

/* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.180 ins/byte,    0.125 cycle/byte,   25.590 GB/s (0.8 %),     3.196 GHz,    1.440 ins/cycle 
   0.719 ins/char,    0.500 cycle/char,    6.397 Gc/s (0.8 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.120 cycle/byte,   25.885 GB/s (0.8 %),     3.097 GHz,    1.438 ins/cycle 
   0.688 ins/char,    0.479 cycle/char,    6.471 Gc/s (0.8 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.914 cycle/byte,    1.664 GB/s (15.8 %),     3.185 GHz,    5.228 ins/cycle 
  40.023 ins/char,    7.656 cycle/char,    0.416 Gc/s (15.8 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (3.9 %),     3.189 GHz,    5.334 ins/cycle 
  78.748 ins/char,   14.764 cycle/char,    0.216 Gc/s (3.9 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.594 ins/byte,    0.152 cycle/byte,   20.963 GB/s (7.6 %),     3.196 GHz,    3.896 ins/cycle 
   2.376 ins/char,    0.610 cycle/char,    5.241 Gc/s (7.6 %)     4.00 byte/char  */
/* std::pair<const char32_t*, char*> sse_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8

    __m128i v_0xFF = _mm_set1_epi32(0xff);
    __m128i shufmask = _mm_set_epi8(
      -1, -1, -1, -1,
       -1, -1, -1, -1,
       -1, -1, -1, -1,
       12, 8, 4, 0);

    // Ensure the loop condition is correct
    while (buf + 8 <= end) {
        // Unrolled Iteration 1
        __m128i in1 = _mm_loadu_si128((__m128i *)buf);

        if (!_mm_testz_si128(_mm_and_si128(in1, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output);
        }

        __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask);
        _mm_storel_epi64((__m128i*)latin1_output, shuffled1);

        // Unrolled Iteration 2
        __m128i in2 = _mm_loadu_si128((__m128i *)(buf + 4));

        if (!_mm_testz_si128(_mm_and_si128(in2, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output + 4);
        }

        __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask);
        _mm_storel_epi64((__m128i*)(latin1_output + 4), shuffled2);

        // Update pointers
        latin1_output += 8;
        buf += 8;
    }

    return std::make_pair(buf, latin1_output);
} */


/* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.180 ins/byte,    0.126 cycle/byte,   25.385 GB/s (0.8 %),     3.197 GHz,    1.428 ins/cycle 
   0.719 ins/char,    0.504 cycle/char,    6.346 Gc/s (0.8 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.121 cycle/byte,   25.677 GB/s (0.8 %),     3.099 GHz,    1.425 ins/cycle 
   0.688 ins/char,    0.483 cycle/char,    6.419 Gc/s (0.8 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.797 cycle/byte,    1.774 GB/s (23.3 %),     3.187 GHz,    5.569 ins/cycle 
  40.023 ins/char,    7.187 cycle/char,    0.443 Gc/s (23.3 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (4.2 %),     3.189 GHz,    5.334 ins/cycle 
  78.749 ins/char,   14.764 cycle/char,    0.216 Gc/s (4.2 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.594 ins/byte,    0.154 cycle/byte,   20.758 GB/s (5.7 %),     3.196 GHz,    3.857 ins/cycle 
   2.376 ins/char,    0.616 cycle/char,    5.189 Gc/s (5.7 %)     4.00 byte/char  */
std::pair<const char32_t*, char*> sse_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8

    __m128i shufmask = _mm_set_epi8(
      -1, -1, -1, -1,
       -1, -1, -1, -1,
       -1, -1, -1, -1,
       12, 8, 4, 0);

    // Ensure the loop condition is correct
    while (buf + 8 <= end) {
        // Unrolled Iteration 1
        __m128i in1 = _mm_loadu_si128((__m128i *)buf);

        if (!_mm_testz_si128(_mm_and_si128(in1, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output);
        }

        __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask);

        // Unrolled Iteration 2
        __m128i in2 = _mm_loadu_si128((__m128i *)(buf + 4));

        if (!_mm_testz_si128(_mm_and_si128(in2, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output + 4);
        }

        __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask);

        // Combine shuffled1 and shuffled2 and store the result
        __m128i combined = _mm_unpacklo_epi32(shuffled1, shuffled2);
        _mm_storeu_si128((__m128i*)latin1_output, combined);
        // _mm_storel_epi64((__m128i*)latin1_output, combined);


        // Update pointers
        latin1_output += 8;
        buf += 8;
    }

    return std::make_pair(buf, latin1_output);
}

/* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.180 ins/byte,    0.126 cycle/byte,   25.326 GB/s (0.9 %),     3.197 GHz,    1.425 ins/cycle 
   0.719 ins/char,    0.505 cycle/char,    6.332 Gc/s (0.9 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.121 cycle/byte,   25.569 GB/s (0.9 %),     3.097 GHz,    1.420 ins/cycle 
   0.688 ins/char,    0.484 cycle/char,    6.392 Gc/s (0.9 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.776 cycle/byte,    1.794 GB/s (19.1 %),     3.186 GHz,    5.634 ins/cycle 
  40.023 ins/char,    7.104 cycle/char,    0.449 Gc/s (19.1 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (3.3 %),     3.189 GHz,    5.334 ins/cycle 
  78.748 ins/char,   14.764 cycle/char,    0.216 Gc/s (3.3 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.594 ins/byte,    0.154 cycle/byte,   20.804 GB/s (5.6 %),     3.196 GHz,    3.866 ins/cycle 
   2.376 ins/char,    0.614 cycle/char,    5.201 Gc/s (5.6 %)     4.00 byte/char  */
std::pair<const char32_t*, char*> sse_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8

    __m128i shufmask = _mm_set_epi8(
      -1, -1, -1, -1,
       -1, -1, -1, -1,
       -1, -1, -1, -1,
       12, 8, 4, 0);

    // Ensure the loop condition is correct
    while (buf + 8 <= end) {
        // Unrolled Iteration 1
        __m128i in1 = _mm_loadu_si128((__m128i *)buf);

        if (!_mm_testz_si128(_mm_and_si128(in1, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output);
        }

        __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask);

        // Unrolled Iteration 2
        __m128i in2 = _mm_loadu_si128((__m128i *)(buf + 4));

        if (!_mm_testz_si128(_mm_and_si128(in2, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output + 4);
        }

        __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask);

        // Combine shuffled1 and shuffled2 and store the result
        __m128i combined = _mm_unpacklo_epi32(shuffled1, shuffled2);
        _mm_storeu_si128((__m128i*)latin1_output, combined);
        // _mm_storel_epi64((__m128i*)latin1_output, combined);


        // Update pointers
        latin1_output += 8;
        buf += 8;
    }

    return std::make_pair(buf, latin1_output);
}


/* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.180 ins/byte,    0.126 cycle/byte,   25.338 GB/s (0.9 %),     3.197 GHz,    1.426 ins/cycle 
   0.719 ins/char,    0.505 cycle/char,    6.335 Gc/s (0.9 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.121 cycle/byte,   25.633 GB/s (0.9 %),     3.096 GHz,    1.424 ins/cycle 
   0.688 ins/char,    0.483 cycle/char,    6.408 Gc/s (0.9 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.783 cycle/byte,    1.787 GB/s (21.7 %),     3.187 GHz,    5.611 ins/cycle 
  40.023 ins/char,    7.133 cycle/char,    0.447 Gc/s (21.7 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (4.0 %),     3.189 GHz,    5.334 ins/cycle 
  78.749 ins/char,   14.764 cycle/char,    0.216 Gc/s (4.0 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.516 ins/byte,    0.153 cycle/byte,   20.927 GB/s (1.4 %),     3.196 GHz,    3.377 ins/cycle 
   2.063 ins/char,    0.611 cycle/char,    5.232 Gc/s (1.4 %)     4.00 byte/char  */
/* std::pair<const char32_t*, char*> sse_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0xF;  // Round down to nearest multiple of 16

    __m128i shufmask = _mm_set_epi8(
      -1, -1, -1, -1,
       -1, -1, -1, -1,
       -1, -1, -1, -1,
       12, 8, 4, 0);

    // Process 16 elements at a time
    while (buf + 16 <= end) {
        // Unrolled Iteration 1
        __m128i in1 = _mm_loadu_si128((__m128i *)buf);

        if (!_mm_testz_si128(_mm_and_si128(in1, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output);
        }

        __m128i shuffled1 = _mm_shuffle_epi8(in1, shufmask);
        _mm_storel_epi64((__m128i*)latin1_output, shuffled1);

        // Unrolled Iteration 2
        __m128i in2 = _mm_loadu_si128((__m128i *)(buf + 4));

        if (!_mm_testz_si128(_mm_and_si128(in2, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output + 4);
        }

        __m128i shuffled2 = _mm_shuffle_epi8(in2, shufmask);
        _mm_storel_epi64((__m128i*)(latin1_output + 4), shuffled2);

        // Unrolled Iteration 3
        __m128i in3 = _mm_loadu_si128((__m128i *)(buf + 8));

        if (!_mm_testz_si128(_mm_and_si128(in3, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output + 8);
        }

        __m128i shuffled3 = _mm_shuffle_epi8(in3, shufmask);
        _mm_storel_epi64((__m128i*)(latin1_output + 8), shuffled3);

        // Unrolled Iteration 4
        __m128i in4 = _mm_loadu_si128((__m128i *)(buf + 12));

        if (!_mm_testz_si128(_mm_and_si128(in4, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output + 12);
        }

        __m128i shuffled4 = _mm_shuffle_epi8(in4, shufmask);
        _mm_storel_epi64((__m128i*)(latin1_output + 12), shuffled4);

        // Update pointers
        latin1_output += 16;
        buf += 16;
    }

    return std::make_pair(buf, latin1_output);
} */


std::pair<result, char*> sse_convert_utf32_to_latin1_with_errors(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* start = buf;
    const char32_t* end = buf + len;
    
    __m128i v_0xFF = _mm_set1_epi32(0xff);
    __m128i shufmask = _mm_set_epi8(
      -1, -1, -1, -1,
      -1, -1, -1, -1,
      -1, -1, -1, -1,
      12, 8, 4, 0);
    
    while (buf + 4 <= end) {
        __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buf));
        
        if (!_mm_testz_si128(_mm_and_si128(in, _mm_set1_epi32(0xFFFFFF00)), _mm_set1_epi32(0xFFFFFF00))) {
            // Fallback to scalar code for handling errors
            for(int k = 0; k < 4; k++) {
                char32_t codepoint = buf[k];
                if(codepoint <= 0xff) {
                    *latin1_output++ = static_cast<char>(codepoint);
                } else {
                    return std::make_pair(result(TOO_LARGE, buf - start + k), latin1_output);
                }
            }
            buf += 4;
        } else {
            __m128i shuffled = _mm_shuffle_epi8(in, shufmask);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(latin1_output), shuffled);
            latin1_output += 4;
            buf += 4;
        }
    }

    return std::make_pair(result(SUCCESS, buf - start), latin1_output);
}