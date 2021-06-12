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

    Ad 3.

    The input is normalized into 32-bit words, it spans two registers.
    Next the 32-bit words are converted into UCS4. For non-surrogate
    entries there's no need to do anything. In the case of words
    containing surrogate pairs, the RFC-imposed algorithm is used
    (RFC-2781, 2.2 Decoding UTF-16).

    Next, the UCS32 is converted into UTF8 withing 32-bit words. Finally
    the registers are compressed. Then we complete missing bits in UTF8
    bytes.

    We need 256-entry lookup table to perform expansion into UCS4. Each
    entry contains two shuffle patterns (32 bytes). We need another
    256-entry lookup table to get a compression pattern, UTF8 bit patterns,
    and the number of output bytes in the compressed vector register.
    Each entry occupies 33 bytes.

    Summarize:
    - We need four 256-entry tables that have 25344 bytes in total.
*/

int convert_UCS4_to_UTF8(uint32x4_t in, uint8x16_t& out) {
  const uint32x4_t v_80   = vmovq_n_u32(0x00000080);
  const uint32x4_t v_7f   = vmovq_n_u32(0x0000007f);
  const uint32x4_t v_7ff  = vmovq_n_u32(0x000007ff);
  const uint32x4_t v_ffff = vmovq_n_u32(0x0000ffff);

  const uint32x4_t mask_1_byte      = vcgtq_u32 (in, v_80);
  const uint32x4_t mask_2_3_4_bytes = vcgtq_u32(in, v_7f);
  const uint32x4_t mask_3_4_bytes   = vcgtq_u32(in, v_7ff);
  const uint32x4_t mask_4_bytes     = vcgtq_u32(in, v_ffff);

  // 0a. convert surrogate pairs into UCS
  {
      const uint32x4_t v_00010400 = vmovq_n_u32(0x00010400);
      const uint32x4_t v_00010000 = vmovq_n_u32(0x00010000);
      const __m128i t0 = _mm_madd_epi16(in, v_00010400); // join 10-bit words into single 20-bit word
      const __m128i t1 = vaddq_s32 (t0, v_00010000); // add 0x1'0000

      in = _mm_blendv_epi8(in, t1, mask_4_bytes);
  }

  // 1a. shift 6-bit words - step #1
  // 1. [00000000|00000000|00000000|0xxxxxxx]
  // 2. [00000000|00000000|00000yyy|yyxxxxxx]
  // 3. [00000000|00000000|zzzzyyyy|yyxxxxxx]
  // 4. [00000000|0000wwzz|zzzzyyyy|yyxxxxxx]

  const uint32x4_t v_0000003f = vmovq_n_u32(0x0000003f);
  const uint32x4_t v_00003f00 = vmovq_n_u32(0x00003f00);
  const uint32x4_t v_003f0000 = vmovq_n_u32(0x003f0000);
  const uint32x4_t v_0f000000 = vmovq_n_u32(0x0f000000);
  __m128i byte1 = _mm_and_si128(in, v_0000003f);
  __m128i byte2 = vshlq_u32(in, 2);
  byte2 = _mm_and_si128(byte2, v_00003f00); // byte2 = [00000000|00000000|00yyyyyy|00000000]
  __m128i byte3 = vshlq_u32(in, 4);
  byte3 = _mm_and_si128(byte3, v_003f0000); // byte3 = [00000000|00zzzzzz|00000000|00000000]
  __m128i byte4 = vshlq_u32(in, 6);
  byte4 = _mm_and_si128(byte4, v_0f000000); // byte4 = [000000ww|00000000|00000000|00000000]

  // merged = [000000ww|00zzzzzz|00yyyyyy|00xxxxxx] for 2, 3, 4 UTF-8 bytes
  __m128i merged;
  merged = _mm_or_si128(_mm_or_si128(byte1, byte2),
                        _mm_or_si128(byte3, byte4));
  // merged = [00000000|00000000|00000000|0xxxxxxx] for 1 UTF-8 byte

  merged = _mm_blendv_epi8(merged, in, mask_1_byte);

  // Count how many UTF-8 bytes produce each 32-bit word. We are adding three bitmask,
  // thus the addition result will be in range 0..3 (it spans 2 bits), where:
  // * 0 = 0b00 - 1 byte
  // * 1 = 0b01 - 2 bytes
  // * 2 = 0b10 - 3 bytes
  // * 3 = 0b11 - 4 bytes
  // We keep just bits #7 and #14 of the masks. After addition the bit #7 will
  // hold the lower bit of the sum, bit #15 -- the higher bit.
  const uint32x4_t v_00004080      = vmovq_n_u32(0x00004080);
  const __m128i cnt_2_3_4_bytes = _mm_and_si128(mask_2_3_4_bytes, v_00004080);
  const __m128i cnt_3_4_bytes   = _mm_and_si128(mask_3_4_bytes, v_00004080);
  const __m128i cnt_4_bytes     = _mm_and_si128(mask_4_bytes, v_00004080);
  const __m128i count = vaddq_s32(vaddq_s32(cnt_2_3_4_bytes, cnt_3_4_bytes), cnt_4_bytes);

  // Now, when we gather MSB using _mm_movemask_epi8, we obtain a bitmask
  // like this: 00hg00fe00dc00ba, where hg, fe, dc and ba are the two-bit counters.
  const int m0 = _mm_movemask_epi8(count);

  // Compress a sparse 16-bit word bitmask into 8-bit one: hgdcfeba
  const uint8_t mask = static_cast<uint8_t>((m0 >> 6) | m0);

  const auto& to_utf8 = tables::utf16_to_utf8::ucs4_to_utf8[mask];
  // merged = [000000ww|00zzzzzz|00yyyyyy|00xxxxxx]
  //          [00000000|00000000|00000000|0xxxxxxx]
  merged = _mm_or_si128(merged, _mm_loadu_si128((__m128i*)to_utf8.const_bits_mask));

  out = _mm_shuffle_epi8(merged, _mm_loadu_si128((__m128i*)to_utf8.shuffle));

  return to_utf8.output_bytes;
}

