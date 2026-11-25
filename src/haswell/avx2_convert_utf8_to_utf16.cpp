// depends on "tables/utf8_to_utf16_tables.h"

// Convert up to 12 bytes from utf8 to utf16 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
template <endianness big_endian>
size_t convert_masked_utf8_to_utf16(const char *input,
                                    uint64_t utf8_end_of_code_point_mask,
                                    char16_t *&utf16_output) {
  // we use an approach where we try to process up to 12 input bytes.
  // Why 12 input bytes and not 16? Because we are concerned with the size of
  // the lookup tables. Also 12 is nicely divisible by two and three.
  //
  //
  // Optimization note: our main path below is load-latency dependent. Thus it
  // is maybe beneficial to have fast paths that depend on branch prediction but
  // have less latency. This results in more instructions but, potentially, also
  // higher speeds.
  //
  // We first try a few fast paths.
  const __m128i swap =
      _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
  const __m128i in = _mm_loadu_si128((__m128i *)input);
  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask & 0xfff;
  if ((utf8_end_of_code_point_mask & 0xfff) == 0xfff) {
    // We process the data in chunks of 12 bytes.
    __m256i ascii = _mm256_cvtepu8_epi16(in);
    if (big_endian) {
      const __m256i swap256 = _mm256_setr_epi8(
          1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 17, 16, 19, 18,
          21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30);
      ascii = _mm256_shuffle_epi8(ascii, swap256);
    }
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(utf16_output), ascii);
    utf16_output += 12; // We wrote 12 16-bit characters.
    return 12;          // We consumed 12 bytes.
  }
  if (((utf8_end_of_code_point_mask & 0xffff) == 0xaaaa)) {
    // We want to take 8 2-byte UTF-8 code units and turn them into 8 2-byte
    // UTF-16 code units. There is probably a more efficient sequence, but the
    // following might do.
    const __m128i sh =
        _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
    const __m128i perm = _mm_shuffle_epi8(in, sh);
    const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi16(0x7f));
    const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi16(0x1f00));
    __m128i composed = _mm_or_si128(ascii, _mm_srli_epi16(highbyte, 2));
    if (big_endian)
      composed = _mm_shuffle_epi8(composed, swap);
    _mm_storeu_si128((__m128i *)utf16_output, composed);
    utf16_output += 8; // We wrote 16 bytes, 8 code points.
    return 16;
  }
  if (input_utf8_end_of_code_point_mask == 0x924) {
    // We want to take 4 3-byte UTF-8 code units and turn them into 4 2-byte
    // UTF-16 code units. There is probably a more efficient sequence, but the
    // following might do.
    const __m128i sh =
        _mm_setr_epi8(2, 1, 0, -1, 5, 4, 3, -1, 8, 7, 6, -1, 11, 10, 9, -1);
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
    __m128i composed_repacked = _mm_packus_epi32(composed, composed);
    if (big_endian)
      composed_repacked = _mm_shuffle_epi8(composed_repacked, swap);
    _mm_storeu_si128((__m128i *)utf16_output, composed_repacked);
    utf16_output += 4;
    return 12;
  }

  const uint8_t idx = simdutf::tables::utf8_to_utf16::utf8bigindex
      [input_utf8_end_of_code_point_mask][0];
  const uint8_t consumed = simdutf::tables::utf8_to_utf16::utf8bigindex
      [input_utf8_end_of_code_point_mask][1];
  if (idx < 64) {
    // SIX (6) input code-code units
    // this is a relatively easy scenario
    // we process SIX (6) input code-code units. The max length in bytes of six
    // code code units spanning between 1 and 2 bytes each is 12 bytes. On
    // processors where pdep/pext is fast, we might be able to use a small
    // lookup table.
    const __m128i sh = _mm_loadu_si128(
        (const __m128i *)simdutf::tables::utf8_to_utf16::shufutf8[idx]);
    const __m128i perm = _mm_shuffle_epi8(in, sh);
    const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi16(0x7f));
    const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi16(0x1f00));
    __m128i composed = _mm_or_si128(ascii, _mm_srli_epi16(highbyte, 2));
    if (big_endian)
      composed = _mm_shuffle_epi8(composed, swap);
    _mm_storeu_si128((__m128i *)utf16_output, composed);
    utf16_output += 6; // We wrote 12 bytes, 6 code points. There is a potential
                       // overflow of 4 bytes.
  } else if (idx < 145) {
    // FOUR (4) input code-code units
    const __m128i sh = _mm_loadu_si128(
        (const __m128i *)simdutf::tables::utf8_to_utf16::shufutf8[idx]);
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
    __m128i composed_repacked = _mm_packus_epi32(composed, composed);
    if (big_endian)
      composed_repacked = _mm_shuffle_epi8(composed_repacked, swap);
    _mm_storeu_si128((__m128i *)utf16_output, composed_repacked);
    utf16_output += 4; // Here we overflow by 8 bytes.
  } else if (idx < 209) {
    // TWO (2) input code-code units
    //////////////
    // There might be garbage inputs where a leading byte mascarades as a
    // four-byte leading byte (by being followed by 3 continuation byte), but is
    // not greater than 0xf0. This could trigger a buffer overflow if we only
    // counted leading bytes of the form 0xf0 as generating surrogate pairs,
    // without further UTF-8 validation. Thus we must be careful to ensure that
    // only leading bytes at least as large as 0xf0 generate surrogate pairs. We
    // do as at the cost of an extra mask.
    /////////////
    const __m128i sh = _mm_loadu_si128(
        (const __m128i *)simdutf::tables::utf8_to_utf16::shufutf8[idx]);
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
    // We deliberately carry the leading four bits in highbyte if they are
    // present, we remove them later when computing hightenbits.
    const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi32(0xff000000));
    const __m128i highbyte_shifted = _mm_srli_epi32(highbyte, 6);
    // When we need to generate a surrogate pair (leading byte > 0xF0), then
    // the corresponding 32-bit value in 'composed'  will be greater than
    // > (0xff00000>>6) or > 0x3c00000. This can be used later to identify the
    // location of the surrogate pairs.
    const __m128i composed =
        _mm_or_si128(_mm_or_si128(ascii, middlebyte_shifted),
                     _mm_or_si128(highbyte_shifted, middlehighbyte_shifted));
    const __m128i composedminus =
        _mm_sub_epi32(composed, _mm_set1_epi32(0x10000));
    const __m128i lowtenbits =
        _mm_and_si128(composedminus, _mm_set1_epi32(0x3ff));
    // Notice the 0x3ff mask:
    const __m128i hightenbits =
        _mm_and_si128(_mm_srli_epi32(composedminus, 10), _mm_set1_epi32(0x3ff));
    const __m128i lowtenbitsadd =
        _mm_add_epi32(lowtenbits, _mm_set1_epi32(0xDC00));
    const __m128i hightenbitsadd =
        _mm_add_epi32(hightenbits, _mm_set1_epi32(0xD800));
    const __m128i lowtenbitsaddshifted = _mm_slli_epi32(lowtenbitsadd, 16);
    __m128i surrogates = _mm_or_si128(hightenbitsadd, lowtenbitsaddshifted);
    uint32_t basic_buffer[4];
    uint32_t basic_buffer_swap[4];
    if (big_endian) {
      _mm_storeu_si128((__m128i *)basic_buffer_swap,
                       _mm_shuffle_epi8(composed, swap));
      surrogates = _mm_shuffle_epi8(surrogates, swap);
    }
    _mm_storeu_si128((__m128i *)basic_buffer, composed);
    uint32_t surrogate_buffer[4];
    _mm_storeu_si128((__m128i *)surrogate_buffer, surrogates);
    for (size_t i = 0; i < 3; i++) {
      if (basic_buffer[i] > 0x3c00000) {
        utf16_output[0] = uint16_t(surrogate_buffer[i] & 0xffff);
        utf16_output[1] = uint16_t(surrogate_buffer[i] >> 16);
        utf16_output += 2;
      } else {
        utf16_output[0] = big_endian ? uint16_t(basic_buffer_swap[i])
                                     : uint16_t(basic_buffer[i]);
        utf16_output++;
      }
    }
  } else {
    // here we know that there is an error but we do not handle errors
  }
  return consumed;
}

