/*

    We consider two cases:
    1. an input register contains no surrogates --- i.e. each value
       is in range 0x0000 .. 0xffff and fits in 16 bits
    2. an input register contains surrogates --- i.e. codepoints 
       can have 16 or 32 bits.
    
    It should be also checked if a special case when the input has only
    surrogates (32-bit codepoints) appears in real data.

    --------------------------------------------------

    Ad 1. A single 16-bit code UTF-16 can yield 1, 2 or 3 UTF-8 bytes.
    It depends on value, as follows:

    - 0x0000 .. 0x007F -> 1 byte (ASCII)
    - 0x0080 .. 0x07FF -> 2 bytes
    - 0x0800 .. 0xFFFF -> 3 bytes

    Example input (a-f are bitfields):
    - word 0 would yield three UTF-8 bytes
    - word 1 would yield two UTF-8 bytes
    - word 2 would yield one UTF-8 byte

    [aaaa|bbbb|bbcc|cccc|0000|0ddd|ddee|eeee|0000|0000|0fff|ffff| ...........]
    |      word 0       |      word 1       |        word 2     | words 3..7 |

    1. detect ASCII

    [0000|0000|0000|0000|0000|0000|0000|0000|1111|1111|1111|1111] - one_byte_bytemask
    |      word 0       |      word 1       |        word 2     |

    2. detect ASCII or 2-byte output

    [0000|0000|0000|0000|1111|1111|1111|1111|1111|1111|1111|1111] - one_or_two_bytes_bytemask
    |      word 0       |      word 1       |        word 2     |

    3. 2-byte output:

    one_or_two_bytes_bytemask and not one_byte_bytemask

    4. 3-byte output

    not one_or_two_bytes_bytemask


*/

#include "sse-convert-utf16-to-utf8-lookup.cpp"

/*
  Returns a pair: the first unprocessed byte from buf and utf8_output
  A scalar routing should carry on the conversion of the tail.
*/
std::pair<const char16_t*, char*> sse_convert_valid_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) {

#if 0
  return std::make_pair(buf, utf8_output);
#else
  char* start = utf8_output;
  const char16_t* end = buf + len;

  const __m128i v_0000 = _mm_setzero_si128();
  const __m128i v_f800 = _mm_set1_epi16((int16_t)0xf800);
  const __m128i v_d800 = _mm_set1_epi16((int16_t)0xd800);

  while (buf + 8 < end) {

    __m128i in = _mm_loadu_si128((__m128i*)buf);

    // 1. Check if there are any surrogate word in the input chunk.
    //    We have also deal with situation when there is a suggogate word
    //    at the and of chunk.
    const __m128i s0 = _mm_and_si128(in, v_f800);
    const __m128i surrogates_bytemask = _mm_cmpeq_epi16(s0, v_d800);

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
          const __m128i v_c080 = _mm_set1_epi16((int16_t)0xc080);

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
          const uint8_t  m2 = (m0 | m1) & 0xff;           // m2 =         hdgcfbea

          // 4. pack the bytes
          const uint8_t* row = &utf16_to_utf8::pack_1_2_utf8_bytes[m2][0];
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
#if 0
          // 1. prepare 3-byte values
          // input 16-bit words : [aaaa|bbbb|bbcc|cccc] x 8
          // output words bc    : [10bb|bbbb|10cc|cccc]
          //              a     : [0000|0000|1110|aaaa]
          const __m128i v_3f00 = _mm_set1_epi16((int16_t)0x3f00);
          const __m128i v_003f = _mm_set1_epi16((int16_t)0x003f);
          const __m128i v_8080 = _mm_set1_epi16((int16_t)0x8080);
          const __m128i v_00e0 = _mm_set1_epi16((int16_t)0x00e0);

          // t0 = [00bb|bbbb|aaaa|aa00]
          const __m128i t0 = _mm_slli_epi16(in, 2);
          // t1 = [00bb|bbbb|0000|0000]
          const __m128i t1 = _mm_and_si128(t0, v_3f00);
          // t2 = [0000|0000|00cc|cccc]
          const __m128i t2 = _mm_and_si128(in, v_003f);
          // t3 = [00bb|bbbb|00cc|cccc]
          const __m128i t3 = _mm_or_si128(t1, t2);
          // bc = [10bb|bbbb|10cc|cccc]
          const __m128i bc = _mm_or_si128(t3, v_8080);

          // t5 = [0000|0000|0000|aaaa]
          const __m128i t5 = _mm_srli_epi16(in, 12);
          // t6 = [0000|0000|1110|aaaa]
          const __m128i t6 = _mm_or_si128(t5, v_00e0);
          // a  = mask t6
          const __m128i a  = _mm_andnot_si128(one_or_two_bytes_bytemask, t6);

          // 2. prepare 2-byte values
          // t3 = [00bb|bbbb|00cc|cccc], but for the 2-byte case 'b' subword has 5 bits, in fact:
          // t3 = [000b|bbbb|00cc|cccc] -- thus
          // t6 = [110b|bbbb|10cc|cccc]
          const __m128i t6 = _mm_or_si128(t3, v_c080);

          // 3. join lower words
          const __m128i t9  = _mm_blendv_epi8(bc, t6, one_or_two_bytes_bytemask);
          const __m128i t10 = _mm_blendv_epi8(t9, t6, one_byte_bytemask);

          // 4. expand words 16-bit => 32-bit
          const __m128i out0 = _mm_unpacklo_epi16(t10, a);
          const __m128i out1 = _mm_unpackhi_epi16(t10, a);

          // 5. compress -- 2 x shuffle
#else
      abort();
#endif
      }
    // surrogate pairs in a register
    } else {
      abort();
    }
  } // while

  return std::make_pair(buf, utf8_output);
#endif
}
