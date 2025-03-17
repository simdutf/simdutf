struct utf16_to_utf32_t {
  error_code err;        // error code
  const char16_t *input; // last position in input buffer
  char32_t *output;      // last position in output buffer
};

template <endianness big_endian>
utf16_to_utf32_t ppc64_convert_utf16_to_utf32(const char16_t *buf, size_t len,
                                              char32_t *utf32_output) {
  const char16_t *end = buf + len;

  const auto v_f800 = vector_u16::splat(0xf800);
  const auto v_d800 = vector_u16::splat(0xd800);
  const auto zero = vector_u8::zero();

  while (end - buf >= vector_u16::ELEMENTS) {
    auto in = vector_u16::load(buf);
    if (not match_system(big_endian)) {
      in = in.swap_bytes();
    }

    // 1. Check if there are any surrogate word in the input chunk.
    //    We have also deal with situation when there is a surrogate word
    //    at the end of a chunk.
    const auto surrogates_bytemask = (in & v_f800) == v_d800;

    // bitmask = 0x0000 if there are no surrogates
    const uint16_t surrogates_bitmask = surrogates_bytemask.to_bitmask();

    // It might seem like checking for surrogates_bitmask == 0xc000 could help.
    // However, it is likely an uncommon occurrence.
    if (surrogates_bitmask == 0x0000) {
      // case: no surrogate pairs, extend 16-bit code units to 32-bit code units
#if SIMDUTF_IS_BIG_ENDIAN
      const auto lo =
          vector_u8(16, 16, 0, 1, 16, 16, 2, 3, 16, 16, 4, 5, 16, 16, 6, 7);
      const auto hi = vector_u8(16, 16, 8 + 0, 8 + 1, 16, 16, 8 + 2, 8 + 3, 16,
                                16, 8 + 4, 8 + 5, 16, 16, 8 + 6, 8 + 7);
#else
      const auto lo =
          vector_u8(0, 1, 16, 16, 2, 3, 16, 16, 4, 5, 16, 16, 6, 7, 16, 16);
      const auto hi = vector_u8(8 + 0, 8 + 1, 16, 16, 8 + 2, 8 + 3, 16, 16,
                                8 + 4, 8 + 5, 16, 16, 8 + 6, 8 + 7, 16, 16);
#endif // SIMDUTF_IS_BIG_ENDIAN

      const auto utf32_0 = lo.lookup_32(as_vector_u8(in), zero);
      const auto utf32_1 = hi.lookup_32(as_vector_u8(in), zero);

      utf32_0.store(utf32_output);
      utf32_1.store(utf32_output + 4);
      utf32_output += 8;
      buf += 8;
      // surrogate pair(s) in a register
    } else {
      // Let us do a scalar fallback.
      // It may seem wasteful to use scalar code, but being efficient with SIMD
      // in the presence of surrogate pairs may require non-trivial tables.
      size_t forward = 15;
      size_t k = 0;
      if (size_t(end - buf) < forward + 1) {
        forward = size_t(end - buf - 1);
      }
      for (; k < forward; k++) {
        const uint16_t word = not match_system(big_endian)
                                  ? scalar::u16_swap_bytes(buf[k])
                                  : buf[k];
        if ((word & 0xF800) != 0xD800) {
          *utf32_output++ = char32_t(word);
        } else {
          // must be a surrogate pair
          uint16_t diff = uint16_t(word - 0xD800);
          uint16_t next_word = not match_system(big_endian)
                                   ? scalar::u16_swap_bytes(buf[k + 1])
                                   : buf[k + 1];
          k++;
          uint16_t diff2 = uint16_t(next_word - 0xDC00);
          if ((diff | diff2) > 0x3FF) {
            return utf16_to_utf32_t{error_code::SURROGATE, buf + k - 1,
                                    utf32_output};
          }
          uint32_t value = (diff << 10) + diff2 + 0x10000;
          *utf32_output++ = char32_t(value);
        }
      }
      buf += k;
    }
  } // while

  return utf16_to_utf32_t{error_code::SUCCESS, buf, utf32_output};
}
