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

#if 0
#include "1.h"
#if 0
#define D(name) printf("%-20s = ", #name); dump_epu32_hex(name);
#else
#define D(_)
#endif
#endif

namespace {
  int convert_UCS4_to_UTF8(__m128i in, __m128i& out) {
    const __m128i v_80   = _mm_set1_epi32(0x00000080);
    const __m128i v_7f   = _mm_set1_epi32(0x0000007f);
    const __m128i v_7ff  = _mm_set1_epi32(0x000007ff);
    const __m128i v_ffff = _mm_set1_epi32(0x0000ffff);

    const __m128i mask_1_byte      = _mm_cmplt_epi32(in, v_80);
    const __m128i mask_2_3_4_bytes = _mm_cmpgt_epi32(in, v_7f);
    const __m128i mask_3_4_bytes   = _mm_cmpgt_epi32(in, v_7ff);
    const __m128i mask_4_bytes     = _mm_cmpgt_epi32(in, v_ffff);

    // 0a. convert surrogate pairs into UCS
    {
        const __m128i v_00010400 = _mm_set1_epi32(0x00010400);
        const __m128i v_00010000 = _mm_set1_epi32(0x00010000);
        const __m128i t0 = _mm_madd_epi16(in, v_00010400); // join 10-bit words into single 20-bit word
        const __m128i t1 = _mm_add_epi32 (t0, v_00010000); // add 0x1'0000

        in = _mm_blendv_epi8(in, t1, mask_4_bytes);
    }

    // 1a. shift 6-bit words - step #1
    // 1. [00000000|00000000|00000000|0xxxxxxx]
    // 2. [00000000|00000000|00000yyy|yyxxxxxx]
    // 3. [00000000|00000000|zzzzyyyy|yyxxxxxx]
    // 4. [00000000|0000wwzz|zzzzyyyy|yyxxxxxx]

    const __m128i v_0000003f = _mm_set1_epi32(0x0000003f);
    const __m128i v_00003f00 = _mm_set1_epi32(0x00003f00);
    const __m128i v_003f0000 = _mm_set1_epi32(0x003f0000);
    const __m128i v_0f000000 = _mm_set1_epi32(0x0f000000);
    __m128i byte1 = _mm_and_si128(in, v_0000003f);
    __m128i byte2 = _mm_slli_epi32(in, 2);
    byte2 = _mm_and_si128(byte2, v_00003f00); // byte2 = [00000000|00000000|00yyyyyy|00000000]
    __m128i byte3 = _mm_slli_epi32(in, 4);
    byte3 = _mm_and_si128(byte3, v_003f0000); // byte3 = [00000000|00zzzzzz|00000000|00000000]
    __m128i byte4 = _mm_slli_epi32(in, 6);
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
    const __m128i v_00004080      = _mm_set1_epi32(0x00004080);
    const __m128i cnt_2_3_4_bytes = _mm_and_si128(mask_2_3_4_bytes, v_00004080);
    const __m128i cnt_3_4_bytes   = _mm_and_si128(mask_3_4_bytes, v_00004080);
    const __m128i cnt_4_bytes     = _mm_and_si128(mask_4_bytes, v_00004080);
    const __m128i count = _mm_add_epi32(_mm_add_epi32(cnt_2_3_4_bytes, cnt_3_4_bytes), cnt_4_bytes);

    // Now, when we gather MSB using _mm_movemask_epi8, we obtain a bitmask
    // like this: 00hg00fe00dc00ba, where hg, fe, dc and ba are the two-bit counters.
    const int m0 = _mm_movemask_epi8(count);

    // Compress a sparse 16-bit word bitmask into 8-bit one: hgdcfeba
    const uint8_t mask = (m0 >> 6) | m0;

    const auto& to_utf8 = utf16_to_utf8::ucs4_to_utf8[mask];
    // merged = [000000ww|00zzzzzz|00yyyyyy|00xxxxxx]
    //          [00000000|00000000|00000000|0xxxxxxx]
    merged = _mm_or_si128(merged, _mm_loadu_si128((__m128i*)to_utf8.const_bits_mask));
    
    out = _mm_shuffle_epi8(merged, _mm_loadu_si128((__m128i*)to_utf8.shuffle));

    return to_utf8.output_bytes;
  }
}


