#include <utility>

#include "simdutf/westmere/begin.h"
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
#ifndef SIMDUTF_WESTMERE_H
#error "westmere.h must be included"
#endif
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

#include "westmere/sse_convert_utf8_to_utf16.cpp"
#include "westmere/sse_convert_utf8_to_utf32.cpp"
#include "westmere/sse_validate_utf16le.cpp"
#include "westmere/sse_convert_utf16_to_utf8.cpp"
#include "westmere/sse_convert_utf16_to_utf32.cpp"

// UTF-16 => UTF-8 conversion

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "generic/buf_block_reader.h"
#include "generic/utf8_validation/utf8_lookup4_algorithm.h"
#include "generic/utf8_validation/utf8_validator.h"
// transcoding from UTF-8 to UTF-16
#include "generic/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "generic/utf8_to_utf16/utf8_to_utf16.h"
// transcoding from UTF-8 to UTF-32
#include "generic/utf8_to_utf32/valid_utf8_to_utf32.h"
#include "generic/utf8_to_utf32/utf8_to_utf32.h"
// other functions
#include "generic/utf8.h"
#include "generic/utf16.h"
//
// Implementation-specific overrides
//

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

simdutf_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return westmere::utf8_validation::generic_validate_utf8(buf, len);
}

simdutf_warn_unused bool implementation::validate_ascii(const char *buf, size_t len) const noexcept {
  return westmere::utf8_validation::generic_validate_ascii(buf, len);
}

simdutf_warn_unused bool implementation::validate_utf16(const char16_t *buf, size_t len) const noexcept {
  const char16_t* tail = sse_validate_utf16le(buf, len);
  if (tail) {
    return scalar::utf16::validate(tail, len - (tail - buf));
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

simdutf_warn_unused size_t implementation::convert_utf8_to_utf32(const char* buf, size_t len, char32_t* utf32_output) const noexcept {
  utf8_to_utf32::validating_transcoder converter;
  return converter.convert(buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf32(const char* input, size_t size,
    char32_t* utf32_output) const noexcept {
  return utf8_to_utf32::convert_valid(input, size,  utf32_output);
}

simdutf_warn_unused size_t implementation::convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  std::pair<const char16_t*, char*> ret = sse_convert_utf16_to_utf8(buf, len, utf8_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf8::convert(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return convert_utf16_to_utf8(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_utf16_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::pair<const char16_t*, char32_t*> ret = sse_convert_utf16_to_utf32(buf, len, utf32_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf32_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return convert_utf16_to_utf32(buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::count_utf16(const char16_t * input, size_t length) const noexcept {
  return utf16::count_code_points(input, length);
}

simdutf_warn_unused size_t implementation::count_utf8(const char * input, size_t length) const noexcept {
  return utf8::count_code_points(input, length);
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf16(const char16_t * input, size_t length) const noexcept {
  return utf16::utf8_length_from_utf16(input, length);
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf16(const char16_t * input, size_t length) const noexcept {
  return utf16::utf32_length_from_utf16(input, length);
}

simdutf_warn_unused size_t implementation::utf16_length_from_utf8(const char * input, size_t length) const noexcept {
  return utf8::utf16_length_from_utf8(input, length);
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf8(const char * input, size_t length) const noexcept {
  return utf8::utf32_length_from_utf8(input, length);
}

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/westmere/end.h"
