/*
    The vectorized algorithm works on single SSE register i.e., it
    loads eight 16-bit code units.

    We consider three cases:
    1. an input register contains no surrogates and each value
       is in range 0x0000 .. 0x07ff.
    2. an input register contains no surrogates and values are
       is in range 0x0000 .. 0xffff.
    3. an input register contains surrogates --- i.e. codepoints
       can have 16 or 32 bits.

    Ad 1.

    When values are less than 0x0800, it means that a 16-bit code unit
    can be converted into: 1) single UTF8 byte (when it is an ASCII
    char) or 2) two UTF8 bytes.

    For this case we do only some shuffle to obtain these 2-byte
    codes and finally compress the whole SSE register with a single
    shuffle.

    We need 256-entry lookup table to get a compression pattern
    and the number of output bytes in the compressed vector register.
    Each entry occupies 17 bytes.

    Ad 2.

    When values fit in 16-bit code units, but are above 0x07ff, then
    a single word may produce one, two or three UTF8 bytes.

    We prepare data for all these three cases in two registers.
    The first register contains lower two UTF8 bytes (used in all
    cases), while the second one contains just the third byte for
    the three-UTF8-bytes case.

    Finally these two registers are interleaved forming eight-element
    array of 32-bit values. The array spans two SSE registers.
    The bytes from the registers are compressed using two shuffles.

    We need 256-entry lookup table to get a compression pattern
    and the number of output bytes in the compressed vector register.
    Each entry occupies 17 bytes.


    To summarize:
    - We need two 256-entry tables that have 8704 bytes in total.
*/