/*
  Returns a pair: the first unprocessed byte from buf and utf8_output
  A scalar routing should carry on the conversion of the tail.
*/
std::pair<const char16_t*, char*> sse_convert_valid_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) {

  char* start = utf8_output;
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
          // 1. prepare 3-byte values
          // input 16-bit words : [aaaa|bbbb|bbcc|cccc] x 8
          // output words bc    : [10bb|bbbb|10cc|cccc]
          //              a     : [0000|0000|1110|aaaa]
          const __m128i v_3f00 = _mm_set1_epi16((int16_t)0x3f00);
          const __m128i v_003f = _mm_set1_epi16((int16_t)0x003f);
          const __m128i v_8080 = _mm_set1_epi16((int16_t)0x8080);
          const __m128i v_00e0 = _mm_set1_epi16((int16_t)0x00e0);

          // t0 = [00bb|bbbb|cccc|cc00]
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
          // a  = [0000|0000|1110|aaaa] -- masking is not needed, as shuffles will omit this byte if not requred in output
          const __m128i a  = _mm_or_si128(t5, v_00e0);

          // 2. prepare 2-byte values
          // t3 = [00bb|bbbb|00cc|cccc], but for the 2-byte case 'b' subword has 5 bits, in fact:
          // t3 = [000b|bbbb|00cc|cccc] -- thus
          // t7 = [110b|bbbb|10cc|cccc]
          const __m128i t7 = _mm_or_si128(t3, v_c080);

          // 3. join lower words
          const __m128i t8 = _mm_blendv_epi8(bc, t7, one_or_two_bytes_bytemask);
          const __m128i t9 = _mm_blendv_epi8(t8, in, one_byte_bytemask);

          // 4. expand words 16-bit => 32-bit
          const __m128i out0 = _mm_unpacklo_epi16(t9, a);
          const __m128i out1 = _mm_unpackhi_epi16(t9, a);

          // 5. compress 32-bit words into 1, 2 or 3 bytes -- 2 x shuffle
          const uint16_t mask = (one_byte_bitmask & 0x5555) | 
                                (one_or_two_bytes_bitmask & 0xaaaa);

          const uint8_t mask0 = uint8_t(mask);

          const uint8_t* row0 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
          const __m128i shuffle0 = _mm_loadu_si128((__m128i*)(row0 + 1));
          const __m128i utf8_0 = _mm_shuffle_epi8(out0, shuffle0);

          const uint8_t mask1 = (mask >> 8);
          const uint8_t* row1 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask1][0];
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
      const uint16_t H = _mm_movemask_epi8(vH);

      // 1c. obtain mask for log surrogates (0xD800..0xDBFF)
      const uint16_t L = ~H & surrogates_bitmask;

      const uint16_t a = L & (H >> 2);
      const uint16_t b = a << 2;
      const uint16_t c = V | a | b;
      ///// end of copy

      // bail out if input is invalid
      if ((c != 0xffff) and (c != 0x3fff))
        return std::make_pair(nullptr, utf8_output);

      // 3. keep 10 lowest bits from surrogate pairs having actual data
      const __m128i v_03ff03ff = _mm_set1_epi32(0x03ff03ff);
      const __m128i t0         = _mm_and_si128(in, v_03ff03ff);
      const __m128i in1        = _mm_blendv_epi8(in, t0, surrogates_bytemask);

      // 2. use surrogates_bitmask to obtain shuffle pattern
      //    we expand 16-bit words into 32-bit words
      // 
      //    surrogates_bitmask = hhggffeeddccbbaa
      //    expansion_id       = 0h0g0f0e0d0c0b0a
      //                       |        0h0g0f0e0
      //                       =        ehdgcfbea
      const uint8_t expansion_id = (surrogates_bitmask & 0x5555) | ((surrogates_bitmask >> 7) & 0xaaaa);

      const __m128i shuffle_lo = _mm_loadu_si128((__m128i*)&utf16_to_utf8::expand_surrogates[expansion_id][0]);
      const __m128i shuffle_hi = _mm_loadu_si128((__m128i*)&utf16_to_utf8::expand_surrogates[expansion_id][16]);

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

      utf8_output -= utf16_to_utf8::expand_surrogates[expansion_id][32];

      if (c == 0xffff)
        buf += 8;
      else
        buf += 7;
    }
  } // while

  return std::make_pair(buf, utf8_output);
}
