/* std::pair<const char16_t*, char*> avx2_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    __m256i v_0xFF = _mm256_set1_epi32(0xff);
    
    while (buf + 8 <= end) {
        __m256i in = _mm256_loadu_si256((__m256i*)buf);
        
        if (_mm256_movemask_epi8(_mm256_cmpgt_epi32(in, v_0xFF))) {
                      return std::make_pair(result(error_code::TOO_LARGE, buf - start + k), latin1_output);

        }
        
        // Shuffle and pack 32-bit integers to 8-bit integers, handling 8 characters
        __m128i packed = _mm_packus_epi32(_mm256_castsi256_si128(in), _mm256_extractf128_si256(in, 1));
        _mm_storel_epi64((__m128i*)latin1_output, packed);
        
        latin1_output += 8;
        buf += 8;
    }

    // return len;
    return std::make_pair(buf + rounded_len, latin1_output + rounded_len);

} */
/* 
std::pair<const char32_t*, char*> avx2_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8
    
    while (buf + rounded_len <= end) {
        __m256i in = _mm256_loadu_si256((__m256i*)buf);
        
        __m256i mask = _mm256_set1_epi32(0xFFFFFF00); // 32-bit mask where only the lowest byte is zero.
        __m256i cmp = _mm256_and_si256(in, mask);
        if (!_mm256_testz_si256(cmp, cmp)) { // Test if any bit is set in the result.
            return std::make_pair(nullptr, reinterpret_cast<char*>(latin1_output));
        }
        
        // Shuffle and pack 32-bit integers to 8-bit integers, handling 8 characters
        __m128i packed = _mm_packus_epi32(_mm256_castsi256_si128(in), _mm256_extractf128_si256(in, 1));
        _mm_storel_epi64((__m128i*)latin1_output, packed);
        
        latin1_output += 8;
        buf += 8;
    }

    return std::make_pair(buf + rounded_len, latin1_output + rounded_len);
} */



/* std::pair<const char32_t*, char*> avx2_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8
    const char32_t* rounded_end = buf + rounded_len;

    while (buf < rounded_end) {
        __m256i in = _mm256_loadu_si256((__m256i*)buf);
        
        // Directly check if any UTF-32 character > 0xFF
        __m256i maxLatin1 = _mm256_set1_epi32(0xFF);
        __m256i result = _mm256_cmpgt_epi32(in, maxLatin1);
        if (!_mm256_testz_si256(result, result)) {
            // return std::make_pair(nullptr, latin1_output);
            return std::make_pair(nullptr, reinterpret_cast<char*>(latin1_output));
        }

        // Shuffle and pack 32-bit integers to 8-bit integers, handling 8 characters
        __m128i packed = _mm_packus_epi16(_mm256_castsi256_si128(in), _mm256_extractf128_si256(in, 1));
        _mm_storel_epi64((__m128i*)latin1_output, packed);
        
        latin1_output += 16;
        buf += 16;
    }


    return std::make_pair(buf + rounded_len, latin1_output + rounded_len);
}  */



/* std::pair<const char32_t*, char*> avx2_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8


    
    __m256i v_0xFF = _mm256_set1_epi32(0xff);
    __m256i shufmask = _mm256_set_epi8(
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 24, 20, 16, 12, 8, 4, 0);


    while (buf + rounded_len <= end) {
        __m256i in = _mm256_loadu_si256((__m256i *)buf);
        
        __m256i high_byte_mask = _mm256_set1_epi32(0xFFFFFF00);
        if (!_mm256_testz_si256(in, high_byte_mask)) {
            return std::make_pair(nullptr, (latin1_output));
        }

        // Shuffle and store the packed Latin1 characters
        __m128i latin1_chars = _mm256_castsi256_si128(_mm256_shuffle_epi8(in, shufmask));
        // _mm_storel_epi64((__m128i *)latin1_output, latin1_chars);
        // _mm_storel_epi64(reinterpret_cast<__m128i*>(latin1_output), latin1_chars);
        _mm_storeu_si64(reinterpret_cast<void*>(latin1_output), latin1_chars);
        
        latin1_output += 8;
        buf += 8;
    }
    
    // Return the pointer to the last processed character and the corresponding latin1_output position
    // return std::make_pair(buf + rounded_len, latin1_output + rounded_len);
    return std::make_pair(buf, latin1_output);
}
  */