/*
 * Adapted from ffutf by Peter Fors (MIT licensed):
 * https://github.com/vtlmks/ffutf
 */
namespace utf8_to_utf16_fast {

alignas(64) constexpr uint8_t second_bounds[64] = {
    0xc0, 0xc0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xa0,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x20, 0x80, 0x80, 0x90, 0x80, 0x80, 0x80, 0x10, 0xc0, 0xc0,
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};

alignas(16) constexpr uint8_t word_shuffle[256][16] = {
    {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {2, 3, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 2, 3, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 4, 5, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {6, 7, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 6, 7, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 128, 128, 128, 128, 128, 128, 128, 128},
    {8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128},
    {6, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 128, 128, 128, 128, 128, 128},
    {10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {6, 7, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 128, 128, 128, 128, 128, 128},
    {8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128},
    {6, 7, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 128, 128, 128, 128},
    {12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128},
    {8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128},
    {6, 7, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 128, 128, 128, 128},
    {10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {6, 7, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 128, 128, 128, 128},
    {8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {4, 5, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128},
    {6, 7, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128},
    {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 128, 128},
    {14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {6, 7, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 128, 128, 128, 128, 128, 128},
    {8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128},
    {6, 7, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 128, 128, 128, 128},
    {10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {6, 7, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 14, 15, 128, 128, 128, 128},
    {8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {4, 5, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128},
    {6, 7, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128},
    {4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 128, 128},
    {12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
     128},
    {0, 1, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {4, 5, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {6, 7, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {4, 5, 6, 7, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 128, 128, 128, 128},
    {8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {4, 5, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128},
    {6, 7, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128},
    {4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 128, 128},
    {10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {2, 3, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {4, 5, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {6, 7, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 6, 7, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 128, 128},
    {8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128, 128, 128},
    {0, 1, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {2, 3, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128},
    {6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128, 128, 128},
    {0, 1, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128},
    {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128, 128, 128},
    {0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128},
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 128, 128},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};

simdutf_really_inline __m256i zext128(__m128i x) {
  return _mm256_set_m128i(_mm_setzero_si128(), x);
}

simdutf_really_inline bool valid_second(__m256i bytes, __m256i next) {
  const __m256i high_table = _mm256_broadcastsi128_si256(
      _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 6, 56));
  const __m256i low_table = _mm256_broadcastsi128_si256(_mm_setr_epi8(
      11, 1, 0, 0, 16, 32, 32, 32, 32, 32, 32, 32, 32, 36, 32, 32));
  const __m256i next_table = _mm256_broadcastsi128_si256(
      _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 43, 51, 53, 53, 0, 0, 0, 0));
  const __m256i low_nibble = _mm256_set1_epi8(15);
  const __m256i high_flags = _mm256_shuffle_epi8(
      high_table, _mm256_and_si256(_mm256_srli_epi16(bytes, 4), low_nibble));
  const __m256i low_flags = _mm256_shuffle_epi8(
      low_table, _mm256_and_si256(bytes, _mm256_set1_epi8(127)));
  const __m256i next_flags = _mm256_shuffle_epi8(
      next_table, _mm256_and_si256(_mm256_srli_epi16(next, 4), low_nibble));
  return _mm256_testz_si256(_mm256_and_si256(high_flags, low_flags),
                            next_flags);
}

simdutf_really_inline size_t compact_words(__m128i words, uint32_t take,
                                           char16_t *output) {
  const __m128i indices =
      _mm_load_si128(reinterpret_cast<const __m128i *>(word_shuffle[take]));
  _mm_storeu_si128(reinterpret_cast<__m128i *>(output),
                   _mm_shuffle_epi8(words, indices));
  return size_t(count_ones(take));
}

template <endianness big_endian>
simdutf_really_inline size_t convert_block(__m256i bytes, __m256i prev1,
                                           __m256i prev2, __m256i prev3,
                                           uint32_t active, char16_t *output) {
  const __m256i first = _mm256_subs_epu8(prev1, _mm256_set1_epi8(0x40));
  const __m256i second = _mm256_subs_epu8(prev2, _mm256_set1_epi8(0x60));
  const __m256i third = _mm256_subs_epu8(prev3, _mm256_set1_epi8(0x70));
  const __m256i needs = _mm256_or_si256(first, _mm256_or_si256(second, third));
  const __m256i cont = _mm256_cmpgt_epi8(_mm256_set1_epi8(-64), bytes);
  if ((uint32_t(_mm256_movemask_epi8(_mm256_xor_si256(needs, cont))) &
       active) ||
      !valid_second(prev1, bytes)) {
    return SIZE_MAX;
  }
  const __m256i long_first = _mm256_subs_epu8(prev1, _mm256_set1_epi8(0x60));
  const __m256i high_surrogate =
      _mm256_subs_epu8(prev2, _mm256_set1_epi8(0x70));
  const __m256i ends = _mm256_or_si256(_mm256_andnot_si256(long_first, first),
                                       _mm256_or_si256(second, third));
  const uint32_t take = active & (~uint32_t(_mm256_movemask_epi8(bytes)) |
                                  uint32_t(_mm256_movemask_epi8(ends)));
  const __m256i shifted = _mm256_slli_epi16(prev1, 6);
  __m256i lo = _mm256_or_si256(_mm256_and_si256(shifted, _mm256_set1_epi8(-64)),
                               _mm256_and_si256(bytes, _mm256_set1_epi8(63)));
  const __m256i mid =
      _mm256_and_si256(_mm256_srli_epi16(prev1, 2), _mm256_set1_epi8(15));
  __m256i top =
      _mm256_and_si256(_mm256_slli_epi16(prev2, 4), _mm256_set1_epi8(-16));
  top = _mm256_blendv_epi8(_mm256_setzero_si256(), top, second);
  __m256i hi = _mm256_and_si256(cont, _mm256_or_si256(mid, top));
  lo = _mm256_blendv_epi8(bytes, lo, bytes);
  if (_mm256_movemask_epi8(_mm256_or_si256(high_surrogate, third))) {
    __m256i high_lo = _mm256_or_si256(
        _mm256_and_si256(_mm256_slli_epi16(prev1, 2), _mm256_set1_epi8(-4)),
        _mm256_and_si256(_mm256_srli_epi16(bytes, 4), _mm256_set1_epi8(3)));
    high_lo = _mm256_sub_epi8(high_lo, _mm256_set1_epi8(0x40));
    const __m256i borrow = _mm256_cmpgt_epi8(_mm256_set1_epi8(-112), prev1);
    __m256i high_hi = _mm256_add_epi8(
        _mm256_and_si256(prev2, _mm256_set1_epi8(7)), _mm256_set1_epi8(-40));
    high_hi = _mm256_add_epi8(high_hi, borrow);
    const __m256i low_hi = _mm256_or_si256(
        _mm256_and_si256(mid, _mm256_set1_epi8(3)), _mm256_set1_epi8(-36));
    lo = _mm256_blendv_epi8(lo, high_lo, high_surrogate);
    hi = _mm256_blendv_epi8(hi, high_hi, high_surrogate);
    hi = _mm256_blendv_epi8(hi, low_hi, third);
  }
  const __m256i even = match_system(big_endian) ? _mm256_unpacklo_epi8(lo, hi)
                                                : _mm256_unpacklo_epi8(hi, lo);
  const __m256i odd = match_system(big_endian) ? _mm256_unpackhi_epi8(lo, hi)
                                               : _mm256_unpackhi_epi8(hi, lo);
  size_t written =
      compact_words(_mm256_castsi256_si128(even), take & 255, output);
  written += compact_words(_mm256_castsi256_si128(odd), (take >> 8) & 255,
                           output + written);
  if (active >> 16) {
    written += compact_words(_mm256_extracti128_si256(even, 1),
                             (take >> 16) & 255, output + written);
    written += compact_words(_mm256_extracti128_si256(odd, 1), take >> 24,
                             output + written);
  }
  return written;
}

template <endianness big_endian>
simdutf_really_inline size_t convert_block_bounded(__m256i bytes, __m256i prev1,
                                                   __m256i prev2, __m256i prev3,
                                                   const uint8_t *next,
                                                   char16_t *output) {
  const __m256i following =
      _mm256_loadu_si256(reinterpret_cast<const __m256i *>(next));
  const uint32_t following_cont = uint32_t(_mm256_movemask_epi8(
      _mm256_cmpgt_epi8(_mm256_set1_epi8(-64), following)));
  if (simdutf_likely(count_ones(following_cont) <= 24)) {
    return convert_block<big_endian>(bytes, prev1, prev2, prev3, UINT32_MAX,
                                     output);
  }
  char16_t packed[40];
  const size_t count =
      convert_block<big_endian>(bytes, prev1, prev2, prev3, UINT32_MAX, packed);
  if (count != SIZE_MAX) {
    std::memcpy(output, packed, count * sizeof(char16_t));
  }
  return count;
}

template <endianness big_endian>
simdutf_really_inline __m256i widen(__m128i bytes) {
  const __m256i words = _mm256_cvtepu8_epi16(bytes);
  return match_system(big_endian) ? words : _mm256_slli_epi16(words, 8);
}

template <endianness big_endian>
simdutf_really_inline void widen_ascii(__m256i bytes, char16_t *output) {
  _mm256_storeu_si256(reinterpret_cast<__m256i *>(output),
                      widen<big_endian>(_mm256_castsi256_si128(bytes)));
  _mm256_storeu_si256(reinterpret_cast<__m256i *>(output + 16),
                      widen<big_endian>(_mm256_extracti128_si256(bytes, 1)));
}

simdutf_really_inline __m256i load_tail(const uint8_t *input, size_t len) {
  if (len >= 32) {
    return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
  }
  if (len >= 16) {
    const __m128i first =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
    const __m128i last =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + len - 16));
    const __m128i indices =
        _mm_add_epi8(_mm_set1_epi8(char(32 - len)),
                     _mm_setr_epi8(112, 113, 114, 115, 116, 117, 118, 119, 120,
                                   121, 122, 123, 124, 125, 126, 127));
    return _mm256_set_m128i(_mm_shuffle_epi8(last, indices), first);
  }
  if (len >= 8) {
    const __m128i first =
        _mm_loadl_epi64(reinterpret_cast<const __m128i *>(input));
    const __m128i last =
        _mm_loadl_epi64(reinterpret_cast<const __m128i *>(input + len - 8));
    const __m128i indices = _mm_add_epi8(
        _mm_set1_epi8(char(16 - len)),
        _mm_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, 112, 113,
                      114, 115, 116, 117, 118, 119));
    return zext128(_mm_or_si128(first, _mm_shuffle_epi8(last, indices)));
  }
  uint64_t packed = 0;
  std::memcpy(&packed, input, len);
  return zext128(_mm_cvtsi64_si128(int64_t(packed)));
}

template <endianness big_endian>
simdutf_really_inline size_t convert_edge(__m256i bytes, uint32_t history,
                                          uint32_t active, char16_t *output) {
  const __m128i prefix = _mm_slli_si128(_mm_cvtsi32_si128(int(history)), 12);
  const __m256i before =
      _mm256_set_m128i(_mm256_castsi256_si128(bytes), prefix);
  const __m256i prev1 = _mm256_alignr_epi8(bytes, before, 15);
  const __m256i prev2 = _mm256_alignr_epi8(bytes, before, 14);
  const __m256i prev3 = _mm256_alignr_epi8(bytes, before, 13);
  return convert_block<big_endian>(bytes, prev1, prev2, prev3, active, output);
}

simdutf_really_inline uint32_t last_continuation(const uint8_t *input) {
  const uint32_t last = input[0];
  const uint32_t prev1 = input[-1];
  const uint32_t prev2 = input[-2];
  const uint32_t prev3 = input[-3];
  uint32_t prefix;
  if (prev1 >= 0xc0) {
    if (uint8_t(last - second_bounds[prev1 & 63]) >= 0x80) {
      return UINT32_MAX;
    }
    prefix = (prev1 & 0x1f) << 6;
  } else if (prev2 >= 0xe0) {
    prefix = ((prev2 & 0x0f) << 12) | ((prev1 & 0x3f) << 6);
  } else if (prev3 >= 0xf0) {
    prefix = 0xdc00 | ((prev1 & 0x0f) << 6);
  } else {
    return UINT32_MAX;
  }
  return prefix | (last & 0x3f);
}

template <endianness big_endian>
simdutf_really_inline size_t convert_tail(const uint8_t *input, size_t len,
                                          uint32_t history, char16_t *output,
                                          size_t written) {
  char16_t packed[64];
  size_t total = 0;
  if (len > 32) {
    total = convert_edge<big_endian>(
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input)), history,
        UINT32_MAX, packed);
    if (total == SIZE_MAX) {
      return SIZE_MAX;
    }
    input += 32;
    len -= 32;
    std::memcpy(&history, input - 4, 4);
  }
  if (len == 1) {
    uint32_t word = input[0];
    if (word >= 0x80) {
      word = last_continuation(input);
      if (word == UINT32_MAX) {
        return SIZE_MAX;
      }
    } else if (input[-1] >= 0xc0 || input[-2] >= 0xe0 || input[-3] >= 0xf0) {
      return SIZE_MAX;
    }
    packed[total++] = char16_t(match_system(big_endian)
                                   ? word
                                   : scalar::u16_swap_bytes(uint16_t(word)));
    std::memcpy(output + written, packed, total * sizeof(char16_t));
    return written + total;
  }
  const size_t count = convert_edge<big_endian>(
      load_tail(input, len), history, UINT32_MAX >> (32 - len), packed + total);
  if (count == SIZE_MAX) {
    return SIZE_MAX;
  }
  total += count;
  std::memcpy(output + written, packed, total * sizeof(char16_t));
  return written + total;
}

struct failure {
  const uint8_t *input;
  size_t written;
};

template <endianness big_endian>
simdutf_really_inline size_t convert_scalar(const uint8_t *input, size_t len,
                                            char16_t *output, size_t written,
                                            failure *fail) {
  const size_t count = scalar::utf8_to_utf16::convert<big_endian>(
      reinterpret_cast<const char *>(input), len, output + written);
  if (count == 0) {
    *fail = {input, written};
    return SIZE_MAX;
  }
  return written + count;
}

template <endianness big_endian>
size_t convert_nonascii(const uint8_t *input, size_t len, char16_t *output,
                        size_t written, failure *fail) {
  if ((input[0] & 0xf0) == 0xe0) {
    for (; len >= 28; input += 24, len -= 24, written += 8) {
      const __m128i first =
          _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
      const __m128i second =
          _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + 12));
      const __m256i source = _mm256_set_m128i(second, first);
      const __m256i indices = _mm256_broadcastsi128_si256(
          _mm_setr_epi8(2, 1, 0, -1, 5, 4, 3, -1, 8, 7, 6, -1, 11, 10, 9, -1));
      const __m256i triples = _mm256_shuffle_epi8(source, indices);
      const __m256i tags = _mm256_xor_si256(
          _mm256_and_si256(triples, _mm256_set1_epi32(0x00f0c0c0)),
          _mm256_set1_epi32(0x00e08080));
      const __m256i payload =
          _mm256_and_si256(triples, _mm256_set1_epi32(0x000f3f3f));
      const __m256i pairs =
          _mm256_maddubs_epi16(payload, _mm256_set1_epi32(0x00014001));
      const __m256i words =
          _mm256_madd_epi16(pairs, _mm256_set1_epi32(0x10000001));
      const __m256i overlong =
          _mm256_cmpgt_epi32(_mm256_set1_epi32(0x800), words);
      const __m256i surrogate =
          _mm256_cmpeq_epi32(_mm256_and_si256(words, _mm256_set1_epi32(0xf800)),
                             _mm256_set1_epi32(0xd800));
      const __m256i invalid =
          _mm256_or_si256(tags, _mm256_or_si256(overlong, surrogate));
      if (!_mm256_testz_si256(invalid, invalid)) {
        break;
      }
      __m128i result = _mm_packus_epi32(_mm256_castsi256_si128(words),
                                        _mm256_extracti128_si256(words, 1));
      if (!match_system(big_endian)) {
        result =
            _mm_or_si128(_mm_slli_epi16(result, 8), _mm_srli_epi16(result, 8));
      }
      _mm_storeu_si128(reinterpret_cast<__m128i *>(output + written), result);
    }
  }
  if ((input[0] & 0xf8) == 0xf0) {
    for (; len >= 32; input += 32, len -= 32, written += 16) {
      const __m256i bytes =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
      const __m256i tags = _mm256_xor_si256(
          _mm256_and_si256(bytes, _mm256_set1_epi32(int32_t(0xc0c0c0f8u))),
          _mm256_set1_epi32(int32_t(0x808080f0u)));
      const __m256i indices = _mm256_broadcastsi128_si256(
          _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12));
      const __m256i payload = _mm256_and_si256(
          _mm256_shuffle_epi8(bytes, indices), _mm256_set1_epi32(0x073f3f3f));
      const __m256i pairs =
          _mm256_maddubs_epi16(payload, _mm256_set1_epi32(0x40014001));
      const __m256i words = _mm256_sub_epi32(
          _mm256_madd_epi16(pairs, _mm256_set1_epi32(0x10000001)),
          _mm256_set1_epi32(0x10000));
      const __m256i invalid = _mm256_or_si256(
          tags, _mm256_and_si256(words, _mm256_set1_epi32(-0x100000)));
      if (!_mm256_testz_si256(invalid, invalid)) {
        break;
      }
      const __m256i high = _mm256_srli_epi32(words, 10);
      const __m256i low = _mm256_slli_epi32(
          _mm256_and_si256(words, _mm256_set1_epi32(0x3ff)), 16);
      __m256i result = _mm256_or_si256(_mm256_or_si256(high, low),
                                       _mm256_set1_epi32(int32_t(0xdc00d800u)));
      if (!match_system(big_endian)) {
        result = _mm256_or_si256(_mm256_slli_epi16(result, 8),
                                 _mm256_srli_epi16(result, 8));
      }
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(output + written),
                          result);
    }
    if (len < 8) {
      if (len == 0) {
        return written;
      }
      return convert_scalar<big_endian>(input, len, output, written, fail);
    }
  }
  uint32_t history = 0;
  if (len >= 64) {
    const __m256i head =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
    size_t count = convert_block_bounded<big_endian>(
        head,
        _mm256_alignr_epi8(head, _mm256_permute2x128_si256(head, head, 0x08),
                           15),
        _mm256_alignr_epi8(head, _mm256_permute2x128_si256(head, head, 0x08),
                           14),
        _mm256_alignr_epi8(head, _mm256_permute2x128_si256(head, head, 0x08),
                           13),
        input + 32, output + written);
    if (count == SIZE_MAX) {
      *fail = {input, written};
      return SIZE_MAX;
    }
    written += count;
    input += 32;
    len -= 32;
    for (; len >= 64; input += 32, len -= 32) {
      __m256i bytes =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
      if (!_mm256_movemask_epi8(bytes)) {
        if (input[-1] >= 0xc0 || input[-2] >= 0xe0 || input[-3] >= 0xf0) {
          *fail = {input, written};
          return SIZE_MAX;
        }
        do {
          widen_ascii<big_endian>(bytes, output + written);
          written += 32;
          input += 32;
          len -= 32;
          if (len < 64) {
            goto bulk_done;
          }
          bytes = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
        } while (!_mm256_movemask_epi8(bytes));
      }
      const __m256i prev1 =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input - 1));
      const __m256i prev2 =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input - 2));
      const __m256i prev3 =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input - 3));
      count = convert_block_bounded<big_endian>(bytes, prev1, prev2, prev3,
                                                input + 32, output + written);
      if (count == SIZE_MAX) {
        *fail = {input, written};
        return SIZE_MAX;
      }
      written += count;
    }
  bulk_done:
    std::memcpy(&history, input - 4, 4);
  }
  const size_t count =
      convert_tail<big_endian>(input, len, history, output, written);
  if (count == SIZE_MAX) {
    *fail = {input, written};
  }
  return count;
}

