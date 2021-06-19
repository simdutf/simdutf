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


    Summarize:
    - We need two 256-entry tables that have 8704 bytes in total.
*/

/*
  Returns a pair: the first unprocessed byte from buf and utf8_output
  A scalar routing should carry on the conversion of the tail.
*/
std::pair<const char16_t*, char*> sse_convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) {


  const char16_t* end = buf + len;

  const uint16x8_t v_0000 = vmovq_n_u16(0);
  const uint16x8_t v_f800 = vmovq_n_u16((uint16_t)0xf800);
  const uint16x8_t v_d800 = vmovq_n_u16((uint16_t)0xd800);
  const uint16x8_t v_c080 = vmovq_n_u16((uint16_t)0xc080);

  while (buf + 16 <= end) {

    uint16x8_t in = vld1q_u16(buf);

    if(vmaxvq_u16(in) <= 0x7F) { // ASCII fast path!!!!
        // It is common enough that we have sequences of 16 consecutive ASCII characters.
        uint16x8_t nextin = vld1q_u16(buf);
        if(vmaxvq_u16(nextin) > 0x7F) {
          // 1. pack the bytes
          // obviously suboptimal.
          uint8x8_t utf8_packed = vmovn_u16(in);
          // 2. store (8 bytes)
          vst1_u8(utf8_output, utf8_packed);
          // 3. adjust pointers
          buf += 8;
          utf8_output += 8;
          in = nextin;
        } else {
          // 1. pack the bytes
          // obviously suboptimal.
          uint8x16_t utf8_packed = vmovn_high_u16(vmovn_u16(in), nextin);
          // 2. store (16 bytes)
          vst1q_u8(utf8_output, utf8_packed);
          // 3. adjust pointers
          buf += 16;
          utf8_output += 16;
          continue; // we are done for this round!
        }
    }

    if (vmaxvq_u16(in) <= 0x7FF) {
          // 1. prepare 2-byte values
          // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
          // expected output   : [110a|aaaa|10bb|bbbb] x 8
          const uint16x8_t v_1f00 = vmovq_n_u16((int16_t)0x1f00);
          const uint16x8_t v_003f = vmovq_n_u16((int16_t)0x003f);

          // t0 = [000a|aaaa|bbbb|bb00]
          const uint16x8_t t0 = vshlq_u16(in, 2);
          // t1 = [000a|aaaa|0000|0000]
          const uint16x8_t t1 = vandq_u16(t0, v_1f00);
          // t2 = [0000|0000|00bb|bbbb]
          const uint16x8_t t2 = vandq_u16(in, v_003f);
          // t3 = [000a|aaaa|00bb|bbbb]
          const uint16x8_t t3 = vorrq_u16(t1, t2);
          // t4 = [110a|aaaa|10bb|bbbb]
          const uint16x8_t t4 = vorrq_u16(t3, v_c080);

          // 2. merge ASCII and 2-byte codewords
          const uint16x8_t v_007f = vmovq_n_u16((uint16_t)0x007F);
          const uint16x8_t one_byte_bytemask = vcleq_u16(in, v_007f);
          const uint8x16_t utf8_unpacked = vreinterpretq_u8_u16(vbslq_u16(t4, in, one_byte_bytemask));

          // 3. prepare bitmask for 8-bit lookup


          const uint16x8_t mask = { 0x0001, 0x0010, 
                                    0x0002, 0x0020, 
                                    0x0004, 0x0040, 
                                    0x0008, 0x0080};
          uint16_t m2 = vaddvq_u16(vandq_u16(one_byte_bytemask, mask));
          // 4. pack the bytes
          const uint8_t* row = &tables::utf16_to_utf8::pack_1_2_utf8_bytes[m2][0];
          const uint8x16_t shuffle = vld1q_u8(row + 1);
          const uint8x16_t utf8_packed = vqtbl1q_u8(utf8_unpacked, shuffle);

          // 5. store bytes
          vst1q_u8(utf8_output, utf8_packed);

          // 6. adjust pointers
          buf += 8;
          utf8_output += row[0];
          continue;

    }

    // no bits set above 7th bit
    const __m128i one_byte_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in, v_ff80), v_0000);
    const uint16_t one_byte_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_byte_bytemask));

    // no bits set above 11th bit
    const __m128i one_or_two_bytes_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in, v_f800), v_0000);
    const uint16_t one_or_two_bytes_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_or_two_bytes_bytemask));

    // 1. Check if there are any surrogate word in the input chunk.
    //    We have also deal with situation when there is a suggogate word
    //    at the end of a chunk.
    const __m128i surrogates_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in, v_f800), v_d800);

    // bitmask = 0x0000 if there are no surrogates
    //         = 0xc000 if the last word is a surrogate
    const uint16_t surrogates_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(surrogates_bytemask));
    // It might seem like checking for surrogates_bitmask == 0xc000 could help. However,
    // it is likely an uncommon occurrence.
    if (surrogates_bitmask == 0x0000) {
      // case: words from register produce either 1, 2 or 3 UTF-8 bytes
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
        if(mask == 0) {
          // We only have three-byte words. Use fast path.
          const __m128i shuffle = _mm_setr_epi8(2,3,1,6,7,5,10,11,9,14,15,13,-1,-1,-1,-1);
          const __m128i utf8_0 = _mm_shuffle_epi8(out0, shuffle);
          const __m128i utf8_1 = _mm_shuffle_epi8(out1, shuffle);
          _mm_storeu_si128((__m128i*)utf8_output, utf8_0);
          utf8_output += 12;
          _mm_storeu_si128((__m128i*)utf8_output, utf8_1);
          utf8_output += 12;
          buf += 8;
          continue;
        }
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

        buf += 8;
    // surrogate pair(s) in a register
    } else {
      // Let us do a scalar fallback.
      int k = 0;
      for(; k < 15; k++) {
        uint16_t word = buf[k];
        if((word & 0xFF80)==0) {
          *utf8_output++ = char(word);
        } else if((word & 0xF800)==0) {
          *utf8_output++ = char((word>>6) | 0b11000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        } else if((word &0xF800 ) != 0xD800) {
          *utf8_output++ = char((word>>12) | 0b11100000);
          *utf8_output++ = char(((word>>6) & 0b111111) | 0b10000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        } else {
          // must be a surrogate pair
          uint16_t diff = uint16_t(word - 0xD800);
          uint16_t next_word = buf[k+1];
          k++;
          uint16_t diff2 = uint16_t(next_word - 0xDC00);
          if((diff | diff2) > 0x3FF)  { return std::make_pair(nullptr, utf8_output); }
          uint32_t value = (diff << 10) + diff2 + 0x10000;
          *utf8_output++ = char((value>>18) | 0b11110000);
          *utf8_output++ = char(((value>>12) & 0b111111) | 0b10000000);
          *utf8_output++ = char(((value>>6) & 0b111111) | 0b10000000);
          *utf8_output++ = char((value & 0b111111) | 0b10000000);
        }
      }
      buf += k;
    }
  } // while

  return std::make_pair(buf, utf8_output);
}


