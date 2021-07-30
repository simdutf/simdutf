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
using vecu8 = __vector uint8_t;
using vecu16 = __vector uint16_t;
using vecu32 = __vector uint32_t;

std::pair<const char16_t*, char*> ppc64_convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_out) {

  const char16_t* end = buf + len;

  const vecu16 v_0000 = vec_splat_u16(0);
  const vecu16 v_f800 = vec_splat_u16(0xf800);
  const vecu16 v_d800 = vec_splat_u16(0xd800);
  const vecu16 v_c080 = vec_splat_u16(0xc080);
  while (buf + 16 <= end) {
    vecu16 in = (vecu16)(vec_vsx_ld(0, reinterpret_cast<const uint16_t *>(buf)));

    // a single 16-bit UTF-16 word can yield 1, 2 or 3 UTF-8 bytes
    const vecu8 v_ff80 = vec_splat_u16(0xff80);
    if(vec_all_lt(in, v_ff80)) { // ASCII fast path!!!!
        vecu8 nextin = (vecu8)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(buf)+1));
        if(!vec_all_lt(nextin, v_ff80)) {
          // 1. pack the bytes
          // obviously suboptimal.
          const vecu8 utf8_packed = vec_pack(in,in);
          // 2. store (16 bytes)
          vec_vsx_st(utf8_packed, 0, reinterpret_cast<vecu8 *>(utf8_output));
          // 3. adjust pointers
          buf += 8;
          utf8_output += 8;
          in = nextin;
        } else {
          // 1. pack the bytes
          // obviously suboptimal.
          const vecu8 utf8_packed = vec_pack(in,nextin);
          // 2. store (16 bytes)
          vec_vsx_st(utf8_packed, 0, reinterpret_cast<vecu8 *>(utf8_output));
          // 3. adjust pointers
          buf += 16;
          utf8_output += 16;
          continue; // we are done for this round!
        }
    }
    const vecu8 perm_mask = {0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x40,
                               0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 0x08, 0x00};


    // no bits set above 7th bit
    const vecu8 one_byte_bytemask = vec_cmpeq(vec_and(in, v_ff80), v_0000);
#ifdef __LITTLE_ENDIAN__
    const uint16_t one_byte_bitmask = static_cast<uint16_t>((vecu32)vec_vbpermq(one_byte_bytemask,perm_mask)[1]);
#else
    const uint16_t one_byte_bitmask = static_cast<uint16_t>((vecu32)vec_vbpermq(one_byte_bytemask,perm_mask)[0]);
#endif

    // no bits set above 11th bit
    const vecu8 one_or_two_bytes_bytemask = vec_cmpeq(vec_and(in, v_f800), v_0000);
#ifdef __LITTLE_ENDIAN__
    const uint16_t one_or_two_bytes_bitmask = static_cast<uint16_t>((vecu32)vec_vbpermq(one_or_two_bytes_bytemask,perm_mask)[1]);
#else
    const uint16_t one_or_two_bytes_bitmask = static_cast<uint16_t>((vecu32)vec_vbpermq(one_or_two_bytes_bytemask,perm_mask)[0]);
