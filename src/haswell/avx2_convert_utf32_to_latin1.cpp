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
}
