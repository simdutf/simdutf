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

#include "arm_convert_utf8_to_utf16.cpp"
#include "arm_validate_utf16le.cpp"

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
  //return arm64::utf16_validation::scalar_validate_utf16(buf, len);
  const char16_t* tail = arm_validate_utf16le(buf, len);
  if (tail) {
    return utf16_validation::scalar_validate_utf16(tail, len - (tail - buf));
  } else {
    return false;
  }
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