#endif

    if (one_or_two_bytes_bitmask == 0xffff) {
          // 1. prepare 2-byte values
          // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
          // expected output   : [110a|aaaa|10bb|bbbb] x 8
          const vecu8 v_1f00 = vec_splat_u16((int16_t)0x1f00);
          const vecu16 v_003f = vec_splat_u16((int16_t)0x003f);

          // t0 = [000a|aaaa|bbbb|bb00]
          const vecu16 t0 = vec_sll(in, 2);
          // t1 = [000a|aaaa|0000|0000]
          const vecu16 t1 = vec_and(t0, v_1f00);
          // t2 = [0000|0000|00bb|bbbb]
          const vecu16 t2 = vec_and(in, v_003f);
          // t3 = [000a|aaaa|00bb|bbbb]
          const vecu16 t3 = vec_or(t1, t2);
          // t4 = [110a|aaaa|10bb|bbbb]
          const vecu16 t4 = vec_or(t3, v_c080);

          // 2. merge ASCII and 2-byte codewords
          const vecu16 v_007f = vec_splat_u16(0x007F);
          const vecu16 one_byte_bytemask = vec_cmple(in, v_007f);
          const vecu16 utf8_unpacked = vec_sel(one_byte_bytemask, in, t4);// check order

          // 3. prepare bitmask for 8-bit lookup
          //    one_byte_bitmask = hhggffeeddccbbaa -- the bits are doubled (h - MSB, a - LSB)
          const uint16_t m0 = one_byte_bitmask & 0x5555;  // m0 = 0h0g0f0e0d0c0b0a
          const uint16_t m1 = static_cast<uint16_t>(m0 >> 7);                    // m1 = 00000000h0g0f0e0
          const uint8_t  m2 = static_cast<uint8_t>((m0 | m1) & 0xff);           // m2 =         hdgcfbea
          // 4. pack the bytes
          const uint8_t* row = &simdutf::tables::utf16_to_utf8::pack_1_2_utf8_bytes[m2][0];
          const vecu8 shuffle = (vecu8)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(row + 1)));

          const vecu8 utf8_packed = vec_perm(utf8_unpacked, utf8_unpacked, shuffle);

          // 5. store bytes
          vec_vsx_st(utf8_packed, 0, reinterpret_cast<vecu8 *>(utf8_output));

          // 6. adjust pointers
          buf += 8;
          utf8_output += row[0];
          continue;

    }

    // 1. Check if there are any surrogate word in the input chunk.
    //    We have also deal with situation when there is a suggogate word
    //    at the end of a chunk.
    const vecu16 surrogates_bytemask = vec_cmpeq(vec_and(in, v_f800), v_d800);

    // bitmask = 0x0000 if there are no surrogates
    //         = 0xc000 if the last word is a surrogate
#ifdef __LITTLE_ENDIAN__
    const uint16_t surrogates_bitmask = static_cast<uint16_t>((vecu32)vec_vbpermq(surrogates_bytemask,perm_mask)[1]);
#else
    const uint16_t surrogates_bitmask = static_cast<uint16_t>((vecu32)vec_vbpermq(surrogates_bytemask,perm_mask)[0]);
