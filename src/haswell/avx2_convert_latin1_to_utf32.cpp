
std::pair<const char*, char32_t*> avx2_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    const char* end = buf + len;

    while (buf + 32 <= end) {  // Process 32 bytes at a time
        // Load 8 Latin1 characters at a time into 128-bit registers
        __m128i in1 = _mm_loadu_si128((__m128i*)buf);
        __m128i in2 = _mm_loadu_si128((__m128i*)(buf + 8));
        __m128i in3 = _mm_loadu_si128((__m128i*)(buf + 16));
        __m128i in4 = _mm_loadu_si128((__m128i*)(buf + 24));

        // Zero extend each set of 8 Latin1 characters to 8 32-bit integers
        __m256i out1 = _mm256_cvtepu8_epi32(in1);
        __m256i out2 = _mm256_cvtepu8_epi32(in2);
        __m256i out3 = _mm256_cvtepu8_epi32(in3);
        __m256i out4 = _mm256_cvtepu8_epi32(in4);

        // Store the results back to memory
        _mm256_storeu_si256((__m256i*)(utf32_output), out1);
        _mm256_storeu_si256((__m256i*)(utf32_output + 8), out2);
        _mm256_storeu_si256((__m256i*)(utf32_output + 16), out3);
        _mm256_storeu_si256((__m256i*)(utf32_output + 24), out4);

        utf32_output += 32;  // Updated 32 32-bit integers
        buf += 32;           // Processed 32 bytes
    }

    return std::make_pair(buf, utf32_output);
}