#ifdef CUTOFF

/*
  Returns a pair: the first unprocessed byte from buf and utf8_output
  A scalar routing should carry on the conversion of the tail.
*/

std::pair<const char16_t*, char*> sse_convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) {

  const char16_t* end = buf + len;

  const __m128i v_0000 = _mm_setzero_si128();
  const __m128i v_f800 = _mm_set1_epi16((int16_t)0xf800);
  const __m128i v_d800 = _mm_set1_epi16((int16_t)0xd800);
  const __m128i v_c080 = _mm_set1_epi16((int16_t)0xc080);

  while (buf + 8 < end) {

    __m128i in = _mm_loadu_si128((__m128i*)buf);

    // 1. Check if there are any surrogate word in the input chunk.
    //    We have also deal with situation when there is a suggogate word
    //    at the and of chunk.
    const __m128i surrogates_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in, v_f800), v_d800);

    // bitmask = 0x0000 if there are no surrogates
    //         = 0xc000 if the last word is a surrogate
    const uint16_t surrogates_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(surrogates_bytemask));

    if ((surrogates_bitmask == 0x0000) or (surrogates_bitmask == 0xc000)) {
      // In case of surrogate on the last position reset its value. Thanks to that
      // it would produce just one output byte and we just trim it.
      in = _mm_andnot_si128(surrogates_bytemask, in);

      // a single 16-bit UTF-16 word can yield 1, 2 or 3 UTF-8 bytes
      const __m128i v_ff80 = _mm_set1_epi16((int16_t)0xff80);

      // no bits set above 7th bit
      const __m128i one_byte_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in, v_ff80), v_0000);
      // no bits set above 11th bit
      const __m128i one_or_two_bytes_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in, v_f800), v_0000);

      const uint16_t one_byte_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_byte_bytemask));
      const uint16_t one_or_two_bytes_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_or_two_bytes_bytemask));

      // case 1: words from register produce either 1 or 2 UTF-8 bytes
      if ((one_byte_bitmask | one_or_two_bytes_bitmask) == 0xffff) {
          // 1. prepare 2-byte values
          // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
          // expected output   : [110a|aaaa|10bb|bbbb] x 8
          const __m128i v_1f00 = _mm_set1_epi16((int16_t)0x1f00);
          const __m128i v_003f = _mm_set1_epi16((int16_t)0x003f);

          // t0 = [000a|aaaa|bbbb|bb00]
          const __m128i t0 = _mm_slli_epi16(in, 2);
          // t1 = [000a|aaaa|0000|0000]
          const __m128i t1 = _mm_and_si128(t0, v_1f00);
          // t2 = [0000|0000|00bb|bbbb]
          const __m128i t2 = _mm_and_si128(in, v_003f);
          // t3 = [000a|aaaa|00bb|bbbb]
          const __m128i t3 = _mm_or_si128(t1, t2);
          // t4 = [110a|aaaa|10bb|bbbb]
          const __m128i t4 = _mm_or_si128(t3, v_c080);

          // 2. merge ASCII and 2-byte codewords
          const __m128i utf8_unpacked = _mm_blendv_epi8(t4, in, one_byte_bytemask);

          // 3. prepare bitmask for 8-bit lookup
          //    one_byte_bitmask = hhggffeeddccbbaa -- the bits are doubled (h - MSB, a - LSB)
          const uint16_t m0 = one_byte_bitmask & 0x5555;  // m0 = 0h0g0f0e0d0c0b0a
          const uint16_t m1 = m0 >> 7;                    // m1 = 00000000h0g0f0e0
          const uint8_t  m2 = static_cast<uint8_t>((m0 | m1) & 0xff);           // m2 =         hdgcfbea

          // 4. pack the bytes
          const uint8_t* row = &tables::utf16_to_utf8::pack_1_2_utf8_bytes[m2][0];
          const __m128i shuffle = _mm_loadu_si128((__m128i*)(row + 1));
          const __m128i utf8_packed = _mm_shuffle_epi8(utf8_unpacked, shuffle);

          // 5. store bytes
          _mm_storeu_si128((__m128i*)utf8_output, utf8_packed);

          // 6. adjust pointers
          if (surrogates_bitmask == 0x0000) {
            buf += 8;
            utf8_output += row[0];
          } else {
            buf += 7;
            utf8_output += row[0] - 1;
          }
      // case 2: words from register produce either 1, 2 or 3 UTF-8 bytes
      } else {
        const __m128i dup_even = _mm_setr_epi16(0x0000, 0x0202, 0x0404, 0x0606,
                                                0x0808, 0x0a0a, 0x0c0c, 0x0e0e);

        /* In this branch we handle three cases:
           1. [0000|0000|0ccc|cccc] => [0ccc|cccc]                           - single UFT-8 byte
           2. [0000|0bbb|bbcc|cccc] => [110b|bbbb], [10cc|cccc]              - two UTF-8 bytes
           3. [aaaa|bbbb|bbcc|cccc] => [1110|aaaa], [10bb|bbbb], [10cc|cccc] - three UTF-8 bytes

          We expand the input word (16-bit) into two words (32-bit), thus
          we have room for four bytes. However, we need five distinct bit
          layouts. Note that the last byte in cases #2 and #3 is the same.

          We precompute byte 1 for case #1 and the common byte for cases #2 & #3
          in register t2.

          We precomputer byte 1 for case #3 and -- **conditionally** -- precompute
          either byte 1 for case #2 or byte 2 for case #3. Note that they
          differ by exactly one bit.

          Finally from these two words we build proper UTF-8 sequence, taking
          into account the case (i.e, the number of bytes to write).
        */
#define vec(x) _mm_set1_epi16(static_cast<uint16_t>(x))
        const __m128i t0 = _mm_shuffle_epi8(in, dup_even);
        const __m128i t1 = _mm_and_si128(t0, vec(0b0011'1111'0111'1111));
        const __m128i t2 = _mm_or_si128 (t1, vec(0b1000'0000'0000'0000));


        const __m128i s0 = _mm_srli_epi16(in, 4);
        const __m128i s1 = _mm_and_si128(s0, vec(0b0000'1111'1111'1100));
        const __m128i s2 = _mm_maddubs_epi16(s1, vec(0x0140));
        const __m128i s3 = _mm_or_si128(s2, vec(0b1100'0000'1110'0000));
        const __m128i m0 = _mm_andnot_si128(one_or_two_bytes_bytemask, vec(0b0100'0000'0000'0000));
        const __m128i s4 = _mm_xor_si128(s3, m0);
#undef vec

        // 4. expand words 16-bit => 32-bit
        const __m128i out0 = _mm_unpacklo_epi16(t2, s4);
        const __m128i out1 = _mm_unpackhi_epi16(t2, s4);

        // 5. compress 32-bit words into 1, 2 or 3 bytes -- 2 x shuffle
        const uint16_t mask = (one_byte_bitmask & 0x5555) |
                              (one_or_two_bytes_bitmask & 0xaaaa);

        const uint8_t mask0 = uint8_t(mask);

        const uint8_t* row0 = &tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
        const __m128i shuffle0 = _mm_loadu_si128((__m128i*)(row0 + 1));
        const __m128i utf8_0 = _mm_shuffle_epi8(out0, shuffle0);

        const uint8_t mask1 = static_cast<uint8_t>(mask >> 8);
        const uint8_t* row1 = &tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask1][0];
        const __m128i shuffle1 = _mm_loadu_si128((__m128i*)(row1 + 1));
        const __m128i utf8_1 = _mm_shuffle_epi8(out1, shuffle1);

        _mm_storeu_si128((__m128i*)utf8_output, utf8_0);
        utf8_output += row0[0];
        _mm_storeu_si128((__m128i*)utf8_output, utf8_1);
        utf8_output += row1[0];

        if (surrogates_bitmask == 0x0000) {
          buf += 8;
        } else {
          buf += 7;
          utf8_output -= 1;
        }
      }
    // surrogate pair(s) in a register
    } else {
      ///// copy of validation from sse_validate_utf16le.cpp
      const __m128i v_fc00 = _mm_set1_epi16(int16_t(0xfc00));
      const __m128i v_dc00 = _mm_set1_epi16(int16_t(0xdc00));
      // 1. validate surrogates
      // 1a. non-surrogate words
      const uint16_t V = ~surrogates_bitmask;

      // 1b. obtain mask for high surrogates (0xDC00..0xDFFF)
      const __m128i vH = _mm_cmpeq_epi16(_mm_and_si128(in, v_fc00), v_dc00);
      const uint16_t H = static_cast<uint16_t>(_mm_movemask_epi8(vH));

      // 1c. obtain mask for log surrogates (0xD800..0xDBFF)
      const uint16_t L = static_cast<uint16_t>(~H & surrogates_bitmask);

      const uint16_t a = L & (H >> 2);
      const uint16_t b = a << 2;
      const uint16_t c = V | a | b;
      ///// end of copy

      // bail out if input is invalid
      if ((c != 0xffff) and (c != 0x3fff))
        return std::make_pair(nullptr, utf8_output);

      // 3. keep 10 lowest bits from surrogate pairs having actual data
      const uint32x4_t v_03ff03ff = vmovq_n_u32(0x03ff03ff);
      const __m128i t0         = _mm_and_si128(in, v_03ff03ff);
      const __m128i in1        = _mm_blendv_epi8(in, t0, surrogates_bytemask);

      // 2. use surrogates_bitmask to obtain shuffle pattern
      //    we expand 16-bit words into 32-bit words
      //
      //    surrogates_bitmask = hhggffeeddccbbaa
      //    expansion_id       = 0h0g0f0e0d0c0b0a
      //                       |        0h0g0f0e0
      //                       =        ehdgcfbea
      const uint8_t expansion_id = static_cast<uint8_t>((surrogates_bitmask & 0x5555) | ((surrogates_bitmask >> 7) & 0xaaaa));

      const __m128i shuffle_lo = _mm_loadu_si128((__m128i*)&tables::utf16_to_utf8::expand_surrogates[expansion_id][0]);
      const __m128i shuffle_hi = _mm_loadu_si128((__m128i*)&tables::utf16_to_utf8::expand_surrogates[expansion_id][16]);

      const __m128i expanded_lo = _mm_shuffle_epi8(in1, shuffle_lo);
      const __m128i expanded_hi = _mm_shuffle_epi8(in1, shuffle_hi);

      // [00000000|00000000|00000000|0xxxxxxx] --- 1 UTF-8 byte  (0x0000 .. 0x007F)
      // [00000000|00000000|00000yyy|yyxxxxxx] --- 2 UTF-8 bytes (0x0080 .. 0x07FF)
      // [00000000|00000000|zzzzyyyy|yyxxxxxx] --- 3 UTF-8 bytes (0x0800 .. 0xFFFF)
      // [00000000|0000bbbb|bbbbbbaa|aaaaaaaa] --- 4 UTF-8 bytes 2 x surrogate words (0x1'0000 ... 0xF'FFFF)

      __m128i utf8_lo;
      __m128i utf8_hi;

      const int utf8_lo_bytes = convert_UCS4_to_UTF8(expanded_lo, utf8_lo);
      const int utf8_hi_bytes = convert_UCS4_to_UTF8(expanded_hi, utf8_hi);

      _mm_storeu_si128((__m128i*)utf8_output, utf8_lo);
      utf8_output += utf8_lo_bytes;
      _mm_storeu_si128((__m128i*)utf8_output, utf8_hi);
      utf8_output += utf8_hi_bytes;

      utf8_output -= tables::utf16_to_utf8::expand_surrogates[expansion_id][32];

      if (c == 0xffff)
        buf += 8;
      else
        buf += 7;
    }
  } // while

  return std::make_pair(buf, utf8_output);
}

#endif