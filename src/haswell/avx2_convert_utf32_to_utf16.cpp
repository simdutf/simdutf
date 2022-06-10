std::pair<const char32_t*, char16_t*> avx2_convert_utf32_to_utf16(const char32_t* buf, size_t len, char16_t* utf16_output) {
  const char32_t* end = buf + len;

  const size_t safety_margin = 11; // to avoid overruns, see issue https://github.com/simdutf/simdutf/issues/92

  while (buf + 8 + safety_margin <= end) {
    __m256i in = _mm256_loadu_si256((__m256i*)buf);

    const __m256i v_00000000 = _mm256_setzero_si256();
    const __m256i v_ffff0000 = _mm256_set1_epi32((int32_t)0xffff0000);

    // no bits set above 16th bit <=> can pack to UTF16 without surrogate pairs
    const __m256i saturation__bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff0000), v_00000000);
    const uint32_t saturation__bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(saturation__bytemask));

    if (saturation__bitmask == 0xffffffff) {
      const __m128i utf16_packed = _mm_packus_epi32(_mm256_castsi256_si128(in),_mm256_extractf128_si256(in,1));
      _mm_storeu_si128((__m128i*)utf16_output, utf16_packed);
      utf16_output += 8;
      buf += 8;
    } else {
      size_t forward = 7;
      size_t k = 0;
      if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
      for(; k < forward; k++) {
        uint32_t word = buf[k];
        if((word & 0xFFFF0000)==0) {
          // will not generate a surrogate pair
          *utf16_output++ = char16_t(word);
        } else {
          // will generate a surrogate pair
          word -= 0x10000;
          *utf16_output++ = char16_t(0xD800 + (word >> 10));
          *utf16_output++ = char16_t(0xDC00 + (word & 0x3FF));
        }
      }
      buf += k;
    }
  }

  return std::make_pair(buf, utf16_output);
}