#include "tables/utf8_to_utf16_tables.h"
#include "scalar/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "scalar/utf8_to_utf16/utf8_to_utf16.h"
#include "tables/utf16_to_utf8_tables.h"

#include "simdutf/westmere/begin.h"
#include <utility>
#include <internal/utf16_decode.h>
#include <internal/utf8_encode.h>

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

using namespace simd;

simdutf_really_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.reduce_or().is_ascii();
}

simdutf_unused simdutf_really_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0b11000000u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

simdutf_really_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_third_byte | is_fourth_byte) > int8_t(0);
}

// Convert up to 12 bytes from utf8 to utf16 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
size_t convert_masked_utf8_to_utf16(const char *input,
                           uint64_t utf8_end_of_code_point_mask,
                           char16_t *&utf16_output) {
  // we use an approach where we try to process up to 12 input bytes.
  // Why 12 input bytes and not 16? Because we are concerned with the size of
  // the lookup tables. Also 12 is nicely divisible by two and three.
  //
  const uint16_t input_mask = 0xFFF;
  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask & input_mask;
  const uint8_t idx =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][0];
  const uint8_t consumed =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][1];
  const __m128i in = _mm_loadu_si128((__m128i *)input);
  if (idx < 64) {
    // SIX (6) input code-words
    // this is a relatively easy scenario
    // we process SIX (6) input code-words. The max length in bytes of six code
    // words spanning between 1 and 2 bytes each is 12 bytes. On processors
    // where pdep/pext is fast, we might be able to use a small lookup table.
    const __m128i sh =
        _mm_loadu_si128((const __m128i *)tables::utf8_to_utf16::shufutf8[idx]);
    const __m128i perm = _mm_shuffle_epi8(in, sh);
    const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi16(0x7f));
    const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi16(0x1f00));
    const __m128i composed = _mm_or_si128(ascii, _mm_srli_epi16(highbyte, 2));
    _mm_storeu_si128((__m128i *)utf16_output, composed);
    utf16_output += 6; // We wrote 12 bytes, 6 code points.
  } else if (idx < 145) {
    // FOUR (4) input code-words
    const __m128i sh =
        _mm_loadu_si128((const __m128i *)tables::utf8_to_utf16::shufutf8[idx]);
    const __m128i perm = _mm_shuffle_epi8(in, sh);
    const __m128i ascii =
        _mm_and_si128(perm, _mm_set1_epi32(0x7f)); // 7 or 6 bits
    const __m128i middlebyte =
        _mm_and_si128(perm, _mm_set1_epi32(0x3f00)); // 5 or 6 bits
    const __m128i middlebyte_shifted = _mm_srli_epi32(middlebyte, 2);
    const __m128i highbyte =
        _mm_and_si128(perm, _mm_set1_epi32(0x0f0000)); // 4 bits
    const __m128i highbyte_shifted = _mm_srli_epi32(highbyte, 4);
    const __m128i composed =
        _mm_or_si128(_mm_or_si128(ascii, middlebyte_shifted), highbyte_shifted);
    const __m128i composed_repacked = _mm_packus_epi32(composed, composed);
    _mm_storeu_si128((__m128i *)utf16_output, composed_repacked);
    utf16_output += 4;
  } else if (idx < 209) {
    // TWO (2) input code-words
    const __m128i sh =
        _mm_loadu_si128((const __m128i *)tables::utf8_to_utf16::shufutf8[idx]);
    const __m128i perm = _mm_shuffle_epi8(in, sh);
    const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi32(0x7f));
    const __m128i middlebyte = _mm_and_si128(perm, _mm_set1_epi32(0x3f00));
    const __m128i middlebyte_shifted = _mm_srli_epi32(middlebyte, 2);
    __m128i middlehighbyte = _mm_and_si128(perm, _mm_set1_epi32(0x3f0000));
    // correct for spurious high bit
    const __m128i correct =
        _mm_srli_epi32(_mm_and_si128(perm, _mm_set1_epi32(0x400000)), 1);
    middlehighbyte = _mm_xor_si128(correct, middlehighbyte);
    const __m128i middlehighbyte_shifted = _mm_srli_epi32(middlehighbyte, 4);
    const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi32(0x07000000));
    const __m128i highbyte_shifted = _mm_srli_epi32(highbyte, 6);
    const __m128i composed =
        _mm_or_si128(_mm_or_si128(ascii, middlebyte_shifted),
                     _mm_or_si128(highbyte_shifted, middlehighbyte_shifted));
    const __m128i composedminus =
        _mm_sub_epi32(composed, _mm_set1_epi32(0x10000));
    const __m128i lowtenbits =
        _mm_and_si128(composedminus, _mm_set1_epi32(0x3ff));
    const __m128i hightenbits = _mm_srli_epi32(composedminus, 10);
    const __m128i lowtenbitsadd =
        _mm_add_epi32(lowtenbits, _mm_set1_epi32(0xDC00));
    const __m128i hightenbitsadd =
        _mm_add_epi32(hightenbits, _mm_set1_epi32(0xD800));
    const __m128i lowtenbitsaddshifted = _mm_slli_epi32(lowtenbitsadd, 16);
    const __m128i surrogates =
        _mm_or_si128(hightenbitsadd, lowtenbitsaddshifted);
    uint32_t basic_buffer[4];
    _mm_storeu_si128((__m128i *)basic_buffer, composed);
    uint32_t surrogate_buffer[4];
    _mm_storeu_si128((__m128i *)surrogate_buffer, surrogates);
    for (size_t i = 0; i < 3; i++) {
      if (basic_buffer[i] < 65536) {
        utf16_output[0] = uint16_t(basic_buffer[i]);
        utf16_output++;
      } else {
        utf16_output[0] = uint16_t(surrogate_buffer[i] & 0xFFFF);
        utf16_output[1] = uint16_t(surrogate_buffer[i] >> 16);
        utf16_output += 2;
      }
    }
  } else {
    // here we know that there is an error but we do not handle errors
  }
  return consumed;
}

