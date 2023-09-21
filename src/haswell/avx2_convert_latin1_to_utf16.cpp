template <endianness big_endian>
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
  
/*   if (rounded_len != len) {
    __m128i in = _mm_loadu_si128((__m128i *)&latin1_input[rounded_len]);
    __m256i out = _mm256_cvtepu8_epi16(in);
    if (big_endian) {
      out = _mm256_shuffle_epi8(out, byteflip);
    }
    // Handle the mask store manually for epi16
    __m256i orig = _mm256_loadu_si256((__m256i *)&utf16_output[rounded_len]);
    __m256i mask_vec = _mm256_set1_epi16((1 << (len - rounded_len)) - 1);
    __m256i blended = _mm256_blendv_epi8(orig, out, mask_vec);
    _mm256_storeu_si256((__m256i *)&utf16_output[rounded_len], blended);
  }

  return len; */
    // return pointers pointing to where we left off
    return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);

}


/* template <endianness big_endian>
size_t avx2_convert_latin1_to_utf16(const char *latin1_input, size_t len,
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
  
  if (rounded_len != len) {
    uint16_t mask = uint16_t(1 << (len - rounded_len)) - 1;
    __m128i in = _mm_maskz_loadu_epi8(mask, latin1_input + rounded_len);
    __m256i out = _mm256_cvtepu8_epi16(in);
    if (big_endian) {
      out = _mm256_shuffle_epi8(out, byteflip);
    }
    // Handle the mask store manually for epi16
    __m256i orig = _mm256_loadu_si256((__m256i *)&utf16_output[rounded_len]);
    __m256i blended = _mm256_blendv_epi8(orig, out, _mm256_cvtepi16_epi32(_mm_set1_epi16(mask)));
    _mm256_storeu_si256((__m256i *)&utf16_output[rounded_len], blended);
  }

  return len;
} */


/* #include <immintrin.h>

template <endianness big_endian>
size_t icelake_convert_latin1_to_utf16(const char *latin1_input, size_t len,
                                       char16_t *utf16_output) {
  size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 16

  __m256i byteflip = _mm256_setr_epi64x(0x0607040502030001, 0x0e0f0c0d0a0b0809,
                                        0x0607040502030001, 0x0e0f0c0d0a0b0809);
  for (size_t i = 0; i < rounded_len; i += 16) {
    // Load 16 Latin1 characters into a 128-bit register
    __m128i in = _mm_loadu_si128((__m128i *)&latin1_input[i]);
    // Zero extend each set of 8 Latin1 characters to 16 times 16-bit integers
    __m256i out = _mm256_cvtepu8_epi16(in);
    if (big_endian) {
      out = _mm256_shuffle_epi8(out, byteflip);
    }
    // Store the results back to memory
    _mm256_storeu_si256((__m256i *)&utf16_output[i], out);
  }
  if (rounded_len != len) {
    uint16_t mask = uint16_t(1 << (len - rounded_len)) - 1;
    __m128i in = _mm_maskz_loadu_epi8(mask, latin1_input + rounded_len);

    // Zero extend each set of 8 Latin1 characters to 16 16-bit integers
    __m256i out = _mm256_cvtepu8_epi16(in);
    if (big_endian) {
      out = _mm256_shuffle_epi8(out, byteflip);
    }
    // Store the results back to memory
    _mm256_maskstore_epi16(reinterpret_cast<int16_t *>(utf16_output + rounded_len), _mm256_set1_epi16(mask), out);
  }

  return len;
}
 */
