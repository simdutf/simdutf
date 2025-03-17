struct utf32_to_utf8_t {
  error_code err;
  const char32_t *input;
  char *output;
};

template <ErrorReporting er>
utf32_to_utf8_t ppc64_convert_utf32_to_utf8(const char32_t *buf, size_t len,
                                            char *utf8_output) {
  const char32_t *end = buf + len;

  const auto v_f800 = vector_u16::splat(0xf800);
  const auto v_d800 = vector_u16::splat(0xd800);

  const auto v_ffff0000 = vector_u32::splat(0xffff0000);
  const auto v_00000000 = vector_u32::zero();
  auto forbidden_bytemask = simd16<bool>();
  const size_t safety_margin =
      12; // to avoid overruns, see issue
          // https://github.com/simdutf/simdutf/issues/92

  while (end - buf >=
         std::ptrdiff_t(
             16 + safety_margin)) { // buf is a char32_t pointer, each char32_t
                                    // has 4 bytes or 32 bits, thus buf + 16 *
                                    // char_32t = 512 bits = 64 bytes
    // We load two 16 bytes registers for a total of 32 bytes or 16 characters.
    // These two values can hold only 8 UTF32 chars
    auto in0 = vector_u32::load(buf);
    auto in1 = vector_u32::load(buf + vector_u32::ELEMENTS);

    // Pack 32-bit UTF-32 code units to 16-bit UTF-16 code units with unsigned
    // saturation
    auto in = vector_u32::pack(in0, in1);

    // Try to apply UTF-16 => UTF-8 from ./ppc64_convert_utf16_to_utf8.cpp

    // Check for ASCII fast path

    // ASCII fast path!!!!
    // We eagerly load another 32 bytes, hoping that they will be ASCII too.
    // The intuition is that we try to collect 16 ASCII characters which
    // requires a total of 64 bytes of input. If we fail, we just pass thirdin
    // and fourthin as our new inputs.
    if (in.is_ascii()) { // if the first two blocks are ASCII
      const auto in2 = vector_u32::load(buf + 2 * vector_u32::ELEMENTS);
      const auto in3 = vector_u32::load(buf + 3 * vector_u32::ELEMENTS);

      const auto next = vector_u32::pack(in2, in3);
      if (next.is_ascii()) {
        // 1. pack the bytes
        const auto utf8_packed = vector_u16::pack(in, next);
        // 2. store (16 bytes)
        utf8_packed.store(utf8_output);
        // 3. adjust pointers
        buf += 16;
        utf8_output += 16;
        continue; // we are done for this round!
      }

      // `next` is not ASCII, write `in` and carry on with next

      // 1. pack the bytes
      const auto utf8_packed = vector_u16::pack(in, in);
      utf8_packed.store(utf8_output);
      // 3. adjust pointers
      buf += 8;
      utf8_output += 8;

      // Proceed with next input
      in = next;
      in0 = in2;
      in1 = in3;
    }

    // no bits set above 7th bit
    const auto one_byte_bytemask = in < uint16_t(1 << 7);
    const uint16_t one_byte_bitmask = one_byte_bytemask.to_bitmask();

    // no bits set above 11th bit
    const auto one_or_two_bytes_bytemask = in < uint16_t(1 << 11);
    const uint16_t one_or_two_bytes_bitmask =
        one_or_two_bytes_bytemask.to_bitmask();

    if (one_or_two_bytes_bitmask == 0xffff) {
      write_v_u16_11bits_to_utf8(
          in, utf8_output, as_vector_u8(one_byte_bytemask), one_byte_bitmask);
      buf += 8;
      continue;
    }

    // Check for overflow in packing
    const auto saturation_bytemask = ((in0 | in1) & v_ffff0000) == v_00000000;
    const uint16_t saturation_bitmask = saturation_bytemask.to_bitmask();
    if (saturation_bitmask == 0xffff) {
      switch (er) {
      case ErrorReporting::precise: {
        const auto forbidden = (in & v_f800) == v_d800;
        if (forbidden.any()) {
          // We return no error code, instead we force the scalar procedure
          // to rescan the portion of input where we've just found an error.
          return utf32_to_utf8_t{error_code::SUCCESS, buf, utf8_output};
        }
      } break;
      case ErrorReporting::at_the_end:
        forbidden_bytemask |= (in & v_f800) == v_d800;
        break;
      case ErrorReporting::none:
        break;
      }

      ppc64_convert_utf16_to_1_2_3_bytes_of_utf8(
          in, one_byte_bitmask, one_or_two_bytes_bytemask,
          one_or_two_bytes_bitmask, utf8_output);
      buf += 8;
    } else {
      // case: at least one 32-bit word produce a surrogate pair in UTF-16 <=>
      // will produce four UTF-8 bytes Let us do a scalar fallback. It may seem
      // wasteful to use scalar code, but being efficient with SIMD in the
      // presence of surrogate pairs may require non-trivial tables.
      size_t forward = 15;
      size_t k = 0;
      if (size_t(end - buf) < forward + 1) {
        forward = size_t(end - buf - 1);
      }
      for (; k < forward; k++) {
        uint32_t word = buf[k];
        if ((word & 0xFFFFFF80) == 0) {
          *utf8_output++ = char(word);
        } else if ((word & 0xFFFFF800) == 0) {
          *utf8_output++ = char((word >> 6) | 0b11000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        } else if ((word & 0xFFFF0000) == 0) {
          if (er != ErrorReporting::none and
              (word >= 0xD800 && word <= 0xDFFF)) {
            return utf32_to_utf8_t{error_code::SURROGATE, buf + k, utf8_output};
          }
          *utf8_output++ = char((word >> 12) | 0b11100000);
          *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        } else {
          if (er != ErrorReporting::none and (word > 0x10FFFF)) {
            return utf32_to_utf8_t{error_code::TOO_LARGE, buf + k, utf8_output};
          }
          *utf8_output++ = char((word >> 18) | 0b11110000);
          *utf8_output++ = char(((word >> 12) & 0b111111) | 0b10000000);
          *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        }
      }
      buf += k;
    }
  } // while

  if (er == ErrorReporting::at_the_end) {
    if (forbidden_bytemask.any()) {
      return utf32_to_utf8_t{error_code::SURROGATE, buf, utf8_output};
    }
  }

  return utf32_to_utf8_t{
      error_code::SUCCESS,
      buf,
      utf8_output,
  };
}