template <endianness big_endian>
size_t convert(const uint8_t *input, size_t len, char16_t *output,
               failure *fail) {
  size_t written = 0;
  if (len > 128 && len < 144) {
    const __m256i first =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
    const __m256i second =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + 32));
    const __m256i third =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + 64));
    const __m256i fourth =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + 96));
    const __m128i last =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + len - 16));
    const __m256i combined = _mm256_or_si256(_mm256_or_si256(first, second),
                                             _mm256_or_si256(third, fourth));
    if (!_mm256_movemask_epi8(combined) && !_mm_movemask_epi8(last)) {
      widen_ascii<big_endian>(first, output);
      widen_ascii<big_endian>(second, output + 32);
      widen_ascii<big_endian>(third, output + 64);
      widen_ascii<big_endian>(fourth, output + 96);
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(output + len - 16),
                          widen<big_endian>(last));
      return len;
    }
  }
  for (; len >= 128; input += 128, len -= 128, written += 128) {
    const __m256i first =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
    const __m256i second =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + 32));
    const __m256i third =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + 64));
    const __m256i fourth =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + 96));
    const __m256i combined = _mm256_or_si256(_mm256_or_si256(first, second),
                                             _mm256_or_si256(third, fourth));
    if (_mm256_movemask_epi8(combined)) {
      return convert_nonascii<big_endian>(input, len, output, written, fail);
    }
    widen_ascii<big_endian>(first, output + written);
    widen_ascii<big_endian>(second, output + written + 32);
    widen_ascii<big_endian>(third, output + written + 64);
    widen_ascii<big_endian>(fourth, output + written + 96);
  }
  if (len >= 16) {
    __m256i flags = _mm256_setzero_si256();
    size_t i = 0;
    for (; i + 32 <= len; i += 32) {
      flags = _mm256_or_si256(
          flags,
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + i)));
    }
    const __m128i last =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + len - 16));
    const __m128i before_last = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + (len >= 32 ? len - 32 : 0)));
    if (!_mm256_movemask_epi8(flags) &&
        !_mm_movemask_epi8(_mm_or_si128(before_last, last))) {
      for (i = 0; i + 32 <= len; i += 32) {
        widen_ascii<big_endian>(
            _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input + i)),
            output + written + i);
      }
      if (i != len) {
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(
                                output + written + (len >= 32 ? len - 32 : 0)),
                            widen<big_endian>(before_last));
        _mm256_storeu_si256(
            reinterpret_cast<__m256i *>(output + written + len - 16),
            widen<big_endian>(last));
      }
      return written + len;
    }
  }
  if (!len) {
    return written;
  }
  if (len < 16 && written) {
    const __m128i last =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + len - 16));
    if (!_mm_movemask_epi8(last)) {
      _mm256_storeu_si256(
          reinterpret_cast<__m256i *>(output + written + len - 16),
          widen<big_endian>(last));
      return written + len;
    }
  }
  if (len < 8) {
    return convert_scalar<big_endian>(input, len, output, written, fail);
  }
  if (len < 28) {
    const size_t count =
        convert_tail<big_endian>(input, len, 0, output, written);
    if (count == SIZE_MAX) {
      *fail = {input, written};
    }
    return count;
  }
  return convert_nonascii<big_endian>(input, len, output, written, fail);
}

