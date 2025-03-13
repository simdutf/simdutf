struct utf32_to_utf16_t {
  error_code err;
  const char32_t *input;
  char16_t *output;
};

template <endianness big_endian, ErrorReporting er>
utf32_to_utf16_t ppc64_convert_utf32_to_utf16(const char32_t *buf, size_t len,
                                              char16_t *utf16_output) {

  const char32_t *end = buf + len;

  const auto zero = vector_u32::zero();
  const auto v_ffff0000 = vector_u32::splat(0xffff0000);

  auto forbidden_global = simd16<bool>();

  while (end - buf >= 8) {
    const auto in0 = vector_u32::load(buf);
    const auto in1 = vector_u32::load(buf + vector_u32::ELEMENTS);

    const auto any_surrogate = ((in0 | in1) & v_ffff0000) != zero;

    // Check if no bits set above 15th
    if (any_surrogate.is_zero()) {
      // Pack UTF-32 to UTF-16
#if SIMDUTF_IS_BIG_ENDIAN
      const auto sh = big_endian ? vector_u8(2, 3, 6, 7, 10, 11, 14, 15, 18, 19,
                                             22, 23, 26, 27, 30, 31)
                                 : vector_u8(3, 2, 7, 6, 11, 10, 15, 14, 19, 18,
                                             23, 22, 27, 26, 31, 30);
#else
      const auto sh = big_endian ? vector_u8(1, 0, 5, 4, 9, 8, 13, 12, 17, 16,
                                             21, 20, 25, 24, 29, 28)
                                 : vector_u8(0, 1, 4, 5, 8, 9, 12, 13, 16, 17,
                                             20, 21, 24, 25, 28, 29);
#endif // SIMDUTF_IS_BIG_ENDIAN
      const auto packed0 = sh.lookup_32(as_vector_u8(in0), as_vector_u8(in1));
      const auto packed = as_vector_u16(packed0);

#if SIMDUTF_IS_BIG_ENDIAN
      const auto v_f800 =
          big_endian ? vector_u16::splat(0xf800) : vector_u16::splat(0x00f8);
      const auto v_d800 =
          big_endian ? vector_u16::splat(0xd800) : vector_u16::splat(0x00d8);
#else
      const auto v_f800 =
          big_endian ? vector_u16::splat(0x00f8) : vector_u16::splat(0xf800);
      const auto v_d800 =
          big_endian ? vector_u16::splat(0x00d8) : vector_u16::splat(0xd800);
#endif // SIMDUTF_IS_BIG_ENDIAN
      const auto forbidden = (packed & v_f800) == v_d800;

      switch (er) {
      case ErrorReporting::precise:
        if (not forbidden.is_zero()) {
          // scalar procedure will rescan the portion of buffer we've just
          // analysed
          return utf32_to_utf16_t{error_code::OTHER, buf, utf16_output};
        }
        break;
      case ErrorReporting::at_the_end:
        forbidden_global |= forbidden;
        break;
      case ErrorReporting::none:
        break;
      }

      packed.store(utf16_output);
      utf16_output += 8;
      buf += 8;
    } else {
      size_t forward = 7;
      size_t k = 0;
      if (size_t(end - buf) < forward + 1) {
        forward = size_t(end - buf - 1);
      }
      for (; k < forward; k++) {
        uint32_t word = buf[k];
        if ((word & 0xFFFF0000) == 0) {
          // will not generate a surrogate pair
          if (word >= 0xD800 && word <= 0xDFFF) {
            return utf32_to_utf16_t{error_code::SURROGATE, buf + k,
                                    utf16_output};
          }
          *utf16_output++ = not match_system(big_endian)
                                ? scalar::u16_swap_bytes(uint16_t(word))
                                : uint16_t(word);
        } else {
          // will generate a surrogate pair
          if (word > 0x10FFFF) {
            return utf32_to_utf16_t{error_code::TOO_LARGE, buf + k,
                                    utf16_output};
          }
          word -= 0x10000;
          uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
          uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
          if (not match_system(big_endian)) {
            high_surrogate = scalar::u16_swap_bytes(high_surrogate);
            low_surrogate = scalar::u16_swap_bytes(low_surrogate);
          }
          *utf16_output++ = char16_t(high_surrogate);
          *utf16_output++ = char16_t(low_surrogate);
        }
      }
      buf += k;
    }
  }

  if (er == ErrorReporting::at_the_end) {
    // check for invalid input
    if (not forbidden_global.is_zero()) {
      return utf32_to_utf16_t{error_code::SURROGATE, buf, utf16_output};
    }
  }

  return utf32_to_utf16_t{error_code::SUCCESS, buf, utf16_output};
}
