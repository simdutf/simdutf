/* convert_latin1_to_utf16+haswell, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.313 ins/byte,    0.244 cycle/byte,   13.119 GB/s (5.1 %),     3.200 GHz,    1.284 ins/cycle 
   0.313 ins/char,    0.244 cycle/char,   13.119 Gc/s (5.1 %)     1.00 byte/char 
convert_latin1_to_utf16+icelake, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.157 ins/byte,    0.220 cycle/byte,   14.074 GB/s (2.4 %),     3.101 GHz,    0.712 ins/cycle 
   0.157 ins/char,    0.220 cycle/char,   14.074 Gc/s (2.4 %)     1.00 byte/char 
convert_latin1_to_utf16+iconv, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
  31.022 ins/byte,    7.011 cycle/byte,    0.455 GB/s (0.4 %),     3.193 GHz,    4.425 ins/cycle 
  31.022 ins/char,    7.011 cycle/char,    0.455 Gc/s (0.4 %)     1.00 byte/char 
convert_latin1_to_utf16+icu, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   2.505 ins/byte,    0.638 cycle/byte,    5.011 GB/s (1.2 %),     3.195 GHz,    3.929 ins/cycle 
   2.505 ins/char,    0.638 cycle/char,    5.011 Gc/s (1.2 %)     1.00 byte/char 
convert_latin1_to_utf16+westmere, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.563 ins/byte,    0.181 cycle/byte,   17.678 GB/s (2.8 %),     3.203 GHz,    3.108 ins/cycle 
   0.563 ins/char,    0.181 cycle/char,   17.678 Gc/s (2.8 %)     1.00 byte/char  */
/* template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char *latin1_input, size_t len,
                                    char16_t *utf16_output) {
  size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 16

  __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                        0x0607040502030001, 0x0e0f0c0d0a0b0809);
  
  for (size_t i = 0; i < rounded_len; i += 16) {
    // Load 16 Latin1 characters into a 128-bit register
    __m128i in = _mm_loadu_si128((__m128i *)&latin1_input[i]);
    // Zero extend each set of 8 Latin1 characters to 16 16-bit integers
    __m256i out = _mm256_cvtepu8_epi16(in);
    if (big_endian) {
      out = _mm256_shuffle_epi8(out, byteflip);
    }
    // Store the results back to memory
    _mm256_storeu_si256((__m256i *)&utf16_output[i], out);
  }
  
    // return pointers pointing to where we left off
    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

} */

/* convert_latin1_to_utf16+haswell, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.219 ins/byte,    0.207 cycle/byte,   15.423 GB/s (3.2 %),     3.199 GHz,    1.058 ins/cycle 
   0.219 ins/char,    0.207 cycle/char,   15.423 Gc/s (3.2 %)     1.00 byte/char 
convert_latin1_to_utf16+icelake, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.157 ins/byte,    0.217 cycle/byte,   14.287 GB/s (2.1 %),     3.103 GHz,    0.722 ins/cycle 
   0.157 ins/char,    0.217 cycle/char,   14.287 Gc/s (2.1 %)     1.00 byte/char  */


template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char* latin1_input, size_t len, char16_t* utf16_output) {
    size_t rounded_len = len & ~0x1F; // Round down to nearest multiple of 32
    
    __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                          0x0607040502030001, 0x0e0f0c0d0a0b0809);
    
    for (size_t i = 0; i < rounded_len; i += 32) {
        __m128i in_lo = _mm_loadu_si128((__m128i*)&latin1_input[i]);
        __m128i in_hi = _mm_loadu_si128((__m128i*)&latin1_input[i + 16]);
        
        __m256i out_lo = _mm256_cvtepu8_epi16(in_lo);
        __m256i out_hi = _mm256_cvtepu8_epi16(in_hi);
        
        if (big_endian){
            out_lo = _mm256_shuffle_epi8(out_lo, byteflip);
            out_hi = _mm256_shuffle_epi8(out_hi, byteflip);
        }
        
        _mm256_storeu_si256((__m256i*)&utf16_output[i], out_lo);
        _mm256_storeu_si256((__m256i*)&utf16_output[i + 16], out_hi);
    }

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

}


/* convert_latin1_to_utf16+haswell, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.532 ins/byte,    0.448 cycle/byte,    7.139 GB/s (2.9 %),     3.197 GHz,    1.188 ins/cycle 
   0.532 ins/char,    0.448 cycle/char,    7.139 Gc/s (2.9 %)     1.00 byte/char 
convert_latin1_to_utf16+icelake, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.157 ins/byte,    0.220 cycle/byte,   14.123 GB/s (2.1 %),     3.101 GHz,    0.714 ins/cycle 
   0.157 ins/char,    0.220 cycle/char,   14.123 Gc/s (2.1 %)     1.00 byte/char  */

/* template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char* latin1_input, size_t len, char16_t* utf16_output) {
    size_t rounded_len = len & ~0x1F; // Round down to nearest multiple of 32
    
    __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                          0x0607040502030001, 0x0e0f0c0d0a0b0809);
    
    for (size_t i = 0; i < rounded_len; i += 32) {
        __m128i in_lo = _mm_loadu_si128((__m128i*)&latin1_input[i]);
        __m128i in_hi = _mm_loadu_si128((__m128i*)&latin1_input[i + 16]);
        
        __m256i out_lo = _mm256_cvtepu8_epi16(in_lo);
        __m256i out_hi = _mm256_cvtepu8_epi16(in_hi);
        
        if (big_endian){
            out_lo = _mm256_shuffle_epi8(out_lo, byteflip);
            out_hi = _mm256_shuffle_epi8(out_hi, byteflip);
        }
        
        _mm256_storeu_si256((__m256i*)&utf16_output[i], out_lo);
        _mm256_storeu_si256((__m256i*)&utf16_output[i + 16], out_hi);
    }

    rounded_len = len & ~0xF; // Round down to nearest multiple of 32

     for (size_t i = 0; i < rounded_len; i += 16) {
        // Load 16 Latin1 characters into a 128-bit register
        __m128i in = _mm_loadu_si128((__m128i *)&latin1_input[i]);
        // Zero extend each set of 8 Latin1 characters to 16 16-bit integers
        __m256i out = _mm256_cvtepu8_epi16(in);
        if (big_endian) {
          out = _mm256_shuffle_epi8(out, byteflip);
        }
        // Store the results back to memory
        _mm256_storeu_si256((__m256i *)&utf16_output[i], out);
      }

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

} */

//  not working but seems to be about as fast as the others so probably not worth pursuing:
/* template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char* latin1_input, size_t len, char16_t* utf16_output) {
    size_t rounded_len = len & ~0x1F; // Round down to nearest multiple of 32
    
    __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                          0x0607040502030001, 0x0e0f0c0d0a0b0809);
    
    for (size_t i = 0; i < rounded_len; i += 32) {
        __m256i load = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(latin1_input + i));
        __m256i lo = _mm256_unpacklo_epi8(load, _mm256_setzero_si256()); // in theory unpacklo takes less cycles than cvtge
        __m256i hi = _mm256_unpackhi_epi8(load, _mm256_setzero_si256());
        
        if (big_endian) {
            lo = _mm256_shuffle_epi8(lo, byteflip);
            hi = _mm256_shuffle_epi8(hi, byteflip);
        }
        
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(utf16_output + i), lo);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(utf16_output + i + 16), hi);
    }

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);
} */