simdutf_really_inline bool incomplete_end(const uint8_t *input, size_t len) {
  return (len && input[len - 1] >= 0xc0) ||
         (len >= 2 && input[len - 2] >= 0xe0) ||
         (len >= 3 && input[len - 3] >= 0xf0);
}

template <endianness big_endian>
size_t convert_utf8_to_utf16(const char *buf, size_t len, char16_t *output) {
  const uint8_t *input = reinterpret_cast<const uint8_t *>(buf);
  if (incomplete_end(input, len)) {
    return 0;
  }
  failure fail;
  const size_t count = convert<big_endian>(input, len, output, &fail);
  return count == SIZE_MAX ? 0 : count;
}

template <endianness big_endian>
simdutf::result convert_utf8_to_utf16_with_errors(const char *buf, size_t len,
                                                  char16_t *output) {
  const uint8_t *input = reinterpret_cast<const uint8_t *>(buf);
  size_t prefix = len;
  while (incomplete_end(input, prefix)) {
    prefix = prefix >= 3 && input[prefix - 3] >= 0xf0   ? prefix - 3
             : prefix >= 2 && input[prefix - 2] >= 0xe0 ? prefix - 2
                                                        : prefix - 1;
  }
  failure fail{input + prefix, 0};
  const size_t count = convert<big_endian>(input, prefix, output, &fail);
  if (count != SIZE_MAX) {
    if (prefix == len) {
      return simdutf::result(error_code::SUCCESS, count);
    }
    fail.written = count;
  }
  size_t restart = size_t(fail.input - input);
  size_t written = fail.written;
  if (restart > 0) {
    size_t lead = restart - 1;
    for (int back = 0; back < 3 && lead > 0 && (input[lead] & 0xc0) == 0x80;
         back++) {
      lead--;
    }
    const uint8_t c = input[lead];
    const size_t seq_len = c < 0xc0 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
    if (lead + seq_len > restart) {
      if (c >= 0xf0 && restart - lead == 3) {
        written--;
      }
      restart = lead;
    }
  }
  simdutf::result res = scalar::utf8_to_utf16::convert_with_errors<big_endian>(
      buf + restart, len - restart, output + written);
  if (res.error) {
    res.count += restart;
  } else {
    res.count += written;
  }
  return res;
}

} // namespace utf8_to_utf16_fast