/*
  Returns a pair: the first unprocessed byte from buf and utf8_output
  A scalar routing should carry on the conversion of the tail.
*/

std::pair<const char16_t*, char*> arm_convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) {

  const char16_t* end = buf + len;

  const uint16x8_t v_0000 = vmovq_n_u16(0);
  const uint16x8_t v_f800 = vmovq_n_u16((uint16_t)0xf800);
  const uint16x8_t v_d800 = vmovq_n_u16((uint16_t)0xd800);
  const uint16x8_t v_c080 = vmovq_n_u16((uint16_t)0xc080);

  while (buf + 8 < end) {

    uint16x8_t in = vld1q_u16(buf);

    if(vmaxvq_u16(in) == 0xffff) { // ASCII fast path!!!!
      // 1. pack the bytes
      // obviously suboptimal.
      const __m128i utf8_packed = _mm_packus_epi16(in,in);
      // 2. store (64 bytes)
      _mm_storeu_si128((__m128i*)utf8_output, utf8_packed);

      // 3. adjust pointers
      buf += 8;
      utf8_output += 8;
      continue; // we are done for this round!
    }
    
    const uint16_t one_byte_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_byte_bytemask));


    // 1. Check if there are any surrogate word in the input chunk.
    //    We have also deal with situation when there is a suggogate word
    //    at the and of chunk.
    const uint16x8_t surrogates_bytemask = vceqq_u16(vandq_u16(in, v_f800), v_d800);

    // Performance note: movmask is expensive under ARM NEON so we simplify the
    // algorithm somewhat.
    if(vmaxvq_u16(surrogates_bytemask) == 0) {

      // a single 16-bit UTF-16 word can yield 1, 2 or 3 UTF-8 bytes
      const uint16x8_t v_ff80 = vmovq_n_u16((uint16_t)0xff80);

      // no bits set above 7th bit
      const uint16x8_t one_byte_bytemask = vceqq_u16(vandq_u32(in, v_ff80), v_0000);
      // no bits set above 11th bit
      const uint16x8_t one_or_two_bytes_bytemask = vceqq_u16(vandq_u32(in, v_f800), v_0000);

      const uint16_t one_byte_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_byte_bytemask));
      const uint16_t one_or_two_bytes_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_or_two_bytes_bytemask));

      // case 1: words from register produce either 1 or 2 UTF-8 bytes
      if ((one_byte_bitmask | one_or_two_bytes_bitmask) == 0xffff) {
          // 1. prepare 2-byte values
          // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
          // expected output   : [110a|aaaa|10bb|bbbb] x 8
          const __m128i v_1f00 = vmovq_n_u16((int16_t)0x1f00);
          const __m128i v_003f = vmovq_n_u16((int16_t)0x003f);

          // t0 = [000a|aaaa|bbbb|bb00]
          const __m128i t0 = _mm_slli_epi16(in, 2);
          // t1 = [000a|aaaa|0000|0000]
          const __m128i t1 = vandq_u32(t0, v_1f00);
          // t2 = [0000|0000|00bb|bbbb]
          const __m128i t2 = vandq_u32(in, v_003f);
          // t3 = [000a|aaaa|00bb|bbbb]
          const __m128i t3 = vorrq_u32(t1, t2);
          // t4 = [110a|aaaa|10bb|bbbb]
          const __m128i t4 = vorrq_u32(t3, v_c080);

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

    }
    // bitmask = 0x0000 if there are no surrogates
    //         = 0xc000 if the last word is a surrogate
    const uint16_t surrogates_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(surrogates_bytemask));

    if ((surrogates_bitmask == 0x0000) or (surrogates_bitmask == 0xc000)) {
      // In case of surrogate on the last position reset its value. Thanks to that
      // it would produce just one output byte and we just trim it.
      in = _mm_andnot_si128(surrogates_bytemask, in);

      // a single 16-bit UTF-16 word can yield 1, 2 or 3 UTF-8 bytes
      const __m128i v_ff80 = vmovq_n_u16((int16_t)0xff80);

      // no bits set above 7th bit
      const __m128i one_byte_bytemask = vceqq_u16(vandq_u32(in, v_ff80), v_0000);
      // no bits set above 11th bit
      const __m128i one_or_two_bytes_bytemask = vceqq_u16(vandq_u32(in, v_f800), v_0000);

      const uint16_t one_byte_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_byte_bytemask));
      const uint16_t one_or_two_bytes_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_or_two_bytes_bytemask));

      // case 1: words from register produce either 1 or 2 UTF-8 bytes
      if ((one_byte_bitmask | one_or_two_bytes_bitmask) == 0xffff) {
          // 1. prepare 2-byte values
          // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
          // expected output   : [110a|aaaa|10bb|bbbb] x 8
          const __m128i v_1f00 = vmovq_n_u16((int16_t)0x1f00);
          const __m128i v_003f = vmovq_n_u16((int16_t)0x003f);

          // t0 = [000a|aaaa|bbbb|bb00]
          const __m128i t0 = _mm_slli_epi16(in, 2);
          // t1 = [000a|aaaa|0000|0000]
          const __m128i t1 = vandq_u32(t0, v_1f00);
          // t2 = [0000|0000|00bb|bbbb]
          const __m128i t2 = vandq_u32(in, v_003f);
          // t3 = [000a|aaaa|00bb|bbbb]
          const __m128i t3 = vorrq_u32(t1, t2);
          // t4 = [110a|aaaa|10bb|bbbb]
          const __m128i t4 = vorrq_u32(t3, v_c080);

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
#define vec(x) vmovq_n_u16(static_cast<uint16_t>(x))
        const __m128i t0 = _mm_shuffle_epi8(in, dup_even);
        const __m128i t1 = vandq_u32(t0, vec(0b0011'1111'0111'1111));
        const __m128i t2 = vorrq_u32 (t1, vec(0b1000'0000'0000'0000));


        const __m128i s0 = _mm_srli_epi16(in, 4);
        const __m128i s1 = vandq_u32(s0, vec(0b0000'1111'1111'1100));
        const __m128i s2 = _mm_maddubs_epi16(s1, vec(0x0140));
        const __m128i s3 = vorrq_u32(s2, vec(0b1100'0000'1110'0000));
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
      const __m128i v_fc00 = vmovq_n_u16(int16_t(0xfc00));
      const __m128i v_dc00 = vmovq_n_u16(int16_t(0xdc00));
      // 1. validate surrogates
      // 1a. non-surrogate words
      const uint16_t V = ~surrogates_bitmask;

      // 1b. obtain mask for high surrogates (0xDC00..0xDFFF)
      const __m128i vH = vceqq_u16(vandq_u32(in, v_fc00), v_dc00);
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
      const __m128i t0         = vandq_u32(in, v_03ff03ff);
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