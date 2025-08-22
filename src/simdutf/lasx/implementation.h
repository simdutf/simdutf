#ifndef SIMDUTF_LASX_IMPLEMENTATION_H
#define SIMDUTF_LASX_IMPLEMENTATION_H

#include "simdutf.h"
#include "simdutf/internal/isadetection.h"

namespace simdutf {
namespace lasx {

namespace {
using namespace simdutf;
}

class implementation final : public simdutf::implementation {
public:
  simdutf_really_inline implementation()
      : simdutf::implementation("lasx", "LOONGARCH ASX",
                                internal::instruction_set::LSX |
                                    internal::instruction_set::LASX) {}
#if SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused int detect_encodings(const char *input,
                                           size_t length) const noexcept final;
#endif // SIMDUTF_FEATURE_DETECT_ENCODING
#if SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool validate_utf8(const char *buf,
                                         size_t len) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING
#if SIMDUTF_FEATURE_UTF8
  simdutf_warn_unused result
  validate_utf8_with_errors(const char *buf, size_t len) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF8
#if SIMDUTF_FEATURE_ASCII
  simdutf_warn_unused bool validate_ascii(const char *buf,
                                          size_t len) const noexcept final;
  simdutf_warn_unused result
  validate_ascii_with_errors(const char *buf, size_t len) const noexcept final;
#endif // SIMDUTF_FEATURE_ASCII

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_ASCII
  simdutf_warn_unused bool
  validate_utf16le_as_ascii(const char16_t *buf,
                            size_t len) const noexcept final;

  simdutf_warn_unused bool
  validate_utf16be_as_ascii(const char16_t *buf,
                            size_t len) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_ASCII

#if SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool validate_utf16le(const char16_t *buf,
                                            size_t len) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
#if SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused bool validate_utf16be(const char16_t *buf,
                                            size_t len) const noexcept final;
  simdutf_warn_unused result validate_utf16le_with_errors(
      const char16_t *buf, size_t len) const noexcept final;
  simdutf_warn_unused result validate_utf16be_with_errors(
      const char16_t *buf, size_t len) const noexcept final;
  void to_well_formed_utf16be(const char16_t *input, size_t len,
                              char16_t *output) const noexcept final;
  void to_well_formed_utf16le(const char16_t *input, size_t len,
                              char16_t *output) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF16
#if SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool validate_utf32(const char32_t *buf,
                                          size_t len) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING
