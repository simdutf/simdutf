std::pair<const char32_t*, char*> sse_convert_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) {

  const char32_t* end = buf + len;

  const __m128i v_0000 = _mm_setzero_si128();
  const __m128i v_f800 = _mm_set1_epi16((int16_t)0xf800);
  const __m128i v_c080 = _mm_set1_epi16((int16_t)0xc080);
  const __m128i v_ff80 = _mm_set1_epi16((int16_t)0xff80);
  const __m128i v_ffff0000 = _mm_set1_epi32((int32_t)0xffff0000);

  while (buf + 16 <= end) {
    __m128i in = _mm_loadu_si128((__m128i*)buf);
    __m128i nextin = _mm_loadu_si128((__m128i*)buf+1);
    // Pack 32-bit UTF-32 words to 16-bit UTF-16 words with unsigned saturation
    __m128i in_16 = _mm_packus_epi32(in, nextin);
    
    // Try to apply UTF-16 => UTF-8 from ./sse_convert_utf16_to_utf8.cpp
    
    // Check for ASCII fast path
    if(_mm_testz_si128(in_16, v_ff80)) { // ASCII fast path!!!!
      __m128i thirdin = _mm_loadu_si128((__m128i*)buf+2);
      __m128i fourthin = _mm_loadu_si128((__m128i*)buf+3);
      __m128i nextin_16 = _mm_packus_epi32(thirdin, fourthin);
      if(!_mm_testz_si128(nextin_16, v_ff80)) {
        // 1. pack the bytes
        // obviously suboptimal.
        const __m128i utf8_packed = _mm_packus_epi16(in_16,in_16);
        // 2. store (16 bytes)
        _mm_storeu_si128((__m128i*)utf8_output, utf8_packed);
        // 3. adjust pointers
        buf += 8;
        utf8_output += 8;
        // Proceed with next input
        in_16 = nextin_16;
      } else {
        // 1. pack the bytes
        const __m128i utf8_packed = _mm_packus_epi16(in_16, nextin_16);
        // 2. store (16 bytes)
        _mm_storeu_si128((__m128i*)utf8_output, utf8_packed);
        // 3. adjust pointers
        buf += 16;
        utf8_output += 16;
        continue; // we are done for this round!
      }
    }

    // no bits set above 7th bit
    const __m128i one_byte_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in_16, v_ff80), v_0000);
    const uint16_t one_byte_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_byte_bytemask));

    // no bits set above 11th bit
    const __m128i one_or_two_bytes_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in_16, v_f800), v_0000);
    const uint16_t one_or_two_bytes_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(one_or_two_bytes_bytemask));

    if (one_or_two_bytes_bitmask == 0xffff) {
      // case: all words either produce 1 or 2 UTF-8 bytes (at least one produces 2 bytes)
      // 1. prepare 2-byte values
      // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
      // expected output   : [110a|aaaa|10bb|bbbb] x 8
      const __m128i v_1f00 = _mm_set1_epi16((int16_t)0x1f00);
      const __m128i v_003f = _mm_set1_epi16((int16_t)0x003f);

      // t0 = [000a|aaaa|bbbb|bb00]
      const __m128i t0 = _mm_slli_epi16(in_16, 2);
      // t1 = [000a|aaaa|0000|0000]
      const __m128i t1 = _mm_and_si128(t0, v_1f00);
      // t2 = [0000|0000|00bb|bbbb]
      const __m128i t2 = _mm_and_si128(in_16, v_003f);
      // t3 = [000a|aaaa|00bb|bbbb]
      const __m128i t3 = _mm_or_si128(t1, t2);
      // t4 = [110a|aaaa|10bb|bbbb]
      const __m128i t4 = _mm_or_si128(t3, v_c080);

      // 2. merge ASCII and 2-byte codewords
      const __m128i utf8_unpacked = _mm_blendv_epi8(t4, in_16, one_byte_bytemask);

      // 3. prepare bitmask for 8-bit lookup
      //    one_byte_bitmask = hhggffeeddccbbaa -- the bits are doubled (h - MSB, a - LSB)
      const uint16_t m0 = one_byte_bitmask & 0x5555;  // m0 = 0h0g0f0e0d0c0b0a
      const uint16_t m1 = static_cast<uint16_t>(m0 >> 7);                    // m1 = 00000000h0g0f0e0
      const uint8_t  m2 = static_cast<uint8_t>((m0 | m1) & 0xff);           // m2 =         hdgcfbea
      // 4. pack the bytes
      const uint8_t* row = &simdutf::tables::utf16_to_utf8::pack_1_2_utf8_bytes[m2][0];
      const __m128i shuffle = _mm_loadu_si128((__m128i*)(row + 1));
      const __m128i utf8_packed = _mm_shuffle_epi8(utf8_unpacked, shuffle);

      // 5. store bytes
      _mm_storeu_si128((__m128i*)utf8_output, utf8_packed);

      // 6. adjust pointers
      buf += 8;
      utf8_output += row[0];
      continue;
    }

    
    // Check for overflow in packing
    const __m128i saturation_bytemask = _mm_cmpeq_epi32(_mm_and_si128(_mm_or_si128(in, nextin), v_ffff0000), v_0000);
    const uint32_t saturation_bitmask = static_cast<uint32_t>(_mm_movemask_epi8(saturation_bytemask));

    if (saturation_bitmask == 0xffff) {
      // case: words from register produce either 1, 2 or 3 UTF-8 bytes
      // validate input first
      const __m128i v_dc00 = _mm_set1_epi16((int16_t)0xdc00);
      const __m128i forbidden_bytemask = _mm_cmpeq_epi16(_mm_and_si128(in_16, v_f800), v_dc00);
      const uint32_t forbidden_bitmask = static_cast<uint32_t>(_mm_movemask_epi8(forbidden_bytemask));
      if (forbidden_bitmask != 0) { return std::make_pair(nullptr, utf8_output); }

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
#define vec(x) _mm_set1_epi16(static_cast<uint16_t>(x))
      // [aaaa|bbbb|bbcc|cccc] => [bbcc|cccc|bbcc|cccc]
      const __m128i t0 = _mm_shuffle_epi8(in_16, dup_even);
      // [bbcc|cccc|bbcc|cccc] => [00cc|cccc|0bcc|cccc]
      const __m128i t1 = _mm_and_si128(t0, vec(0b0011111101111111));
      // [00cc|cccc|0bcc|cccc] => [10cc|cccc|0bcc|cccc]
      const __m128i t2 = _mm_or_si128 (t1, vec(0b1000000000000000));

      // [aaaa|bbbb|bbcc|cccc] =>  [0000|aaaa|bbbb|bbcc]
      const __m128i s0 = _mm_srli_epi16(in_16, 4);
      // [0000|aaaa|bbbb|bbcc] => [0000|aaaa|bbbb|bb00]
      const __m128i s1 = _mm_and_si128(s0, vec(0b0000111111111100));
      // [0000|aaaa|bbbb|bb00] => [00bb|bbbb|0000|aaaa]
      const __m128i s2 = _mm_maddubs_epi16(s1, vec(0x0140));
      // [00bb|bbbb|0000|aaaa] => [11bb|bbbb|1110|aaaa]
      const __m128i s3 = _mm_or_si128(s2, vec(0b1100000011100000));
      const __m128i m0 = _mm_andnot_si128(one_or_two_bytes_bytemask, vec(0b0100000000000000));
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

      const uint8_t* row0 = &simdutf::tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
      const __m128i shuffle0 = _mm_loadu_si128((__m128i*)(row0 + 1));
      const __m128i utf8_0 = _mm_shuffle_epi8(out0, shuffle0);

      const uint8_t mask1 = static_cast<uint8_t>(mask >> 8);

      const uint8_t* row1 = &simdutf::tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask1][0];
      const __m128i shuffle1 = _mm_loadu_si128((__m128i*)(row1 + 1));
      const __m128i utf8_1 = _mm_shuffle_epi8(out1, shuffle1);

      _mm_storeu_si128((__m128i*)utf8_output, utf8_0);
      utf8_output += row0[0];
      _mm_storeu_si128((__m128i*)utf8_output, utf8_1);
      utf8_output += row1[0];

      buf += 8;
    } else {
      // case: at least one 32-bit word produce a surrogate pair in UTF-16 <=> will produce four UTF-8 bytes
      // Let us do a scalar fallback.
      // It may seem wasteful to use scalar code, but being efficient with SIMD
      // in the presence of surrogate pairs may require non-trivial tables.
      size_t forward = 15;
      size_t k = 0;
      if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
      for(; k < forward; k++) {
        uint32_t word = buf[k];
        if((word & 0xFFFFFF80)==0) {
          *utf8_output++ = char(word);
        } else if((word & 0xFFFFF800)==0) {
          *utf8_output++ = char((word>>6) | 0b11000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        } else if((word &0xFFFF0000 )==0) {
          *utf8_output++ = char((word>>12) | 0b11100000);
          *utf8_output++ = char(((word>>6) & 0b111111) | 0b10000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        } else {
          *utf8_output++ = char((word>>18) | 0b11110000);
          *utf8_output++ = char(((word>>12) & 0b111111) | 0b10000000);
          *utf8_output++ = char(((word>>6) & 0b111111) | 0b10000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        }
      }
      buf += k;
    }
  } // while

  return std::make_pair(buf, utf8_output);
}