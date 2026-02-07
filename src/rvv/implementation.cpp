#include "simdutf/rvv/begin.h"
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
#ifndef SIMDUTF_RVV_H
  #error "rvv.h must be included"
#endif

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

//
// Implementation-specific overrides
//
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
#include "rvv/rvv_helpers.inl.cpp"

#include "rvv/rvv_length_from.inl.cpp"
#include "rvv/rvv_validate.inl.cpp"

#include "rvv/rvv_latin1_to.inl.cpp"
#include "rvv/rvv_utf16_to.inl.cpp"

#include "rvv/rvv_utf32_to.inl.cpp"
#include "rvv/rvv_utf8_to.inl.cpp"

#if SIMDUTF_FEATURE_BASE64
  #include "rvv/rvv_find.cpp"
#endif // SIMDUTF_FEATURE_BASE64

#if SIMDUTF_FEATURE_UTF16
  #include "rvv/rvv_utf16fix.cpp"
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused int
implementation::detect_encodings(const char *input,
                                 size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = simdutf::BOM::check_bom(input, length);
  if (bom_encoding != encoding_type::unspecified)
    return bom_encoding;
  // todo: reimplement as a one-pass algorithm.
  int out = 0;
  if (validate_utf8(input, length))
    out |= encoding_type::UTF8;
  if (length % 2 == 0) {
    if (validate_utf16le(reinterpret_cast<const char16_t *>(input), length / 2))
      out |= encoding_type::UTF16_LE;
  }
  if (length % 4 == 0) {
    if (validate_utf32(reinterpret_cast<const char32_t *>(input), length / 4))
      out |= encoding_type::UTF32_LE;
  }

  return out;
}
#endif // SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_BASE64
simdutf_warn_unused result implementation::base64_to_binary(
    const char *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  return simdutf::scalar::base64::base64_to_binary_details_impl(
      input, length, output, options, last_chunk_options);
}

simdutf_warn_unused result implementation::base64_to_binary(
    const char16_t *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  return simdutf::scalar::base64::base64_to_binary_details_impl(
      input, length, output, options, last_chunk_options);
}

simdutf_warn_unused full_result implementation::base64_to_binary_details(
    const char *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  return simdutf::scalar::base64::base64_to_binary_details_impl(
      input, length, output, options, last_chunk_options);
}

simdutf_warn_unused full_result implementation::base64_to_binary_details(
    const char16_t *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  return simdutf::scalar::base64::base64_to_binary_details_impl(
      input, length, output, options, last_chunk_options);
}

size_t implementation::binary_to_base64(const char *input, size_t length,
                                        char *output,
                                        base64_options options) const noexcept {
  return scalar::base64::tail_encode_base64(output, input, length, options);
}

size_t implementation::binary_to_base64_with_lines(
    const char *input, size_t length, char *output, size_t line_length,
    base64_options options) const noexcept {
  return scalar::base64::tail_encode_base64_impl<true>(output, input, length,
                                                       options, line_length);
}
#endif // SIMDUTF_FEATURE_BASE64
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused result
implementation::utf8_length_from_utf16le_with_replacement(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16_with_replacement<
      endianness::LITTLE>(input, length);
}

simdutf_warn_unused result
implementation::utf8_length_from_utf16be_with_replacement(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16_with_replacement<
      endianness::BIG>(input, length);
}

simdutf_warn_unused size_t
implementation::convert_utf16le_to_utf8_with_replacement(
    const char16_t *input, size_t length,
    char *utf8_buffer) const noexcept {
  return scalar::utf16_to_utf8::convert_with_replacement<endianness::LITTLE>(
      input, length, utf8_buffer);
}

simdutf_warn_unused size_t
implementation::convert_utf16be_to_utf8_with_replacement(
    const char16_t *input, size_t length,
    char *utf8_buffer) const noexcept {
  return scalar::utf16_to_utf8::convert_with_replacement<endianness::BIG>(
      input, length, utf8_buffer);
}

#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/rvv/end.h"