// UTF-16 => UTF-8 conversion
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

  const auto& to_utf8 = tables::utf16_to_utf8::ucs4_to_utf8[mask];
  // merged = [000000ww|00zzzzzz|00yyyyyy|00xxxxxx]
  //          [00000000|00000000|00000000|0xxxxxxx]
  merged = _mm_or_si128(merged, _mm_loadu_si128((__m128i*)to_utf8.const_bits_mask));

  out = _mm_shuffle_epi8(merged, _mm_loadu_si128((__m128i*)to_utf8.shuffle));

  return to_utf8.output_bytes;
}

/*
  Returns a pair: the first unprocessed byte from buf and utf8_output
  A scalar routing should carry on the conversion of the tail.
*/
std::pair<const char16_t*, char*> sse_convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) {

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

          const uint8_t* row0 = &tables::utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
          const __m128i shuffle0 = _mm_loadu_si128((__m128i*)(row0 + 1));
          const __m128i utf8_0 = _mm_shuffle_epi8(out0, shuffle0);

          const uint8_t mask1 = (mask >> 8);
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

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "generic/buf_block_reader.h"
#include "generic/utf8_validation/utf8_lookup4_algorithm.h"
#include "generic/utf8_validation/utf8_validator.h"
#include "generic/utf16_validation/utf16_scalar_validator.h" // Daniel: This should go in the fallback kernel TODO
// transcoding from UTF-16 to UTF-8
#include "generic/utf16_to_utf8/valid_utf16_to_utf8.h"
#include "generic/utf16_to_utf8/utf16_to_utf8.h"
// transcoding from UTF-8 to UTF-16
#include "generic/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "generic/utf8_to_utf16/utf8_to_utf16.h"
//
// Implementation-specific overrides
//

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

simdutf_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return westmere::utf8_validation::generic_validate_utf8(buf, len);
}

simdutf_warn_unused bool implementation::validate_utf16(const char16_t *buf, size_t len) const noexcept {
  return westmere::utf16_validation::scalar_validate_utf16(buf, len);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16(const char* input, size_t size,
    char16_t* utf16_output) const noexcept {
  return utf8_to_utf16::convert_valid(input, size,  utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  std::pair<const char16_t*, char*> ret = sse_convert_utf16_to_utf8(buf, len, utf8_output);
  if (ret.first == nullptr)
    return 0;

  size_t saved_bytes = ret.second - utf8_output;
  size_t scalar_saved_bytes = 0;

  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = fallback::utf16_to_utf8::scalar_convert(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0)
      return 0;

    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
    return convert_utf16_to_utf8(buf, len, utf8_output);
}

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/westmere/end.h"