#if SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused result validate_utf32_with_errors(
      const char32_t *buf, size_t len) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF32
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_latin1_to_utf8(
      const char *buf, size_t len, char *utf8_output) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_latin1_to_utf16le(
      const char *buf, size_t len, char16_t *utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_latin1_to_utf16be(
      const char *buf, size_t len, char16_t *utf16_buffer) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_latin1_to_utf32(
      const char *buf, size_t len, char32_t *utf32_output) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_utf8_to_latin1(
      const char *buf, size_t len, char *latin1_output) const noexcept final;
  simdutf_warn_unused result convert_utf8_to_latin1_with_errors(
      const char *buf, size_t len, char *latin1_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf8_to_latin1(
      const char *buf, size_t len, char *latin1_output) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t convert_utf8_to_utf16le(
      const char *buf, size_t len, char16_t *utf16_output) const noexcept final;
  simdutf_warn_unused size_t convert_utf8_to_utf16be(
      const char *buf, size_t len, char16_t *utf16_output) const noexcept final;
  simdutf_warn_unused result convert_utf8_to_utf16le_with_errors(
      const char *buf, size_t len, char16_t *utf16_output) const noexcept final;
  simdutf_warn_unused result convert_utf8_to_utf16be_with_errors(
      const char *buf, size_t len, char16_t *utf16_output) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf8_to_utf16le(
      const char *buf, size_t len, char16_t *utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf8_to_utf16be(
      const char *buf, size_t len, char16_t *utf16_buffer) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t convert_utf8_to_utf32(
      const char *buf, size_t len, char32_t *utf32_output) const noexcept final;
  simdutf_warn_unused result convert_utf8_to_utf32_with_errors(
      const char *buf, size_t len, char32_t *utf32_output) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf8_to_utf32(
      const char *buf, size_t len, char32_t *utf32_buffer) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  convert_utf16le_to_latin1(const char16_t *buf, size_t len,
                            char *latin1_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_utf16be_to_latin1(const char16_t *buf, size_t len,
                            char *latin1_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf16le_to_latin1_with_errors(
      const char16_t *buf, size_t len,
      char *latin1_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf16be_to_latin1_with_errors(
      const char16_t *buf, size_t len,
      char *latin1_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_valid_utf16le_to_latin1(const char16_t *buf, size_t len,
                                  char *latin1_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_valid_utf16be_to_latin1(const char16_t *buf, size_t len,
                                  char *latin1_buffer) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t convert_utf16le_to_utf8(
      const char16_t *buf, size_t len, char *utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_utf16be_to_utf8(
      const char16_t *buf, size_t len, char *utf8_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf16le_to_utf8_with_errors(
      const char16_t *buf, size_t len, char *utf8_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf16be_to_utf8_with_errors(
      const char16_t *buf, size_t len, char *utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf16le_to_utf8(
      const char16_t *buf, size_t len, char *utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf16be_to_utf8(
      const char16_t *buf, size_t len, char *utf8_buffer) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  convert_utf32_to_latin1(const char32_t *buf, size_t len,
                          char *latin1_output) const noexcept final;
  simdutf_warn_unused result
  convert_utf32_to_latin1_with_errors(const char32_t *buf, size_t len,
                                      char *latin1_output) const noexcept final;
  simdutf_warn_unused size_t
  convert_valid_utf32_to_latin1(const char32_t *buf, size_t len,
                                char *latin1_output) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t convert_utf32_to_utf8(
      const char32_t *buf, size_t len, char *utf8_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf32_to_utf8_with_errors(
      const char32_t *buf, size_t len, char *utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf32_to_utf8(
      const char32_t *buf, size_t len, char *utf8_buffer) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  convert_utf32_to_utf16le(const char32_t *buf, size_t len,
                           char16_t *utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_utf32_to_utf16be(const char32_t *buf, size_t len,
                           char16_t *utf16_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf32_to_utf16le_with_errors(
      const char32_t *buf, size_t len,
      char16_t *utf16_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf32_to_utf16be_with_errors(
      const char32_t *buf, size_t len,
      char16_t *utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_valid_utf32_to_utf16le(const char32_t *buf, size_t len,
                                 char16_t *utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_valid_utf32_to_utf16be(const char32_t *buf, size_t len,
                                 char16_t *utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_utf16le_to_utf32(const char16_t *buf, size_t len,
                           char32_t *utf32_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_utf16be_to_utf32(const char16_t *buf, size_t len,
                           char32_t *utf32_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf16le_to_utf32_with_errors(
      const char16_t *buf, size_t len,
      char32_t *utf32_buffer) const noexcept final;
  simdutf_warn_unused result convert_utf16be_to_utf32_with_errors(
      const char16_t *buf, size_t len,
      char32_t *utf32_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_valid_utf16le_to_utf32(const char16_t *buf, size_t len,
                                 char32_t *utf32_buffer) const noexcept final;
  simdutf_warn_unused size_t
  convert_valid_utf16be_to_utf32(const char16_t *buf, size_t len,
                                 char32_t *utf32_buffer) const noexcept final;
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
#if SIMDUTF_FEATURE_UTF16
  void change_endianness_utf16(const char16_t *buf, size_t length,
                               char16_t *output) const noexcept final;
  simdutf_warn_unused size_t count_utf16le(const char16_t *buf,
                                           size_t length) const noexcept;
  simdutf_warn_unused size_t count_utf16be(const char16_t *buf,
                                           size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF16
#if SIMDUTF_FEATURE_UTF8
  simdutf_warn_unused size_t count_utf8(const char *buf,
                                        size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF8
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t
  utf8_length_from_utf16le(const char16_t *input, size_t length) const noexcept;
  simdutf_warn_unused size_t
  utf8_length_from_utf16be(const char16_t *input, size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t utf32_length_from_utf16le(
      const char16_t *input, size_t length) const noexcept;
  simdutf_warn_unused size_t utf32_length_from_utf16be(
      const char16_t *input, size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t
  utf16_length_from_utf8(const char *input, size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  utf8_length_from_utf32(const char32_t *input, size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  utf16_length_from_utf32(const char32_t *input, size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  utf32_length_from_utf8(const char *input, size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  latin1_length_from_utf8(const char *input, size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  utf8_length_from_latin1(const char *input, size_t length) const noexcept;
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
#if SIMDUTF_FEATURE_BASE64
  simdutf_warn_unused result base64_to_binary(
      const char *input, size_t length, char *output, base64_options options,
      last_chunk_handling_options last_chunk_options =
          last_chunk_handling_options::loose) const noexcept;
  simdutf_warn_unused full_result base64_to_binary_details(
      const char *input, size_t length, char *output, base64_options options,
      last_chunk_handling_options last_chunk_options =
          last_chunk_handling_options::loose) const noexcept;
  simdutf_warn_unused result
  base64_to_binary(const char16_t *input, size_t length, char *output,
                   base64_options options,
                   last_chunk_handling_options last_chunk_options =
                       last_chunk_handling_options::loose) const noexcept;
  simdutf_warn_unused full_result base64_to_binary_details(
      const char16_t *input, size_t length, char *output,
      base64_options options,
      last_chunk_handling_options last_chunk_options =
          last_chunk_handling_options::loose) const noexcept;
  size_t binary_to_base64(const char *input, size_t length, char *output,
                          base64_options options) const noexcept;
  const char *find(const char *start, const char *end,
                   char character) const noexcept;
  const char16_t *find(const char16_t *start, const char16_t *end,
                       char16_t character) const noexcept;
#endif // SIMDUTF_FEATURE_BASE64
};

} // namespace lasx
} // namespace simdutf

#endif // SIMDUTF_LASX_IMPLEMENTATION_H