// Auxiliary procedure used by UTF-16 and UTF-32 into UTF-8.
// Note the pointer is passed by reference, it is updated by the procedure.
template <typename T>
simdutf_really_inline void ppc64_convert_utf16_to_1_2_3_bytes_of_utf8(
    const vector_u16 in, uint16_t one_byte_bitmask,
    const T one_or_two_bytes_bytemask, uint16_t one_or_two_bytes_bitmask,
    char *&utf8_output) {
  // case: code units from register produce either 1, 2 or 3 UTF-8 bytes
#if SIMDUTF_IS_BIG_ENDIAN
  const auto dup_lsb =
      vector_u8(1, 1, 3, 3, 5, 5, 7, 7, 9, 9, 11, 11, 13, 13, 15, 15);
#else
  const auto dup_lsb =
      vector_u8(0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14);
#endif // SIMDUTF_IS_BIG_ENDIAN

  /* In this branch we handle three cases:
     1. [0000|0000|0ccc|cccc] => [0ccc|cccc]                           -
    single UFT-8 byte
     2. [0000|0bbb|bbcc|cccc] => [110b|bbbb], [10cc|cccc]              - two
    UTF-8 bytes
     3. [aaaa|bbbb|bbcc|cccc] => [1110|aaaa], [10bb|bbbb], [10cc|cccc] -
    three UTF-8 bytes

    We expand the input word (16-bit) into two code units (32-bit), thus
    we have room for four bytes. However, we need five distinct bit
    layouts. Note that the last byte in cases #2 and #3 is the same.

    We precompute byte 1 for case #1 and the common byte for cases #2 & #3
    in register t2.

    We precompute byte 1 for case #3 and -- **conditionally** -- precompute
    either byte 1 for case #2 or byte 2 for case #3. Note that they
    differ by exactly one bit.

    Finally from these two code units we build proper UTF-8 sequence, taking
    into account the case (i.e, the number of bytes to write).
  */
  /**
   * Given [aaaa|bbbb|bbcc|cccc] our goal is to produce:
   * t2 => [0ccc|cccc] [10cc|cccc]
   * s4 => [1110|aaaa] ([110b|bbbb] OR [10bb|bbbb])
   */
  // [aaaa|bbbb|bbcc|cccc] => [bbcc|cccc|bbcc|cccc]
  const auto t0 = as_vector_u16(dup_lsb.lookup_16(as_vector_u8(in)));

  // [bbcc|cccc|bbcc|cccc] => [00cc|cccc|0bcc|cccc]
  const auto t1 = t0 & uint16_t(0b0011111101111111);
  // [00cc|cccc|0bcc|cccc] => [10cc|cccc|0bcc|cccc]
  const auto t2 = t1 | uint16_t(0b1000000000000000);

  // in = [aaaa|bbbb|bbcc|cccc]
  // a0 = [0000|0000|0000|aaaa]
  const auto a0 = in.shr<12>();
  // b0 = [aabb|bbbb|cccc|cc00]
  const auto b0 = in.shl<2>();
  // s0 = [00bb|bbbb|00cc|cccc]
  const auto s0 = select(uint16_t(0x3f00), b0, a0);

  // s3 = [11bb|bbbb|1110|aaaa]
  const auto s3 = s0 | uint16_t(0b1100000011100000);

  const auto m0 =
      ~as_vector_u16(one_or_two_bytes_bytemask) & uint16_t(0b0100000000000000);
  const auto s4 = s3 ^ m0;

  // 4. compress 32-bit code units into 1, 2 or 3 bytes -- 2 x shuffle
  const uint16_t mask =
      (one_byte_bitmask & 0x5555) | (one_or_two_bytes_bitmask & 0xaaaa);
  if (mask == 0) {
    // We only have three-byte code units. Use fast path.
#if SIMDUTF_IS_BIG_ENDIAN
    // Lookups produced by scripts/ppc64_convert_utf16_to_utf8.py
    const auto shuffle0 =
        vector_u8(1, 0, 16, 3, 2, 18, 5, 4, 20, 7, 6, 22, 9, 8, 24, 11);
    const auto shuffle1 = vector_u8(10, 26, 13, 12, 28, 15, 14, 30, -1, -1, -1,
                                    -1, -1, -1, -1, -1);
#else
    const auto shuffle0 =
        vector_u8(0, 1, 17, 2, 3, 19, 4, 5, 21, 6, 7, 23, 8, 9, 25, 10);
    const auto shuffle1 = vector_u8(11, 27, 12, 13, 29, 14, 15, 31, -1, -1, -1,
                                    -1, -1, -1, -1, -1);
#endif // SIMDUTF_IS_BIG_ENDIAN
    const auto utf8_0 = shuffle0.lookup_32(as_vector_u8(s4), as_vector_u8(t2));
    const auto utf8_1 = shuffle1.lookup_32(as_vector_u8(s4), as_vector_u8(t2));

    utf8_0.store(utf8_output);
    utf8_output += 16;
    utf8_1.store(utf8_output);
    utf8_output += 8;
    return;
  }

  const uint8_t mask0 = uint8_t(mask);

  const uint8_t *row0 =
      &simdutf::tables::ppc64_utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
  const auto shuffle0 = vector_u8::load(row0 + 1);

  const auto utf8_0 = shuffle0.lookup_32(as_vector_u8(s4), as_vector_u8(t2));
  const uint8_t mask1 = static_cast<uint8_t>(mask >> 8);

  const uint8_t *row1 =
      &simdutf::tables::ppc64_utf16_to_utf8::pack_1_2_3_utf8_bytes[mask1][0];
  const auto shuffle1 = vector_u8::load(row1 + 1) + uint8_t(8);
  const auto utf8_1 = shuffle1.lookup_32(as_vector_u8(s4), as_vector_u8(t2));

  utf8_0.store(utf8_output);
  utf8_output += row0[0];
  utf8_1.store(utf8_output);
  utf8_output += row1[0];
}

struct utf16_to_utf8_t {
  error_code err;
  const char16_t *input;
  char *output;
};

