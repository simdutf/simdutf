/* Par/scalar benchmark:
convert_latin1_to_utf32+haswell, input size: 432305, iterations: 10000, dataset: unicode_lipsum/wikipedia_mars/french.latin1.txt
0.563 ins/byte, 0.584 cycle/byte, 5.474 GB/s (1.6 %), 3.196 GHz, 0.964 ins/cycle
0.563 ins/char, 0.584 cycle/char, 5.474 Gc/s (1.6 %) 1.00 byte/char */

/* convert_latin1_to_utf32+westmere, input size: 432305, iterations: 10000, dataset: unicode_lipsum/wikipedia_mars/french.latin1.txt
1.126 ins/byte, 0.528 cycle/byte, 6.052 GB/s (1.0 %), 3.196 GHz, 2.131 ins/cycle
1.126 ins/char, 0.528 cycle/char, 6.052 Gc/s (1.0 %) 1.00 byte/char */
/* std::pair<const char*, char32_t*> sse_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    const char* end = buf + len;

    while (buf + 16 <= end) {
        // Load 16 Latin1 characters (16 bytes) into a 128-bit register
        __m128i in = _mm_load_si128((__m128i*)buf);

        // Convert first 4 bytes to 4 32-bit integers
        __m128i out1 = _mm_cvtepu8_epi32(in);

        // Shift input to process next 4 bytes
        __m128i in_shifted1 = _mm_srli_si128(in, 4);
        __m128i out2 = _mm_cvtepu8_epi32(in_shifted1);

        // Shift input to process next set of 4 bytes
        __m128i in_shifted2 = _mm_srli_si128(in, 8);
        __m128i out3 = _mm_cvtepu8_epi32(in_shifted2);

        // Convert last 4 bytes to 4 32-bit integers
        __m128i in_shifted3 = _mm_srli_si128(in, 12);
        __m128i out4 = _mm_cvtepu8_epi32(in_shifted3);

        _mm_store_si128((__m128i*)utf32_output, out1);
        _mm_store_si128((__m128i*)(utf32_output + 4), out2);
        _mm_store_si128((__m128i*)(utf32_output + 8), out3);
        _mm_store_si128((__m128i*)(utf32_output + 12), out4);

        utf32_output += 16;
        buf += 16;
    }

    return std::make_pair(buf, utf32_output);
} */


/* convert_latin1_to_utf32+westmere, input size: 432305, iterations: 20000, dataset: unicode_lipsum/wikipedia_mars/french.latin1.txt
   1.188 ins/byte,    0.500 cycle/byte,    6.400 GB/s (0.9 %),     3.197 GHz,    2.379 ins/cycle 
   1.188 ins/char,    0.500 cycle/char,    6.400 Gc/s (0.9 %)     1.00 byte/char  */
std::pair<const char*, char32_t*> sse_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    const char* end = buf + len;

    while (buf + 16 <= end) {
        // Load 16 Latin1 characters (16 bytes) into a 128-bit register
        __m128i in = _mm_load_si128((__m128i*)buf);

        // Shift input to process next 4 bytes
        __m128i in_shifted1 = _mm_srli_si128(in, 4);
        __m128i in_shifted2 = _mm_srli_si128(in, 8);
        __m128i in_shifted3 = _mm_srli_si128(in, 12);

        // Shift input to process next set of 4 bytes        
        __m128i out1 = _mm_cvtepu8_epi32(in);
        __m128i out2 = _mm_cvtepu8_epi32(in_shifted1);
        __m128i out3 = _mm_cvtepu8_epi32(in_shifted2);
        __m128i out4 = _mm_cvtepu8_epi32(in_shifted3);

        _mm_store_si128((__m128i*)utf32_output, out1);
        _mm_store_si128((__m128i*)(utf32_output + 4), out2);
        _mm_store_si128((__m128i*)(utf32_output + 8), out3);
        _mm_store_si128((__m128i*)(utf32_output + 12), out4);

        utf32_output += 16;
        buf += 16;
    }

    return std::make_pair(buf, utf32_output);
}


