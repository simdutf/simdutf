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



std::pair<const char32_t*, char*> avx2_convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) {
    const char32_t* end = buf + len;
    const size_t rounded_len = len & ~0x7;  // Round down to nearest multiple of 8

    
    __m256i v_0xFF = _mm256_set1_epi32(0xff);
    __m256i shufmask = _mm256_set_epi8(
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 24, 20, 16, 12, 8, 4, 0);


    while (buf + rounded_len < end) {
        __m256i in = _mm256_loadu_si256((__m256i *)buf);
        
        __m256i high_byte_mask = _mm256_set1_epi32(0xFFFFFF00);
        if (_mm256_testz_si256(in, high_byte_mask)) {
            return std::make_pair(nullptr, reinterpret_cast<char*>(latin1_output));
        }
        
        // Shuffle and store the packed Latin1 characters
        __m128i latin1_chars = _mm256_castsi256_si128(_mm256_shuffle_epi8(in, shufmask));
        _mm_storel_epi64((__m128i *)latin1_output, latin1_chars);
        
        latin1_output += 8;
        buf += 8;
    }
    
    // Return the pointer to the last processed character and the corresponding latin1_output position
    return std::make_pair(buf + rounded_len, latin1_output + rounded_len);
}
 

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