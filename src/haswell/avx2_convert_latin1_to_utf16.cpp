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

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

} */


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


// par
/* convert_latin1_to_utf16+haswell, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.563 ins/byte,    0.184 cycle/byte,   17.426 GB/s (2.4 %),     3.204 GHz,    3.063 ins/cycle 
   0.563 ins/char,    0.184 cycle/char,   17.426 Gc/s (2.4 %)     1.00 byte/char 
convert_latin1_to_utf16+icelake, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.157 ins/byte,    0.221 cycle/byte,   14.005 GB/s (2.2 %),     3.101 GHz,    0.708 ins/cycle 
   0.157 ins/char,    0.221 cycle/char,   14.005 Gc/s (2.2 %)     1.00 byte/char 
convert_latin1_to_utf16+iconv, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
  31.022 ins/byte,    7.011 cycle/byte,    0.455 GB/s (0.3 %),     3.193 GHz,    4.425 ins/cycle 
  31.022 ins/char,    7.011 cycle/char,    0.455 Gc/s (0.3 %)     1.00 byte/char 
convert_latin1_to_utf16+icu, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   2.505 ins/byte,    0.640 cycle/byte,    4.992 GB/s (0.7 %),     3.196 GHz,    3.913 ins/cycle 
   2.505 ins/char,    0.640 cycle/char,    4.992 Gc/s (0.7 %)     1.00 byte/char 
convert_latin1_to_utf16+westmere, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.563 ins/byte,    0.188 cycle/byte,   17.047 GB/s (2.1 %),     3.203 GHz,    2.997 ins/cycle 
   0.563 ins/char,    0.188 cycle/char,   17.047 Gc/s (2.1 %)     1.00 byte/char */

template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char* latin1_input, size_t len, char16_t* utf16_output) {
    size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 32
    
/*     __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                          0x0607040502030001, 0x0e0f0c0d0a0b0809); */

    size_t i = 0;
    for (; i < rounded_len; i += 16) {
        // Load 16 bytes from the address (input + i) into xmm0 register
        __m128i xmm0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(latin1_input + i));

/*         if (big_endian) {
            __m128i byteflip = _mm_setr_epi8(0x01, 0x00, 0x03, 0x02, 0x05, 0x04, 0x07, 0x06,
                                  0x09, 0x08, 0x0b, 0x0a, 0x0d, 0x0c, 0x0f, 0x0e);    
            xmm0 = _mm_shuffle_epi8(xmm0, byteflip);
        } */



        // Zero extend each byte in xmm0 to word in xmm1
        __m128i xmm1 = _mm_cvtepu8_epi16(xmm0);
        
        // Shift xmm0 to the right by 8 bytes
        xmm0 = _mm_srli_si128(xmm0, 8);
        
        // Zero extend each byte in the shifted xmm0 to word in xmm0
        xmm0 = _mm_cvtepu8_epi16(xmm0);

        if (big_endian) {
            const __m128i swap = _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
            xmm0 = _mm_shuffle_epi8(xmm0, swap);
            xmm1 = _mm_shuffle_epi8(xmm1, swap);

        }
        
        // Store the contents of xmm1 into the address pointed by (output + i)
        _mm_storeu_si128(reinterpret_cast<__m128i*>(utf16_output + i), xmm1);
        
        // Store the contents of xmm0 into the address pointed by (output + i + 8)
        _mm_storeu_si128(reinterpret_cast<__m128i*>(utf16_output + i + 8), xmm0);

        // The rest of the assembly instructions seem to be for loop control and address calculations, which are automatically handled in C++.
    }

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

}


/* convert_latin1_to_utf16+haswell, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.313 ins/byte,    0.204 cycle/byte,   15.662 GB/s (7.1 %),     3.200 GHz,    1.533 ins/cycle 
   0.313 ins/char,    0.204 cycle/char,   15.662 Gc/s (7.1 %)     1.00 byte/char 
convert_latin1_to_utf16+icelake, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.157 ins/byte,    0.225 cycle/byte,   13.768 GB/s (2.6 %),     3.102 GHz,    0.696 ins/cycle 
   0.157 ins/char,    0.225 cycle/char,   13.768 Gc/s (2.6 %)     1.00 byte/char  */
/* 
template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char* latin1_input, size_t len, char16_t* utf16_output) {
    size_t rounded_len = len & ~0x1F; // Round down to nearest multiple of 32
    
    __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                          0x0607040502030001, 0x0e0f0c0d0a0b0809);
    
    size_t i = 0;

    for (; i < rounded_len; i += 32) {
        // Load 32 bytes from the address (latin1_input + i) into ymm0 register
        __m256i ymm0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(latin1_input + i));
        
        // Zero extend lower 16 bytes in ymm0 to 16 bits each in ymm1
        __m256i ymm1 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(ymm0, 0));

        // ymm0 = _mm256_srli_si256(ymm0,16);
        
        // Zero extend higher 16 bytes in ymm0 to 16 bits each in ymm0
        ymm0 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(ymm0, 1));
        
        // Store the contents of ymm1 and ymm0 into the address pointed by (utf16_output + i)
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(utf16_output + i), ymm1);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(utf16_output + i + 16), ymm0);
    }

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

} */



