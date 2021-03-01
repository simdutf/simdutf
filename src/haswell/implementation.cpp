#include "tables/utf8_to_utf16_tables.h"

#include "simdutf/haswell/begin.h"
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
      utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][0];
  const uint8_t consumed =
      utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][1];
  const __m128i in = _mm_loadu_si128((__m128i *)input);
  if (idx < 64) {
    // SIX (6) input code-words
    // this is a relatively easy scenario
    // we process SIX (6) input code-words. The max length in bytes of six code
    // words spanning between 1 and 2 bytes each is 12 bytes. On processors
    // where pdep/pext is fast, we might be able to use a small lookup table.
    const __m128i sh =
        _mm_loadu_si128((const __m128i *)utf8_to_utf16::shufutf8[idx]);
    const __m128i perm = _mm_shuffle_epi8(in, sh);
    const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi16(0x7f));
    const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi16(0x1f00));
    const __m128i composed = _mm_or_si128(ascii, _mm_srli_epi16(highbyte, 2));
    _mm_storeu_si128((__m128i *)utf16_output, composed);
    utf16_output += 6; // We wrote 12 bytes, 6 code points.
  } else if (idx < 145) {
    // FOUR (4) input code-words
    const __m128i sh =
        _mm_loadu_si128((const __m128i *)utf8_to_utf16::shufutf8[idx]);
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
        _mm_loadu_si128((const __m128i *)utf8_to_utf16::shufutf8[idx]);
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

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

simdutf_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return haswell::utf8_validation::generic_validate_utf8(buf,len);
}

simdutf_warn_unused bool implementation::validate_utf16(const char16_t *buf, size_t len) const noexcept {
  return haswell::utf16_validation::scalar_validate_utf16(buf, len);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return haswell::utf8_to_utf16::scalar_convert_utf8_to_utf16(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16(const char* input, size_t size, 
    char16_t* utf16_output) const noexcept {
  // The implementation is not specific to haswell and should be moved to the generic directory.
  size_t pos = 0;
  char16_t* start{utf16_output};
  while(pos + 64 <= size) {
    // this loop could be unrolled further. For example, we could process the mask
    // far more than 64 bytes.
    //
    // For pure ASCII inputs, this function is not optimally fast because they are
    // faster ways to just check for ASCII than to compute the continuation mask.
    // However, the continuation mask is more informative. There might be a trade-off
    // involved.
    //
    simd8x64<int8_t> in(reinterpret_cast<const int8_t *>(input + pos));
    uint64_t utf8_continuation_mask = in.lt(-65 + 1);
    // -65 is 0b10111111 in two-complement's, so largest possible continuation byte
    if(utf8_continuation_mask != 0) {
      // Slow path. We hope that the compiler will recognize that this is a slow path.
      // Anything that is not a continuation mask is a 'leading byte', that is, the
      // start of a new code point.
      uint64_t utf8_leading_mask = ~utf8_continuation_mask;
      // The *start* of code points is not so useful, rather, we want the *end* of code points.
      uint64_t utf8_end_of_code_point_mask = utf8_leading_mask>>1;
      size_t max_starting_point = (pos + 64) - 12 - 1;
      while(pos <= max_starting_point) {
        size_t consumed = convert_masked_utf8_to_utf16(input + pos,
                            utf8_end_of_code_point_mask, utf16_output);
        pos += consumed;
        utf8_end_of_code_point_mask >>= consumed;
      }
    } else {
      in.store_ascii_as_utf16(utf16_output);
      utf16_output += 64;
      pos += 64;
    }
  }
  utf16_output += utf8_to_utf16::scalar_convert_valid_utf8_to_utf16(input + pos, size - pos, utf16_output);
  return utf16_output - start;
}

simdutf_warn_unused size_t implementation::convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return fallback::utf16_to_utf8::scalar_convert(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return fallback::utf16_to_utf8::scalar_convert(buf, len, utf8_output);
}


} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/haswell/end.h"
