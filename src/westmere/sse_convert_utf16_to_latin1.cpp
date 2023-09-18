template <endianness big_endian>
std::pair<const char16_t*, char*> sse_convert_utf16_to_latin1(const char16_t* buf, size_t len, char* latin1_output) {
  const char16_t* end = buf + len;
  while (buf + 8 <= end) {
    // Load 8 UTF-16 characters into 128-bit SSE register
    __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buf));

    if (!match_system(big_endian)) {
      const __m128i swap = _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
      in = _mm_shuffle_epi8(in, swap);
    }

    __m128i high_byte_mask = _mm_set1_epi16(0xFF00);
    if (_mm_testz_si128(in, high_byte_mask)) {
      // Pack 16-bit characters into 8-bit and store in latin1_output
      __m128i latin1_packed = _mm_packus_epi16(in, in);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(latin1_output), latin1_packed);
      // Adjust pointers for next iteration
      buf += 8;
      latin1_output += 8;
    } else {
      return std::make_pair(nullptr, reinterpret_cast<char*>(latin1_output));
    }
  } // while
  return std::make_pair(buf, latin1_output);
}