#endif
    // It might seem like checking for surrogates_bitmask == 0xc000 could help. However,
    // it is likely an uncommon occurrence.
    if (surrogates_bitmask == 0x0000) {
      // case: words from register produce either 1, 2 or 3 UTF-8 bytes
        const vecu16 dup_even = {0x0000, 0x0202, 0x0404, 0x0606,
                                0x0808, 0x0a0a, 0x0c0c, 0x0e0e};

        /* In this branch we handle three cases:
           1. [0000|0000|0ccc|cccc] => [0ccc|cccc]                           - single UFT-8 byte
           2. [0000|0bbb|bbcc|cccc] => [110b|bbbb], [10cc|cccc]              - two UTF-8 bytes
           3. [aaaa|bbbb|bbcc|cccc] => [1110|aaaa], [10bb|bbbb], [10cc|cccc] - three UTF-8 bytes

          We expand the input word (16-bit) into two words (32-bit), thus
          we have room for four bytes. However, we need five distinct bit
          layouts. Note that the last byte in cases #2 and #3 is the same.

          We precompute byte 1 for case #1 and the common byte for cases #2 & #3
          in register t2.

          We precompute byte 1 for case #3 and -- **conditionally** -- precompute
          either byte 1 for case #2 or byte 2 for case #3. Note that they
          differ by exactly one bit.

          Finally from these two words we build proper UTF-8 sequence, taking
          into account the case (i.e, the number of bytes to write).
        */
        /**
         * Given [aaaa|bbbb|bbcc|cccc] our goal is to produce:
         * t2 => [0ccc|cccc] [10cc|cccc]
         * s4 => [1110|aaaa] ([110b|bbbb] OR [10bb|bbbb])
         */
#define vec(x) vec_splat_u16(static_cast<uint16_t>(x))
        // [aaaa|bbbb|bbcc|cccc] => [bbcc|cccc|bbcc|cccc]
        const vecu8 t0 = vec_perm(in, in, dup_even);
        // [bbcc|cccc|bbcc|cccc] => [00cc|cccc|0bcc|cccc]
        const vecu16 t1 = vec_and(t0, vec(0b0011111101111111));
        // [00cc|cccc|0bcc|cccc] => [10cc|cccc|0bcc|cccc]
        const vecu16 t2 = vec_or (t1, vec(0b1000000000000000));
        // s0: [aaaa|bbbb|bbcc|cccc] => [0000|0000|0000|aaaa]
        const vecu16 s0 = vec_srl(in, 12);
        // s1: [aaaa|bbbb|bbcc|cccc] => [0000|bbbb|bb00|0000]
        const vecu16 s1 = vec_and(in, vec(0b0000111111000000));
        // [0000|bbbb|bb00|0000] => [00bb|bbbb|0000|0000]
        const vecu16 s1s = vec_srl(s1, 2);
        // [00bb|bbbb|0000|aaaa]
        const vecu16 s2 = vec_or(s0, s1s);
        // s3: [00bb|bbbb|0000|aaaa] => [11bb|bbbb|1110|aaaa]
        const vecu16 s3 = vec_or(s2, vec(0b1100000011100000));
        const vecu16 v_07ff = vec_splat_u16((uint16_t)0x07FF);
        const vecu16 one_or_two_bytes_bytemask = vec_cmple(in, v_07ff);
        const vecu16 m0 = vac_andc(vec(0b0100000000000000), one_or_two_bytes_bytemask);
        const vecu16 s4 = vec_xor(s3, m0);
#undef vec

        // 4. expand words 16-bit => 32-bit
        const vecu16 out0 = vec_unpackl(t2, s4);
        const vecu16 out1 = vec_unpackh(t2, s4);

        // 5. compress 32-bit words into 1, 2 or 3 bytes -- 2 x shuffle
        const uint16_t mask = (one_byte_bitmask & 0x5555) |
                              (one_or_two_bytes_bitmask & 0xaaaa);
        if(mask == 0) {
          // We only have three-byte words. Use fast path.
          const vecu16 shuffle = {2,3,1,6,7,5,10,11,9,14,15,13,-1,-1,-1,-1};
          const vecu16 utf8_0 = vec_perm(out0, out0, shuffle);
          const vecu8 utf8_1 = vec_perm(out1, out1, shuffle);
          vec_vsx_st(utf8_0, 0, reinterpret_cast<vecu8 *>(utf8_output));
          utf8_output += 12;
          vec_vsx_st(utf8_1, 0, reinterpret_cast<vecu8 *>(utf8_output));
          utf8_output += 12;
          buf += 8;
          continue;
        }
        const uint8_t mask0 = uint8_t(mask);

        const uint8_t* row0 = &simdutf::tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
        const vecu16 shuffle0 = (vecu16)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(row0 + 1)));

        const vecu16 utf8_0 = vec_perm(out0, out0, shuffle0);

        const uint8_t mask1 = static_cast<uint8_t>(mask >> 8);

        const uint8_t* row1 = &simdutf::tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask1][0];
        const vecu8 shuffle1 = (vecu8)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(row1 + 1)));

        const vecu8 utf8_1 = vec_perm(out1, out1, shuffle1);
        vec_vsx_st(utf8_0, 0, reinterpret_cast<vecu8 *>(utf8_output));
        utf8_output += row0[0];
        vec_vsx_st(utf8_1, 0, reinterpret_cast<vecu8 *>(utf8_output));
        utf8_output += row1[0];

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
