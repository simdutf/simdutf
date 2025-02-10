template <endianness big_endian>
std::pair<const char *, char16_t *>
avx2_convert_latin1_to_utf16(const char *latin1_input, size_t len,
                             char16_t *utf16_output) {
  size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 16

  size_t i = 0;
  for (; i < rounded_len; i += 16) {
    // Load 16 bytes from the address (input + i) into a xmm register
    const __m128i latin1 =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(latin1_input + i));

    // Zero extend each byte in `in` to word
    __m256i utf16 = _mm256_cvtepu8_epi16(latin1);

    if (big_endian) {
      const __m128i swap128 =
          _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
      const __m256i swap = _mm256_set_m128i(swap128, swap128);
      utf16 = _mm256_shuffle_epi8(utf16, swap);
    }

    // Store the contents of xmm1 into the address pointed by (output + i)
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(utf16_output + i), utf16);
  }

  return std::make_pair(latin1_input + rounded_len, utf16_output + rounded_len);
}
