struct utf16_to_latin1_t {
  error_code err;
  const char16_t *input;
  char *output;
};

template <endianness big_endian>
utf16_to_latin1_t ppc64_convert_utf16_to_latin1(const char16_t *buf, size_t len,
                                                char *latin1_output) {
  const char16_t *end = buf + len;
  while (end - buf >= 8) {
    // Load 8 x UTF-16 characters
    auto in = vector_u8::load(buf);

    // Move low bytes of UTF-16 chars to lower half of `in`
    // and upper bytes to upper half of `in`.
    if (!match_system(big_endian)) {
      const auto perm =
          vector_u8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15);
      in = perm.lookup_16(in);
    } else {
      const auto perm =
          vector_u8(1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14);
      in = perm.lookup_16(in);
    }

    // AltiVec-specific
#if defined(__clang__)
    __attribute__((aligned(16))) uint64_t tmp[8];
    in.store(tmp);
  #if SIMDUTF_IS_BIG_ENDIAN
    memcpy(latin1_output, &tmp[0], 8);
    const uint64_t upper = tmp[1];
  #else
    memcpy(latin1_output, &tmp[1], 8);
    const uint64_t upper = tmp[0];
  #endif // SIMDUTF_IS_BIG_ENDIAN
#else
    const auto tmp = vec_u64_t(in.value);
  #if SIMDUTF_IS_BIG_ENDIAN
    memcpy(latin1_output, &tmp[0], 8);
    const uint64_t upper = tmp[1];
  #else
    memcpy(latin1_output, &tmp[1], 8);
    const uint64_t upper = tmp[0];
  #endif // SIMDUTF_IS_BIG_ENDIAN
#endif   // defined(__clang__)
    // AltiVec

    if (simdutf_unlikely(upper)) {
      uint8_t bytes[8];
      memcpy(bytes, &upper, 8);
      for (size_t k = 0; k < 8; k++) {
        if (bytes[k] != 0) {
          return utf16_to_latin1_t{error_code::TOO_LARGE, buf + k,
                                   latin1_output};
        }
      }
    } else {
      // Adjust pointers for next iteration
      buf += 8;
      latin1_output += 8;
    }
  } // while

  return utf16_to_latin1_t{error_code::SUCCESS, buf, latin1_output};
}
