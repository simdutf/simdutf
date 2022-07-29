/*
    The vectorized algorithm works on single SSE register i.e., it
    loads eight 16-bit words.

    We consider three cases:
    1. an input register contains no surrogates and each value
       is in range 0x0000 .. 0x07ff.
    2. an input register contains no surrogates and values are
       is in range 0x0000 .. 0xffff.
    3. an input register contains surrogates --- i.e. codepoints
       can have 16 or 32 bits.

    Ad 1.

    When values are less than 0x0800, it means that a 16-bit words
    can be converted into: 1) single UTF8 byte (when it's an ASCII
    char) or 2) two UTF8 bytes.

    For this case we do only some shuffle to obtain these 2-byte
    codes and finally compress the whole SSE register with a single
    shuffle.

    We need 256-entry lookup table to get a compression pattern
    and the number of output bytes in the compressed vector register.
    Each entry occupies 17 bytes.

    Ad 2.

    When values fit in 16-bit words, but are above 0x07ff, then
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
/*
  Returns a pair: the first unprocessed byte from buf and utf8_output
  A scalar routing should carry on the conversion of the tail.
*/
template <endianness big_endian>
std::pair<const char16_t*, char32_t*> arm_convert_utf16_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_out) {
  uint32_t * utf32_output = reinterpret_cast<uint32_t*>(utf32_out);
  const char16_t* end = buf + len;

  const uint16x8_t v_f800 = vmovq_n_u16((uint16_t)0xf800);
  const uint16x8_t v_d800 = vmovq_n_u16((uint16_t)0xd800);

  while (buf + 16 <= end) {
    uint16x8_t in = vld1q_u16(reinterpret_cast<const uint16_t *>(buf));
    if (big_endian) {
      #ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
      const uint8x16_t swap = make_uint8x16_t(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
      #else
      const uint8x16_t swap = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14};
      #endif
      in = vreinterpretq_u16_u8(vqtbl1q_u8(vreinterpretq_u8_u16(in), swap));
    }

    const uint16x8_t surrogates_bytemask = vceqq_u16(vandq_u16(in, v_f800), v_d800);
    // It might seem like checking for surrogates_bitmask == 0xc000 could help. However,
    // it is likely an uncommon occurrence.
      if (vmaxvq_u16(surrogates_bytemask) == 0) {
      // case: no surrogate pairs, extend all 16-bit words to 32-bit words
      vst1q_u32(utf32_output,  vmovl_u16(vget_low_u16(in)));
      vst1q_u32(utf32_output+4,  vmovl_high_u16(in));
      utf32_output += 8;
      buf += 8;
    // surrogate pair(s) in a register
    } else {
      // Let us do a scalar fallback.
      // It may seem wasteful to use scalar code, but being efficient with SIMD
      // in the presence of surrogate pairs may require non-trivial tables.
      size_t forward = 15;
      size_t k = 0;
      if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
      for(; k < forward; k++) {
        uint16_t word = big_endian ? scalar::utf16::swap_bytes(buf[k]) : buf[k];
        if((word &0xF800 ) != 0xD800) {
          *utf32_output++ = char32_t(word);
        } else {
          // must be a surrogate pair
          uint16_t diff = uint16_t(word - 0xD800);
          uint16_t next_word = big_endian ? scalar::utf16::swap_bytes(buf[k + 1]) : buf[k + 1];
          k++;
          uint16_t diff2 = uint16_t(next_word - 0xDC00);
          if((diff | diff2) > 0x3FF)  { return std::make_pair(nullptr, reinterpret_cast<char32_t*>(utf32_output)); }
          uint32_t value = (diff << 10) + diff2 + 0x10000;
          *utf32_output++ = char32_t(value);
        }
      }
      buf += k;
    }
  } // while
  return std::make_pair(buf, reinterpret_cast<char32_t*>(utf32_output));
}