/* 
This fares worse:
convert_latin1_to_utf32+westmere, input size: 432305, iterations: 10000, dataset: unicode_lipsum/wikipedia_mars/french.latin1.txt
   1.001 ins/byte,    0.849 cycle/byte,    3.762 GB/s (4.5 %),     3.195 GHz,    1.178 ins/cycle 
   1.001 ins/char,    0.849 cycle/char,    3.762 Gc/s (4.5 %)     1.00 byte/char 
*/
/*
std::pair<const char*, char32_t*> sse_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    const char* end = buf + len;

    while (buf + 32 <= end) {  // Process 32 bytes at a time
        // Chunk 1
        __m128i in1 = _mm_load_si128((__m128i*)buf);
        _mm_store_si128((__m128i*)utf32_output, _mm_cvtepu8_epi32(in1));
        _mm_store_si128((__m128i*)(utf32_output + 4), _mm_cvtepu8_epi32(_mm_srli_si128(in1, 4)));
        _mm_store_si128((__m128i*)(utf32_output + 8), _mm_cvtepu8_epi32(_mm_srli_si128(in1, 8)));
        _mm_store_si128((__m128i*)(utf32_output + 12), _mm_cvtepu8_epi32(_mm_srli_si128(in1, 12)));

        // Chunk 2
        __m128i in2 = _mm_load_si128((__m128i*)(buf + 16));
        _mm_store_si128((__m128i*)(utf32_output + 16), _mm_cvtepu8_epi32(in2));
        _mm_store_si128((__m128i*)(utf32_output + 20), _mm_cvtepu8_epi32(_mm_srli_si128(in2, 4)));
        _mm_store_si128((__m128i*)(utf32_output + 24), _mm_cvtepu8_epi32(_mm_srli_si128(in2, 8)));
        _mm_store_si128((__m128i*)(utf32_output + 28), _mm_cvtepu8_epi32(_mm_srli_si128(in2, 12)));

        utf32_output += 32;
        buf += 32;
    }

    return std::make_pair(buf, utf32_output);
}
 */

/*  
convert_latin1_to_utf32+westmere, input size: 432305, iterations: 20000, dataset: unicode_lipsum/wikipedia_mars/french.latin1.txt
   1.126 ins/byte,    0.797 cycle/byte,    4.010 GB/s (5.3 %),     3.195 GHz,    1.413 ins/cycle 
   1.126 ins/char,    0.797 cycle/char,    4.010 Gc/s (5.3 %)     1.00 byte/char */
/* std::pair<const char*, char32_t*> sse_convert_latin1_to_utf32(const char* buf, size_t len, char32_t* utf32_output) {
    const char* end = buf + len;

    while (buf + 16 <= end) {
        // Load 16 Latin1 characters (16 bytes) into a 128-bit register
        __m128i in = _mm_load_si128((__m128i*)buf); // Assuming alignment

        // Convert first 4 bytes to 4 32-bit integers
        __m128i out1 = _mm_cvtepu8_epi32(in);
        _mm_store_si128((__m128i*)utf32_output, out1);

        // Shift input to process next 4 bytes
        __m128i in_shifted1 = _mm_srli_si128(in, 4);
        __m128i out2 = _mm_cvtepu8_epi32(in_shifted1);
        _mm_store_si128((__m128i*)(utf32_output + 4), out2);

        // Shift input to process next set of 4 bytes
        __m128i in_shifted2 = _mm_srli_si128(in, 8);
        __m128i out3 = _mm_cvtepu8_epi32(in_shifted2);
        _mm_store_si128((__m128i*)(utf32_output + 8), out3);

        // Convert last 4 bytes to 4 32-bit integers
        __m128i in_shifted3 = _mm_srli_si128(in, 12);
        __m128i out4 = _mm_cvtepu8_epi32(in_shifted3);
        _mm_store_si128((__m128i*)(utf32_output + 12), out4);

        utf32_output += 16;
        buf += 16;
    }

    return std::make_pair(buf, utf32_output);
}
 */
