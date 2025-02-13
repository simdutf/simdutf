template <endianness big_endian>
std::pair<const char16_t *, char *>
ppc64_convert_utf16_to_latin1(const char16_t *buf, size_t len,
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
    const auto tmp = vec_u64_t(in.value);
    memcpy(latin1_output, &tmp[0], 8);
    // AltiVec

    if (simdutf_unlikely(tmp[1] != 0)) {
      return std::make_pair(nullptr, reinterpret_cast<char *>(latin1_output));
    } else {
      // Adjust pointers for next iteration
      buf += 8;
      latin1_output += 8;
    }
  } // while

  return std::make_pair(buf, latin1_output);
}

template <endianness big_endian>
std::pair<result, char *>
ppc64_convert_utf16_to_latin1_with_errors(const char16_t *buf, size_t len,
                                          char *latin1_output) {
  const char16_t *start = buf;
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
    const auto tmp = vec_u64_t(in.value);
    memcpy(latin1_output, &tmp[0], 8);
    // AltiVec

    if (simdutf_unlikely(tmp[1] != 0)) {
      uint8_t bytes[8];
      memcpy(bytes, &tmp[1], 8);
      for (size_t k = 0; k < 8; k++) {
        if (bytes[k] != 0) {
          return std::make_pair(result(error_code::TOO_LARGE, buf - start + k),
                                latin1_output);
        }
      }
    } else {
      // Adjust pointers for next iteration
      buf += 8;
      latin1_output += 8;
    }
  } // while

  return std::make_pair(result(error_code::SUCCESS, buf - start),
                        latin1_output);
}