/*
  Returns utf16_to_utf8_t value
  A scalar routine should carry on the conversion of the tail,
  iff there was no error.
*/
template <endianness big_endian>
utf16_to_utf8_t ppc64_convert_utf16_to_utf8(const char16_t *buf, size_t len,
                                            char *utf8_output) {

  const char16_t *end = buf + len;

  const auto v_f800 = vector_u16(0xf800);
  const auto v_d800 = vector_u16(0xd800);
  const size_t safety_margin =
      12; // to avoid overruns, see issue
          // https://github.com/simdutf/simdutf/issues/92

  while (end - buf >= std::ptrdiff_t(16 + safety_margin)) {
    auto in = vector_u16::load(buf);
    if (not match_system(big_endian)) {
      in = in.swap_bytes();
    }
    // a single 16-bit UTF-16 word can yield 1, 2 or 3 UTF-8 bytes
    if (in.is_ascii()) {
      auto nextin = vector_u16::load(buf + vector_u16::ELEMENTS);
      if (not match_system(big_endian)) {
        nextin = nextin.swap_bytes();
      }

      if (nextin.is_ascii()) {
        // 1. pack the bytes
        const auto utf8_packed = vector_u16::pack(in, nextin);
        // 2. store (16 bytes)
        utf8_packed.store(utf8_output);
        // 3. adjust pointers
        buf += 16;
        utf8_output += 16;
        continue; // we are done for this round!
      }

      // next block is not ASCII
      const auto utf8_packed = vector_u16::pack(in, in);
      // 2. store (16 bytes)
      utf8_packed.store(utf8_output);
      // 3. adjust pointers
      buf += 8;
      utf8_output += 8;
      in = nextin;
      // fallback
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

    // 1. Check if there are any surrogate word in the input chunk.
    //    We have also to deal with situation when there is a surrogate word
    //    at the end of a chunk.
    const auto surrogates_bytemask = (in & v_f800) == v_d800;

    // bitmask = 0x0000 if there are no surrogates
    //         = 0xc000 if the last word is a surrogate
    const uint16_t surrogates_bitmask = surrogates_bytemask.to_bitmask();
    // It might seem like checking for surrogates_bitmask == 0xc000 could help.
    // However, it is likely an uncommon occurrence.
    if (surrogates_bitmask == 0x0000) {
      ppc64_convert_utf16_to_1_2_3_bytes_of_utf8(
          in, one_byte_bitmask, one_or_two_bytes_bytemask,
          one_or_two_bytes_bitmask, utf8_output);

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
        uint16_t word = not match_system(big_endian)
                            ? scalar::u16_swap_bytes(buf[k])
                            : buf[k];
        if ((word & 0xFF80) == 0) {
          *utf8_output++ = uint8_t(word);
        } else if ((word & 0xF800) == 0) {
          *utf8_output++ = uint8_t((word >> 6) | 0b11000000);
          *utf8_output++ = uint8_t((word & 0b111111) | 0b10000000);
        } else if ((word & 0xF800) != 0xD800) {
          *utf8_output++ = uint8_t((word >> 12) | 0b11100000);
          *utf8_output++ = uint8_t(((word >> 6) & 0b111111) | 0b10000000);
          *utf8_output++ = uint8_t((word & 0b111111) | 0b10000000);
        } else {
          // must be a surrogate pair
          uint16_t diff = uint16_t(word - 0xD800);
          uint16_t next_word = not match_system(big_endian)
                                   ? scalar::u16_swap_bytes(buf[k + 1])
                                   : buf[k + 1];
          k++;
          uint16_t diff2 = uint16_t(next_word - 0xDC00);
          if ((diff | diff2) > 0x3FF) {
            return utf16_to_utf8_t{error_code::SURROGATE, buf + k - 1,
                                   utf8_output};
          }
          uint32_t value = (diff << 10) + diff2 + 0x10000;
          *utf8_output++ = uint8_t((value >> 18) | 0b11110000);
          *utf8_output++ = uint8_t(((value >> 12) & 0b111111) | 0b10000000);
          *utf8_output++ = uint8_t(((value >> 6) & 0b111111) | 0b10000000);
          *utf8_output++ = uint8_t((value & 0b111111) | 0b10000000);
        }
      }
      buf += k;
    }
  } // while

  return utf16_to_utf8_t{error_code::SUCCESS, buf, utf8_output};
}
