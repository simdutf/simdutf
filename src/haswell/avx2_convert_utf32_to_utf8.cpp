std::pair<const char32_t*, char*> avx2_convert_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) {
  const char32_t* end = buf + len;
  const __m256i v_00000000 = _mm256_setzero_si256();
  const __m256i v_ffffff80 = _mm256_set1_epi32((int32_t)0xffffff80);
  const __m256i v_fffff800 = _mm256_set1_epi32((int32_t)0xfffff800);
  const __m256i v_ffff0000 = _mm256_set1_epi32((int32_t)0xffff0000); 
  const __m256i v_0000c080 = _mm256_set1_epi32((int32_t)0x0000c080);
  const size_t safety_margin = 11; // to avoid overruns, see issue https://github.com/simdutf/simdutf/issues/92

  while (buf + 16 + safety_margin <= end) {
    __m256i in = _mm256_loadu_si256((__m256i*)buf);
    
    if(_mm256_testz_si256(in, v_ffffff80)) { // ASCII fast path!!!!
        // Could be improved by checking if the following 256 bits are also ASCII
        // 1. pack the bytes
        const __m128i utf16_packed = _mm_packus_epi32(_mm256_castsi256_si128(in),_mm256_extractf128_si256(in,1));
        const __m128i utf8_packed = _mm_packus_epi16(utf16_packed, _mm_setzero_si128());
        // 2. store (16 bytes)
        _mm_storeu_si128((__m128i*)utf8_output, utf8_packed);
        // 3. adjust pointers
        buf += 8;
        utf8_output += 8;
        continue; // we are done for this round!
    }
    // no bits set above 7th bit
    const __m256i one_byte_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffffff80), v_00000000);
    const uint32_t one_byte_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(one_byte_bytemask));

    // no bits set above 11th bit
    const __m256i one_or_two_bytes_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_fffff800), v_00000000);
    const uint32_t one_or_two_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(one_or_two_bytes_bytemask));
    if (one_or_two_bytes_bitmask == 0xffffffff) {

          // 1. prepare 2-byte values
          // input 32-bit word : [0000|0000|0000|0000|0000|0aaa|aabb|bbbb]
          // expected output   : [110a|aaaa|10bb|bbbb]
          const __m256i v_1f00 = _mm256_set1_epi32((int32_t)0x1f00);
          const __m256i v_003f = _mm256_set1_epi32((int32_t)0x003f);

          // t0 = [0000|0000|0000|0000|000a|aaaa|bbbb|bb00]
          const __m256i t0 = _mm256_slli_epi16(in, 2);
          // t1 = [0000|0000|000a|aaaa|0000|0000]
          const __m256i t1 = _mm256_and_si256(t0, v_1f00);
          // t2 = [0000|0000|0000|0000|0000|0000|00bb|bbbb]
          const __m256i t2 = _mm256_and_si256(in, v_003f);
          // t3 = [0000|0000|0000|0000|000a|aaaa|00bb|bbbb]
          const __m256i t3 = _mm256_or_si256(t1, t2);
          // t4 = [0000|0000|0000|0000|110a|aaaa|10bb|bbbb]
          const __m256i t4 = _mm256_or_si256(t3, v_0000c080);

          // 2. merge ASCII and 2-byte codewords
          const __m256i utf8_unpacked = _mm256_blendv_epi8(t4, in, one_byte_bytemask);

          // Need new tables to pack utf8_unpacked

          buf += 8;
          continue;
    }
    // no bits set above 16th bit
    const __m256i one_two_three_bytes_bytemask = _mm256_cmpeq_epi16(_mm256_and_si256(in, v_ffff0000), v_00000000);
    const uint32_t one_two_three_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(one_two_three_bytes_bytemask));
    if (one_two_three_bytes_bitmask == 0x00000000) {
      // case: words from register produce either 1, 2 or 3 UTF-8 bytes
        const __m256i dup_even = _mm256_setr_epi16(0x0000, 0x0202, 0x0404, 0x0606,
                                                0x0808, 0x0a0a, 0x0c0c, 0x0e0e,
                                                0x0000, 0x0202, 0x0404, 0x0606,
                                                0x0808, 0x0a0a, 0x0c0c, 0x0e0e);

        /* In this branch we handle three cases:
           1. [0000|0000|0000|0000|0000|0000|0ccc|cccc] => [0ccc|cccc]                           - single UFT-8 byte
           2. [0000|0000|0000|0000|0000|0bbb|bbcc|cccc] => [110b|bbbb], [10cc|cccc]              - two UTF-8 bytes
           3. [0000|0000|0000|0000|aaaa|bbbb|bbcc|cccc] => [1110|aaaa], [10bb|bbbb], [10cc|cccc] - three UTF-8 bytes

          Note that the last byte in cases #2 and #3 is the same. We first pack the words in
          the registers from 32-bit to 16-bit. Then, we simply apply the procedure for same case
          in UTF16 => UTF8.

          We precompute byte 1 for case #1 and the common byte for cases #2 & #3
          in register t2.

          We precompute byte 1 for case #3 and -- **conditionally** -- precompute
          either byte 1 for case #2 or byte 2 for case #3. Note that they
          differ by exactly one bit.

          Finally from these two words we build proper UTF-8 sequence, taking
          into account the case (i.e, the number of bytes to write).
        */
        

        // 1. Pack 32-bit integers into 16-bit integers. Then, apply routine from UTF16 to UTF8.
        in = _mm256_packus_epi32(in, v_00000000);

        // 2. Produce t2 and s4.
        /**
         * Given [0000|0000|0000|0000|aaaa|bbbb|bbcc|cccc] our goal is to produce:
         * t2 => [0ccc|cccc] [10cc|cccc]
         * s4 => [1110|aaaa] ([110b|bbbb] OR [10bb|bbbb])
         */
#define vec(x) _mm256_set1_epi16(static_cast<uint16_t>(x))
        // [aaaa|bbbb|bbcc|cccc] => [bbcc|cccc|bbcc|cccc]
        const __m256i t0 = _mm256_shuffle_epi8(in, dup_even);
        // [bbcc|cccc|bbcc|cccc] => [00cc|cccc|0bcc|cccc]
        const __m256i t1 = _mm256_and_si256(t0, vec(0b0011111101111111));
        // [00cc|cccc|0bcc|cccc] => [10cc|cccc|0bcc|cccc]
        const __m256i t2 = _mm256_or_si256 (t1, vec(0b1000000000000000));

        // [aaaa|bbbb|bbcc|cccc] =>  [0000|aaaa|bbbb|bbcc]
        const __m256i s0 = _mm256_srli_epi16(in, 4);
        // [0000|aaaa|bbbb|bbcc] => [0000|aaaa|bbbb|bb00]
        const __m256i s1 = _mm256_and_si256(s0, vec(0b0000111111111100));
        // [0000|aaaa|bbbb|bb00] => [00bb|bbbb|0000|aaaa]
        const __m256i s2 = _mm256_maddubs_epi16(s1, vec(0x0140));
        // [00bb|bbbb|0000|aaaa] => [11bb|bbbb|1110|aaaa]
        const __m256i s3 = _mm256_or_si256(s2, vec(0b1100000011100000));
        const __m256i m0 = _mm256_andnot_si256(one_or_two_bytes_bytemask, vec(0b0100000000000000));
        const __m256i s4 = _mm256_xor_si256(s3, m0);
#undef vec

        // 3. expand words 16-bit => 32-bit
        const __m256i out0 = _mm256_unpacklo_epi16(t2, s4);
        const __m256i out1 = _mm256_unpackhi_epi16(t2, s4);

        // 4. compress 32-bit words into 1, 2 or 3 bytes -- 2 x shuffle
        const uint32_t mask = (one_byte_bitmask & 0x55555555) |
                              (one_or_two_bytes_bitmask & 0xaaaaaaaa);
        // Due to the wider registers, the following path is less likely to be useful.
        /*if(mask == 0) {
          // We only have three-byte words. Use fast path.
          const __m256i shuffle = _mm256_setr_epi8(2,3,1,6,7,5,10,11,9,14,15,13,-1,-1,-1,-1, 2,3,1,6,7,5,10,11,9,14,15,13,-1,-1,-1,-1);
          const __m256i utf8_0 = _mm256_shuffle_epi8(out0, shuffle);
          const __m256i utf8_1 = _mm256_shuffle_epi8(out1, shuffle);
          _mm_storeu_si128((__m128i*)utf8_output, _mm256_castsi256_si128(utf8_0));
          utf8_output += 12;
          _mm_storeu_si128((__m128i*)utf8_output, _mm256_castsi256_si128(utf8_1));
          utf8_output += 12;
          _mm_storeu_si128((__m128i*)utf8_output, _mm256_extractf128_si256(utf8_0,1));
          utf8_output += 12;
          _mm_storeu_si128((__m128i*)utf8_output, _mm256_extractf128_si256(utf8_1,1));
          utf8_output += 12;
          buf += 16;
          continue;
        }*/
        const uint8_t mask0 = uint8_t(mask);
        const uint8_t* row0 = &simdutf::tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
        const __m128i shuffle0 = _mm_loadu_si128((__m128i*)(row0 + 1));
        const __m128i utf8_0 = _mm_shuffle_epi8(_mm256_castsi256_si128(out0), shuffle0);

        const uint8_t mask1 = static_cast<uint8_t>(mask >> 8);
        const uint8_t* row1 = &simdutf::tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask1][0];
        const __m128i shuffle1 = _mm_loadu_si128((__m128i*)(row1 + 1));
        const __m128i utf8_1 = _mm_shuffle_epi8(_mm256_castsi256_si128(out1), shuffle1);

        const uint8_t mask2 = static_cast<uint8_t>(mask >> 16);
        const uint8_t* row2 = &simdutf::tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask2][0];
        const __m128i shuffle2 = _mm_loadu_si128((__m128i*)(row2 + 1));
        const __m128i utf8_2 = _mm_shuffle_epi8(_mm256_extractf128_si256(out0,1), shuffle2);


        const uint8_t mask3 = static_cast<uint8_t>(mask >> 24);
        const uint8_t* row3 = &simdutf::tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask3][0];
        const __m128i shuffle3 = _mm_loadu_si128((__m128i*)(row3 + 1));
        const __m128i utf8_3 = _mm_shuffle_epi8(_mm256_extractf128_si256(out1,1), shuffle3);

        // 5. Adjust pointers.
        _mm_storeu_si128((__m128i*)utf8_output, utf8_0);
        utf8_output += row0[0];
        _mm_storeu_si128((__m128i*)utf8_output, utf8_1);
        utf8_output += row1[0];
        _mm_storeu_si128((__m128i*)utf8_output, utf8_2);
        utf8_output += row2[0];
        _mm_storeu_si128((__m128i*)utf8_output, utf8_3);
        utf8_output += row3[0];
        buf += 8;
    // At least one 32-bit word will produce a 4-byte UTF-8.
    } else {
      // Let us do a scalar fallback.
      // It may seem wasteful to use scalar code, but being efficient with SIMD
      // may require large, non-trivial tables?
      size_t forward = 7;
      size_t k = 0;
      if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
      for(; k < forward; k++) {
        uint32_t word = buf[k];
        if((word & 0xFFFFFF80)==0) {  // 1-byte (ASCII)
          *utf8_output++ = char(word);
        } else if((word & 0xFFFFF800)==0) { // 2-byte
          *utf8_output++ = char((word>>6) | 0b11000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        } else if((word & 0xFFFF0000 )==0) {  // 3-byte
          *utf8_output++ = char((word>>12) | 0b11100000);
          *utf8_output++ = char(((word>>6) & 0b111111) | 0b10000000);
          *utf8_output++ = char((word & 0b111111) | 0b10000000);
        } else {  // 4-byte
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