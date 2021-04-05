#include "tables/utf8_to_utf16_tables.h"
#include "scalar/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "scalar/utf8_to_utf16/utf8_to_utf16.h"
#include "scalar/utf8.h"
#include "scalar/utf16.h"

#include "simdutf/arm64/begin.h"
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

using namespace simd;

simdutf_really_inline bool is_ascii(const simd8x64<uint8_t>& input) {
    simd8<uint8_t> bits = input.reduce_or();
    return bits.max_val() < 0b10000000u;
}

simdutf_unused simdutf_really_inline simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<bool> is_second_byte = prev1 >= uint8_t(0b11000000u);
    simd8<bool> is_third_byte  = prev2 >= uint8_t(0b11100000u);
    simd8<bool> is_fourth_byte = prev3 >= uint8_t(0b11110000u);
    // Use ^ instead of | for is_*_byte, because ^ is commutative, and the caller is using ^ as well.
    // This will work fine because we only have to report errors for cases with 0-1 lead bytes.
    // Multiple lead bytes implies 2 overlapping multibyte characters, and if that happens, there is
    // guaranteed to be at least *one* lead byte that is part of only 1 other multibyte character.
    // The error will be detected there.
    return is_second_byte ^ is_third_byte ^ is_fourth_byte;
}

simdutf_really_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<bool> is_third_byte  = prev2 >= uint8_t(0b11100000u);
    simd8<bool> is_fourth_byte = prev3 >= uint8_t(0b11110000u);
    return is_third_byte ^ is_fourth_byte;
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

  uint8x16_t in = vld1q_u8(reinterpret_cast<const uint8_t*>(input));

  if (idx < 64) {
    // SIX (6) input code-words
    // this is a relatively easy scenario
    // we process SIX (6) input code-words. The max length in bytes of six code
    // words spanning between 1 and 2 bytes each is 12 bytes.
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(tables::utf8_to_utf16::shufutf8[idx]));
    uint8x16_t perm = vqtbl1q_u8(in, sh);
    uint8x16_t ascii = vandq_u8(perm, vmovq_n_u16(0x7f));
    uint8x16_t highbyte = vandq_u8(perm, vmovq_n_u16(0x1f00));
    uint8x16_t composed = vorrq_u8(ascii, vshrq_n_u16(highbyte, 2));
    vst1q_u8(reinterpret_cast<uint8_t*>(utf16_output), composed);
    utf16_output += 6; // We wrote 12 bytes, 6 code points.
  } else if (idx < 145) {
    // FOUR (4) input code-words
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(tables::utf8_to_utf16::shufutf8[idx]));
    uint8x16_t perm = vqtbl1q_u8(in, sh);
    uint8x16_t ascii =
        vandq_u8(perm, vmovq_n_u32(0x7f)); // 7 or 6 bits
    uint8x16_t middlebyte =
        vandq_u8(perm, vmovq_n_u32(0x3f00)); // 5 or 6 bits
    uint8x16_t middlebyte_shifted = vshrq_n_u32(middlebyte, 2);
    uint32x4_t highbyte =
        vandq_u8(perm, vmovq_n_u32(0x0f0000)); // 4 bits
    uint32x4_t highbyte_shifted = vshrq_n_u32(highbyte, 4);
    uint32x4_t composed =
        vorrq_u32(vorrq_u32(ascii, middlebyte_shifted), highbyte_shifted);
    uint16x8_t composed_repacked = vmovn_high_u32(vmovn_u32(composed), composed);
    vst1q_u16(reinterpret_cast<uint16_t*>(utf16_output), composed_repacked);
    utf16_output += 4;
  } else if (idx < 209) {
    // TWO (2) input code-words
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(tables::utf8_to_utf16::shufutf8[idx]));
    uint8x16_t perm = vqtbl1q_u8(in, sh);
    uint8x16_t ascii = vandq_u8(perm, vmovq_n_u32(0x7f));
    uint8x16_t middlebyte = vandq_u8(perm, vmovq_n_u32(0x3f00));
    uint8x16_t middlebyte_shifted = vshrq_n_u32(middlebyte, 2);
    uint8x16_t middlehighbyte = vandq_u8(perm, vmovq_n_u32(0x3f0000));
    // correct for spurious high bit
    uint8x16_t correct =
        vshrq_n_u32(vandq_u8(perm, vmovq_n_u32(0x400000)), 1);
    middlehighbyte = veorq_u8(correct, middlehighbyte);
    uint8x16_t middlehighbyte_shifted = vshrq_n_u32(middlehighbyte, 4);
    uint8x16_t highbyte = vandq_u8(perm, vmovq_n_u32(0x07000000));
    uint8x16_t highbyte_shifted = vshrq_n_u32(highbyte, 6);
    uint8x16_t composed =
        vorrq_u8(vorrq_u8(ascii, middlebyte_shifted),
                     vorrq_u8(highbyte_shifted, middlehighbyte_shifted));
    uint32x4_t composedminus =
        vsubq_u32(composed, vmovq_n_u32(0x10000));
    uint32x4_t lowtenbits =
        vandq_u32(composedminus, vmovq_n_u32(0x3ff));
    uint32x4_t hightenbits = vshrq_n_u32(composedminus, 10);
    uint32x4_t lowtenbitsadd =
        vaddq_u32(lowtenbits, vmovq_n_u32(0xDC00));
    uint32x4_t hightenbitsadd =
        vaddq_u32(hightenbits, vmovq_n_u32(0xD800));
    uint32x4_t lowtenbitsaddshifted = vshlq_n_u32(lowtenbitsadd, 16);
    uint32x4_t surrogates =
        vorrq_u32(hightenbitsadd, lowtenbitsaddshifted);
    uint32_t basic_buffer[4];
    vst1q_u32(basic_buffer, composed);
    uint32_t surrogate_buffer[4];
    vst1q_u32(surrogate_buffer, surrogates);
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
// other functions
#include "generic/utf8.h"
#include "generic/utf16.h"
//
// Implementation-specific overrides
//
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

simdutf_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return arm64::utf8_validation::generic_validate_utf8(buf,len);
}

simdutf_warn_unused bool implementation::validate_utf16(const char16_t *buf, size_t len) const noexcept {
  return arm64::utf16_validation::scalar_validate_utf16(buf, len);
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
  return utf16_to_utf8::scalar_convert(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return utf16_to_utf8::scalar_convert(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::count_utf16(const char16_t * input, size_t length) const noexcept {
  return utf16::count_code_points(input, length);
}

simdutf_warn_unused size_t implementation::count_utf8(const char * input, size_t length) const noexcept {
  return utf8::count_code_points(input, length);
}


} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/arm64/end.h"