/* convert_latin1_to_utf16+haswell, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.469 ins/byte,    0.191 cycle/byte,   16.756 GB/s (2.2 %),     3.202 GHz,    2.456 ins/cycle 
   0.469 ins/char,    0.191 cycle/char,   16.756 Gc/s (2.2 %)     1.00 byte/char 
convert_latin1_to_utf16+icelake, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.157 ins/byte,    0.232 cycle/byte,   13.351 GB/s (2.3 %),     3.101 GHz,    0.675 ins/cycle 
   0.157 ins/char,    0.232 cycle/char,   13.351 Gc/s (2.3 %)     1.00 byte/char 
convert_latin1_to_utf16+iconv, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
  31.022 ins/byte,    7.011 cycle/byte,    0.455 GB/s (0.3 %),     3.193 GHz,    4.425 ins/cycle 
  31.022 ins/char,    7.011 cycle/char,    0.455 Gc/s (0.3 %)     1.00 byte/char 
convert_latin1_to_utf16+icu, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   2.505 ins/byte,    0.639 cycle/byte,    5.001 GB/s (1.0 %),     3.195 GHz,    3.921 ins/cycle 
   2.505 ins/char,    0.639 cycle/char,    5.001 Gc/s (1.0 %)     1.00 byte/char 
convert_latin1_to_utf16+westmere, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.563 ins/byte,    0.185 cycle/byte,   17.357 GB/s (2.7 %),     3.205 GHz,    3.049 ins/cycle 
   0.563 ins/char,    0.185 cycle/char,   17.357 Gc/s (2.7 %)     1.00 byte/char  */

/* template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char* latin1_input, size_t len, char16_t* utf16_output) {
    size_t rounded_len = len & ~0x1F; // Round down to nearest multiple of 32
    
    __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                          0x0607040502030001, 0x0e0f0c0d0a0b0809);
    
    size_t i = 0;

for (; i < rounded_len; i += 32) {
    // Load first 16 bytes from the address (input + i) into xmm0 register
    __m128i xmm0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(latin1_input + i));
    
    // Zero extend each byte in xmm0 to word in xmm1
    __m128i xmm1 = _mm_cvtepu8_epi16(xmm0);
    
    // Shift xmm0 to the right by 8 bytes
    xmm0 = _mm_srli_si128(xmm0, 8);
    
    // Zero extend each byte in the shifted xmm0 to word in xmm0
    xmm0 = _mm_cvtepu8_epi16(xmm0);
    
    // Load second 16 bytes from the address (input + i + 16) into xmm2 register
    __m128i xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(latin1_input + i + 16));
    
    // Zero extend each byte in xmm2 to word in xmm3
    __m128i xmm3 = _mm_cvtepu8_epi16(xmm2);
    
    // Shift xmm2 to the right by 8 bytes
    xmm2 = _mm_srli_si128(xmm2, 8);
    
    // Zero extend each byte in the shifted xmm2 to word in xmm2
    xmm2 = _mm_cvtepu8_epi16(xmm2);
    
    // Store the contents of xmm1 and xmm0 into the address pointed by (output + i)
    _mm_storeu_si128(reinterpret_cast<__m128i*>(utf16_output + i), xmm1);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(utf16_output + i + 8), xmm0);
    
    // Store the contents of xmm3 and xmm2 into the address pointed by (output + i + 16)
    _mm_storeu_si128(reinterpret_cast<__m128i*>(utf16_output + i + 16), xmm3);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(utf16_output + i + 24), xmm2);
}

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

} */


/* convert_latin1_to_utf16+haswell, input size: 432305, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.latin1.txt
   0.282 ins/byte,    0.207 cycle/byte,   15.425 GB/s (4.0 %),     3.200 GHz,    1.359 ins/cycle 
   0.282 ins/char,    0.207 cycle/char,   15.425 Gc/s (4.0 %)     1.00 byte/char  */
/* template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char* latin1_input, size_t len, char16_t* utf16_output) {
    size_t rounded_len = len & ~0x1F; // Round down to nearest multiple of 32
    
    __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                          0x0607040502030001, 0x0e0f0c0d0a0b0809);
    
    size_t i = 0;
    for (; i < rounded_len; i += 32) {
        // Load 32 bytes from the address (input + i) into ymm register
        __m256i ymm0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(latin1_input + i));
        
        // Zero extend the lower 16 bytes in ymm0 to words in ymm1
        __m256i ymm1 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(ymm0));
        
        // Shift ymm0 to the right by 16 bytes
        ymm0 = _mm256_permute4x64_epi64(ymm0, _MM_SHUFFLE(1, 0, 3, 2));
        
        // Zero extend each byte in the shifted ymm0 to word in ymm0
        ymm0 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(ymm0));
        
        // Interleave ymm1 and ymm0 and store the result
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(utf16_output + i), ymm1);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(utf16_output + i + 16), ymm0);
    }

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

} */

/* template <endianness big_endian>
std::pair<const char*, char16_t*> avx2_convert_latin1_to_utf16(const char* latin1_input, size_t len, char16_t* utf16_output) {
    size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 32
    
    __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                          0x0607040502030001, 0x0e0f0c0d0a0b0809);
    
    size_t i = 0;
    for (; i < rounded_len; i += 16) {
        // Load 16 bytes from the address (input + i) into xmm0 register
        __m128i xmm0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(latin1_input + i));
        
        // Zero extend each byte in xmm0 to word in xmm1
        __m128i xmm1 = _mm_cvtepu8_epi16(xmm0);
        
        // Shift xmm0 to the right by 8 bytes
        xmm0 = _mm_srli_si128(xmm0, 8);
        
        // Zero extend each byte in the shifted xmm0 to word in xmm0
        xmm0 = _mm_cvtepu8_epi16(xmm0);
        
    // Combine xmm1 and xmm0 into ymm1
    // __m256i ymm1 = _mm256_setr_m128i(xmm1, xmm0);
    __m256i ymm1 = _mm256_insertf128_si256(_mm256_castsi128_si256(xmm1), xmm0, 1);
    
    // Store the contents of ymm1 into the address pointed by (utf16_output + i)
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(utf16_output + i), ymm1);
        // The rest of the assembly instructions seem to be for loop control and address calculations, which are automatically handled in C++.
    }

    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

} */