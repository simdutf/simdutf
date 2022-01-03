#include "simdutf/avx512bw/begin.h"

#include "scalar/utf16_to_utf8/valid_utf16_to_utf8.h"
#include "scalar/utf16_to_utf8/utf16_to_utf8.h"
#include "scalar/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "scalar/utf8_to_utf16/utf8_to_utf16.h"
#include "scalar/utf8.h"
#include "scalar/utf16.h"

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
#   include "avx512bw-constants.inl.cpp"
#   include "avx512bw-utf8-common.inl.cpp"
#   include "avx512bw-from-valid-utf8.inl.cpp"
#   include "avx512bw-from-utf8.inl.cpp"
} // namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

simdutf_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
    return scalar::utf8::validate(buf, len);
}

simdutf_warn_unused bool implementation::validate_utf16(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate(buf, len);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = avx512bw::validating_utf8_to_fixed_length<char16_t>(buf, len, utf16_output);
  if (ret.second == nullptr)
    return 0;

  size_t saved_bytes = ret.second - utf16_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end and ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf16::convert_valid(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = avx512bw::valid_utf8_to_fixed_length<char16_t>(buf, len, utf16_output);
  size_t saved_bytes = ret.second - utf16_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf16::convert_valid(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_valid(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::count_utf16(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::count_code_points(input, length);
}

simdutf_warn_unused size_t implementation::count_utf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::count_code_points(input, length);
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf16(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16(input, length);
}

simdutf_warn_unused size_t implementation::utf16_length_from_utf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::utf16_length_from_utf8(input, length);
}

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/avx512bw/end.h"
