/*convert_latin1_to_utf32+fallback, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  16.451 GB/s (9.7 %)   16.451 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+haswell, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  20.765 GB/s (15.1 %)   20.765 Gc/s     1.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_latin1_to_utf32+icelake, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
convert_latin1_to_utf32+icelake: unsupported by the system
convert_latin1_to_utf32+iconv, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.968 GB/s (8.8 %)    0.968 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+icu, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.494 GB/s (6.1 %)    0.494 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+westmere, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  17.448 GB/s (7.0 %)   17.448 Gc/s     1.00 byte/char   */

/* std::pair<const char*, char32_t*> avx2_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
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
} */


/* convert_latin1_to_utf32+fallback, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  16.513 GB/s (5.2 %)   16.513 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+haswell, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  21.069 GB/s (2.4 %)   21.069 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+icelake, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
convert_latin1_to_utf32+icelake: unsupported by the system
convert_latin1_to_utf32+iconv, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.953 GB/s (0.7 %)    0.953 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+icu, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.496 GB/s (6.0 %)    0.496 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+westmere, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  16.653 GB/s (8.6 %)   16.653 Gc/s     1.00 byte/char  */

/* std::pair<const char*, char32_t*> avx2_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    const char* end = buf + len;

    while (buf + 8 <= end) {  // Process 8 bytes at a time
        // Load 8 Latin1 characters into a 64-bit register
        __m128i in = _mm_loadl_epi64((__m128i*)buf);

        // Zero extend each set of 8 Latin1 characters to 8 32-bit integers using vpmovzxbd
        __m256i out = _mm256_cvtepu8_epi32(in);

        // Store the results back to memory
        _mm256_storeu_si256((__m256i*)utf32_output, out);

        utf32_output += 8;  // Updated 8 32-bit integers
        buf += 8;           // Processed 8 bytes
    }

    return std::make_pair(buf, utf32_output);
} */


/* convert_latin1_to_utf32+fallback, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  16.301 GB/s (5.5 %)   16.301 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+haswell, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  21.870 GB/s (7.1 %)   21.870 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+icelake, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
convert_latin1_to_utf32+icelake: unsupported by the system
convert_latin1_to_utf32+iconv, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.984 GB/s (3.4 %)    0.984 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+icu, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.492 GB/s (4.2 %)    0.492 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+westmere, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  17.677 GB/s (2.3 %)   17.677 Gc/s     1.00 byte/char  */

std::pair<const char*, char32_t*> avx2_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    size_t rounded_len = ((len | 7) ^ 7);  // Round down to nearest multiple of 8
    
    for (size_t i = 0; i < rounded_len; i += 8) { 
        // Load 8 Latin1 characters into a 64-bit register
        __m128i in = _mm_loadl_epi64((__m128i*)&buf[i]);
        
        // Zero extend each set of 8 Latin1 characters to 8 32-bit integers using vpmovzxbd
        __m256i out = _mm256_cvtepu8_epi32(in);
        
        // Store the results back to memory
        _mm256_storeu_si256((__m256i*)&utf32_output[i], out);
    }

    // return pointers pointing to where we left off
    return std::make_pair(buf + rounded_len, utf32_output + rounded_len);
}


/* std::pair<const char*, char32_t*> avx2_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    size_t rounded_len = ((len | 15) ^ 15);  // Round down to nearest multiple of 16

    for (size_t i = 0; i < rounded_len; i += 16) { 
        // Load 8 Latin1 characters into a 64-bit register (first half)
        __m128i in1 = _mm_loadl_epi64((__m128i*)&buf[i]);
        // Load 8 Latin1 characters into a 64-bit register (second half)
        __m128i in2 = _mm_loadl_epi64((__m128i*)&buf[i + 8]);

        // Zero extend each set of 8 Latin1 characters to 8 32-bit integers
        __m256i out1 = _mm256_cvtepu8_epi32(in1);
        __m256i out2 = _mm256_cvtepu8_epi32(in2);

        // Store the results back to memory
        _mm256_storeu_si256((__m256i*)&utf32_output[i], out1);
        _mm256_storeu_si256((__m256i*)&utf32_output[i + 8], out2);
    }

    // return pointers pointing to where we left off
    return std::make_pair(buf + rounded_len, utf32_output + rounded_len);
}
 */

/* convert_latin1_to_utf32+fallback, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  16.744 GB/s (14.1 %)   16.744 Gc/s     1.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
convert_latin1_to_utf32+haswell, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  20.666 GB/s (7.2 %)   20.666 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+icelake, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
convert_latin1_to_utf32+icelake: unsupported by the system
convert_latin1_to_utf32+iconv, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.968 GB/s (8.1 %)    0.968 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+icu, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.492 GB/s (7.4 %)    0.492 Gc/s     1.00 byte/char 
convert_latin1_to_utf32+westmere, input size: 432305, iterations: 30000, dataset: /unicode_lipsum/wikipedia_mars/french.latin1.txt
  17.691 GB/s (30.3 %)   17.691 Gc/s     1.00 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I). */

/* 
std::pair<const char*, char32_t*> avx2_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    size_t rounded_len = ((len | 31) ^ 31);  // Round down to nearest multiple of 32

    for (size_t i = 0; i < rounded_len; i += 32) { 
        // Load 8 Latin1 characters into 64-bit registers
        __m128i in1 = _mm_loadl_epi64((__m128i*)&buf[i]);
        __m128i in2 = _mm_loadl_epi64((__m128i*)&buf[i + 8]);
        __m128i in3 = _mm_loadl_epi64((__m128i*)&buf[i + 16]);
        __m128i in4 = _mm_loadl_epi64((__m128i*)&buf[i + 24]);

        // Zero extend each set of 8 Latin1 characters to 8 32-bit integers
        __m256i out1 = _mm256_cvtepu8_epi32(in1);
        __m256i out2 = _mm256_cvtepu8_epi32(in2);
        __m256i out3 = _mm256_cvtepu8_epi32(in3);
        __m256i out4 = _mm256_cvtepu8_epi32(in4);

        // Store the results back to memory
        _mm256_storeu_si256((__m256i*)&utf32_output[i], out1);
        _mm256_storeu_si256((__m256i*)&utf32_output[i + 8], out2);
        _mm256_storeu_si256((__m256i*)&utf32_output[i + 16], out3);
        _mm256_storeu_si256((__m256i*)&utf32_output[i + 24], out4);
    }

    // return pointers pointing to where we left off
    return std::make_pair(buf + rounded_len, utf32_output + rounded_len);
} */