/* 
std::pair<const char32_t*, char*> avx2_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8

    __m256i v_0xFF = _mm256_set1_epi32(0xff);
    __m256i shufmask = _mm256_set_epi8(
      -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, 28, 24, 20, 16,
        -1, -1, -1, -1, -1, -1, -1, -1, 
       -1, -1, -1, -1,12, 8, 4, 0);

    // Ensure the loop condition is correct
    while (buf + 8 <= end) {
        __m256i in = _mm256_loadu_si256((__m256i *)buf);

        // Treat the comparisons as unsigned using _mm256_cmpgt_epu32
        if (!_mm256_testz_si256(_mm256_and_si256(in, _mm256_set1_epi32(0xFFFFFF00)), _mm256_set1_epi32(0xFFFFFF00))) {
            return std::make_pair(nullptr, latin1_output);
        }

        __m256i shuffled = _mm256_shuffle_epi8(in, shufmask);
        __m128i latin1_chars = _mm256_castsi256_si128(shuffled);

        _mm_storel_epi64((__m128i*)latin1_output, latin1_chars);

        // __m256i packed_16bit = _mm256_packus_epi32(in, in); // Pack 32-bit integers to 16-bit integers

latin1_output += 8;
buf += 8;

    }

    return std::make_pair(buf, latin1_output);
} */


/*  std::pair<const char32_t*, char*> avx2_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    
    // Shuffle mask to select only the lowest byte from each 32-bit integer.
    __m256i shufmask = _mm256_set_epi8(
        -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1,
        28, 24, 20, 16, 12, 8, 4, 0);
    
    while (buf + 8 <= end) { // Process 8 characters at a time
        __m256i in = _mm256_loadu_si256((__m256i *)buf);

        __m256i high_byte_mask = _mm256_set1_epi32(0xFFFFFF00);
        if (_mm256_testz_si256(in, high_byte_mask)) {
            return std::make_pair(nullptr, reinterpret_cast<char*>(latin1_output));
        }
        
        // Shuffle and store the truncated Latin1 characters.
        __m128i latin1_chars = _mm256_castsi256_si128(_mm256_shuffle_epi8(in, shufmask));
        _mm_storel_epi64((__m128i *)latin1_output, latin1_chars);
        
        latin1_output += 8;
        buf += 8;
    }
    
    // Return the pointer to the last processed character and the corresponding latin1_output position
    return std::make_pair(buf, latin1_output);
}
 */

// incorrect
/* convert_utf32_to_latin1+haswell, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.203 ins/byte,    0.121 cycle/byte,   26.366 GB/s (0.8 %),     3.197 GHz,    1.676 ins/cycle 
   0.813 ins/char,    0.485 cycle/char,    6.592 Gc/s (0.8 %)     4.00 byte/char 
convert_utf32_to_latin1+icelake, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.172 ins/byte,    0.121 cycle/byte,   25.660 GB/s (0.8 %),     3.098 GHz,    1.425 ins/cycle 
   0.688 ins/char,    0.483 cycle/char,    6.415 Gc/s (0.8 %)     4.00 byte/char 
convert_utf32_to_latin1+iconv, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  10.006 ins/byte,    1.800 cycle/byte,    1.770 GB/s (17.5 %),     3.187 GHz,    5.557 ins/cycle 
  40.023 ins/char,    7.202 cycle/char,    0.443 Gc/s (17.5 %)     4.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_utf32_to_latin1+icu, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
  19.687 ins/byte,    3.691 cycle/byte,    0.864 GB/s (4.2 %),     3.189 GHz,    5.334 ins/cycle 
  78.748 ins/char,   14.764 cycle/char,    0.216 Gc/s (4.2 %)     4.00 byte/char 
convert_utf32_to_latin1+westmere, input size: 1729220, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin32.txt
   0.406 ins/byte,    0.129 cycle/byte,   24.797 GB/s (1.2 %),     3.197 GHz,    3.152 ins/cycle 
   1.626 ins/char,    0.516 cycle/char,    6.199 Gc/s (1.2 %)     4.00 byte/char  */
std::pair<const char32_t*, char*> avx2_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0xF;  // Round down to nearest multiple of 16 for AVX2

    __m256i high_bytes_mask = _mm256_set1_epi32(0xFFFFFF00);
    __m256i shufmask = _mm256_set_epi8(
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0);

    for (int i=0; i < rounded_len; i += 16) {
        __m256i in1 = _mm256_loadu_si256((__m256i *)buf);
        __m256i in2 = _mm256_loadu_si256((__m256i *)(buf + 8));

        __m256i check_combined = _mm256_or_si256(in1,in2);
        if (!_mm256_testz_si256(check_combined, high_bytes_mask)) {
            return std::make_pair(nullptr, latin1_output);
        }

        __m256i shuffled1 = _mm256_shuffle_epi8(in1, shufmask);
        // _mm256_alignr_epi8
        _mm_storeu_si128((__m128i*)latin1_output, _mm256_castsi256_si128(shuffled1));
        latin1_output += 8;

        __m256i shuffled2 = _mm256_shuffle_epi8(in2, shufmask);
        _mm_storeu_si128((__m128i*)latin1_output, _mm256_castsi256_si128(shuffled2));
        latin1_output += 8;

        buf += 16;
    }

    return std::make_pair(buf, latin1_output);
}
