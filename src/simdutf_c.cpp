#include "simdutf/simdutf_c.h"
#include "simdutf/implementation.h"

static simdutf_result to_c_result(const simdutf::result &r) {
  simdutf_result out;
  out.error = static_cast<simdutf_error_code>(r.error);
  out.count = r.count;
  return out;
}

extern "C" {

bool simdutf_validate_utf8(const char *buf, size_t len) {
  return simdutf::validate_utf8(buf, len);
}

simdutf_result simdutf_validate_utf8_with_errors(const char *buf, size_t len) {
  return to_c_result(simdutf::validate_utf8_with_errors(buf, len));
}

simdutf_encoding_type simdutf_autodetect_encoding(const char *input,
                                                  size_t length) {
  return static_cast<simdutf_encoding_type>(
      simdutf::autodetect_encoding(input, length));
}

int simdutf_detect_encodings(const char *input, size_t length) {
  return simdutf::detect_encodings(input, length);
}

bool simdutf_validate_ascii(const char *buf, size_t len) {
  return simdutf::validate_ascii(buf, len);
}
simdutf_result simdutf_validate_ascii_with_errors(const char *buf, size_t len) {
  return to_c_result(simdutf::validate_ascii_with_errors(buf, len));
}

bool simdutf_validate_utf16_as_ascii(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16_as_ascii(
      reinterpret_cast<const char16_t *>(buf), len);
}
bool simdutf_validate_utf16be_as_ascii(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16be_as_ascii(
      reinterpret_cast<const char16_t *>(buf), len);
}
bool simdutf_validate_utf16le_as_ascii(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16le_as_ascii(
      reinterpret_cast<const char16_t *>(buf), len);
}

bool simdutf_validate_utf16(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16(reinterpret_cast<const char16_t *>(buf), len);
}
bool simdutf_validate_utf16le(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16le(reinterpret_cast<const char16_t *>(buf),
                                   len);
}
bool simdutf_validate_utf16be(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16be(reinterpret_cast<const char16_t *>(buf),
                                   len);
}
simdutf_result simdutf_validate_utf16_with_errors(const char16_t *buf,
                                                  size_t len) {
  return to_c_result(simdutf::validate_utf16_with_errors(
      reinterpret_cast<const char16_t *>(buf), len));
}
simdutf_result simdutf_validate_utf16le_with_errors(const char16_t *buf,
                                                    size_t len) {
  return to_c_result(simdutf::validate_utf16le_with_errors(
      reinterpret_cast<const char16_t *>(buf), len));
}
simdutf_result simdutf_validate_utf16be_with_errors(const char16_t *buf,
                                                    size_t len) {
  return to_c_result(simdutf::validate_utf16be_with_errors(
      reinterpret_cast<const char16_t *>(buf), len));
}

bool simdutf_validate_utf32(const char32_t *buf, size_t len) {
  return simdutf::validate_utf32(reinterpret_cast<const char32_t *>(buf), len);
}
simdutf_result simdutf_validate_utf32_with_errors(const char32_t *buf,
                                                  size_t len) {
  return to_c_result(simdutf::validate_utf32_with_errors(
      reinterpret_cast<const char32_t *>(buf), len));
}

void simdutf_to_well_formed_utf16le(const char16_t *input, size_t len,
                                    char16_t *output) {
  simdutf::to_well_formed_utf16le(reinterpret_cast<const char16_t *>(input),
                                  len, reinterpret_cast<char16_t *>(output));
}
void simdutf_to_well_formed_utf16be(const char16_t *input, size_t len,
                                    char16_t *output) {
  simdutf::to_well_formed_utf16be(reinterpret_cast<const char16_t *>(input),
                                  len, reinterpret_cast<char16_t *>(output));
}
void simdutf_to_well_formed_utf16(const char16_t *input, size_t len,
                                  char16_t *output) {
  simdutf::to_well_formed_utf16(reinterpret_cast<const char16_t *>(input), len,
                                reinterpret_cast<char16_t *>(output));
}

size_t simdutf_count_utf16(const char16_t *input, size_t length) {
  return simdutf::count_utf16(reinterpret_cast<const char16_t *>(input),
                              length);
}
size_t simdutf_count_utf16le(const char16_t *input, size_t length) {
  return simdutf::count_utf16le(reinterpret_cast<const char16_t *>(input),
                                length);
}
size_t simdutf_count_utf16be(const char16_t *input, size_t length) {
  return simdutf::count_utf16be(reinterpret_cast<const char16_t *>(input),
                                length);
}
size_t simdutf_count_utf8(const char *input, size_t length) {
  return simdutf::count_utf8(input, length);
}

size_t simdutf_utf8_length_from_latin1(const char *input, size_t length) {
  return simdutf::utf8_length_from_latin1(input, length);
}
size_t simdutf_latin1_length_from_utf8(const char *input, size_t length) {
  return simdutf::latin1_length_from_utf8(input, length);
}
size_t simdutf_latin1_length_from_utf16(size_t length) {
  return simdutf::latin1_length_from_utf16(length);
}
size_t simdutf_latin1_length_from_utf32(size_t length) {
  return simdutf::latin1_length_from_utf32(length);
}
size_t simdutf_utf16_length_from_utf8(const char *input, size_t length) {
  return simdutf::utf16_length_from_utf8(input, length);
}
size_t simdutf_utf32_length_from_utf8(const char *input, size_t length) {
  return simdutf::utf32_length_from_utf8(input, length);
}
size_t simdutf_utf8_length_from_utf16(const char16_t *input, size_t length) {
  return simdutf::utf8_length_from_utf16(
      reinterpret_cast<const char16_t *>(input), length);
}
simdutf_result
simdutf_utf8_length_from_utf16_with_replacement(const char16_t *input,
                                                size_t length) {
  return to_c_result(simdutf::utf8_length_from_utf16_with_replacement(
      reinterpret_cast<const char16_t *>(input), length));
}
size_t simdutf_utf8_length_from_utf16le(const char16_t *input, size_t length) {
  return simdutf::utf8_length_from_utf16le(
      reinterpret_cast<const char16_t *>(input), length);
}
size_t simdutf_utf8_length_from_utf16be(const char16_t *input, size_t length) {
  return simdutf::utf8_length_from_utf16be(
      reinterpret_cast<const char16_t *>(input), length);
}
simdutf_result
simdutf_utf8_length_from_utf16le_with_replacement(const char16_t *input,
                                                  size_t length) {
  return to_c_result(simdutf::utf8_length_from_utf16le_with_replacement(
      reinterpret_cast<const char16_t *>(input), length));
}
simdutf_result
simdutf_utf8_length_from_utf16be_with_replacement(const char16_t *input,
                                                  size_t length) {
  return to_c_result(simdutf::utf8_length_from_utf16be_with_replacement(
      reinterpret_cast<const char16_t *>(input), length));
}

/* Conversions: latin1 <-> utf8, utf8 <-> utf16/utf32, utf16 <-> utf8, etc. */
size_t simdutf_convert_latin1_to_utf8(const char *input, size_t length,
                                      char *output) {
  return simdutf::convert_latin1_to_utf8(input, length, output);
}
size_t simdutf_convert_latin1_to_utf16le(const char *input, size_t length,
                                         char16_t *output) {
  return simdutf::convert_latin1_to_utf16le(
      input, length, reinterpret_cast<char16_t *>(output));
}
size_t simdutf_convert_latin1_to_utf16be(const char *input, size_t length,
                                         char16_t *output) {
  return simdutf::convert_latin1_to_utf16be(
      input, length, reinterpret_cast<char16_t *>(output));
}
size_t simdutf_convert_latin1_to_utf32(const char *input, size_t length,
                                       char32_t *output) {
  return simdutf::convert_latin1_to_utf32(input, length,
                                          reinterpret_cast<char32_t *>(output));
}

size_t simdutf_convert_utf8_to_latin1(const char *input, size_t length,
                                      char *output) {
  return simdutf::convert_utf8_to_latin1(input, length, output);
}
size_t simdutf_convert_utf8_to_utf16le(const char *input, size_t length,
                                       char16_t *output) {
  return simdutf::convert_utf8_to_utf16le(input, length,
                                          reinterpret_cast<char16_t *>(output));
}
size_t simdutf_convert_utf8_to_utf16be(const char *input, size_t length,
                                       char16_t *output) {
  return simdutf::convert_utf8_to_utf16be(input, length,
                                          reinterpret_cast<char16_t *>(output));
}
size_t simdutf_convert_utf8_to_utf32(const char *input, size_t length,
                                     char32_t *output) {
  return simdutf::convert_utf8_to_utf32(input, length,
                                        reinterpret_cast<char32_t *>(output));
}
simdutf_result simdutf_convert_utf8_to_latin1_with_errors(const char *input,
                                                          size_t length,
                                                          char *output) {
  return to_c_result(
      simdutf::convert_utf8_to_latin1_with_errors(input, length, output));
}
simdutf_result simdutf_convert_utf8_to_utf16_with_errors(const char *input,
                                                         size_t length,
                                                         char16_t *output) {
  return to_c_result(simdutf::convert_utf8_to_utf16_with_errors(
      input, length, reinterpret_cast<char16_t *>(output)));
}
simdutf_result simdutf_convert_utf8_to_utf16le_with_errors(const char *input,
                                                           size_t length,
                                                           char16_t *output) {
  return to_c_result(simdutf::convert_utf8_to_utf16le_with_errors(
      input, length, reinterpret_cast<char16_t *>(output)));
}
simdutf_result simdutf_convert_utf8_to_utf16be_with_errors(const char *input,
                                                           size_t length,
                                                           char16_t *output) {
  return to_c_result(simdutf::convert_utf8_to_utf16be_with_errors(
      input, length, reinterpret_cast<char16_t *>(output)));
}
simdutf_result simdutf_convert_utf8_to_utf32_with_errors(const char *input,
                                                         size_t length,
                                                         char32_t *output) {
  return to_c_result(simdutf::convert_utf8_to_utf32_with_errors(
      input, length, reinterpret_cast<char32_t *>(output)));
}

/* Conversions assuming valid input */
size_t simdutf_convert_valid_utf8_to_latin1(const char *input, size_t length,
                                            char *output) {
  return simdutf::convert_valid_utf8_to_latin1(input, length, output);
}
size_t simdutf_convert_valid_utf8_to_utf16le(const char *input, size_t length,
                                             char16_t *output) {
  return simdutf::convert_valid_utf8_to_utf16le(
      input, length, reinterpret_cast<char16_t *>(output));
}
size_t simdutf_convert_valid_utf8_to_utf16be(const char *input, size_t length,
                                             char16_t *output) {
  return simdutf::convert_valid_utf8_to_utf16be(
      input, length, reinterpret_cast<char16_t *>(output));
}
size_t simdutf_convert_valid_utf8_to_utf32(const char *input, size_t length,
                                           char32_t *output) {
  return simdutf::convert_valid_utf8_to_utf32(
      input, length, reinterpret_cast<char32_t *>(output));
}

/* UTF-16 -> UTF-8 and related conversions */
size_t simdutf_convert_utf16_to_utf8(const char16_t *input, size_t length,
                                     char *output) {
  return simdutf::convert_utf16_to_utf8(
      reinterpret_cast<const char16_t *>(input), length, output);
}
size_t simdutf_convert_utf16_to_utf8_safe(const char16_t *input, size_t length,
                                          char *output, size_t utf8_len) {
  return simdutf::convert_utf16_to_utf8_safe(
      reinterpret_cast<const char16_t *>(input), length, output, utf8_len);
}
size_t simdutf_convert_utf16_to_latin1(const char16_t *input, size_t length,
                                       char *output) {
  return simdutf::convert_utf16_to_latin1(
      reinterpret_cast<const char16_t *>(input), length, output);
}
size_t simdutf_convert_utf16le_to_latin1(const char16_t *input, size_t length,
                                         char *output) {
  return simdutf::convert_utf16le_to_latin1(
      reinterpret_cast<const char16_t *>(input), length, output);
}
size_t simdutf_convert_utf16be_to_latin1(const char16_t *input, size_t length,
                                         char *output) {
  return simdutf::convert_utf16be_to_latin1(
      reinterpret_cast<const char16_t *>(input), length, output);
}
simdutf_result
simdutf_convert_utf16_to_latin1_with_errors(const char16_t *input,
                                            size_t length, char *output) {
  return to_c_result(simdutf::convert_utf16_to_latin1_with_errors(
      reinterpret_cast<const char16_t *>(input), length, output));
}
simdutf_result
simdutf_convert_utf16le_to_latin1_with_errors(const char16_t *input,
                                              size_t length, char *output) {
  return to_c_result(simdutf::convert_utf16le_to_latin1_with_errors(
      reinterpret_cast<const char16_t *>(input), length, output));
}
simdutf_result
simdutf_convert_utf16be_to_latin1_with_errors(const char16_t *input,
                                              size_t length, char *output) {
  return to_c_result(simdutf::convert_utf16be_to_latin1_with_errors(
      reinterpret_cast<const char16_t *>(input), length, output));
}

simdutf_result simdutf_convert_utf16_to_utf8_with_errors(const char16_t *input,
                                                         size_t length,
                                                         char *output) {
  return to_c_result(simdutf::convert_utf16_to_utf8_with_errors(
      reinterpret_cast<const char16_t *>(input), length, output));
}
simdutf_result
simdutf_convert_utf16le_to_utf8_with_errors(const char16_t *input,
                                            size_t length, char *output) {
  return to_c_result(simdutf::convert_utf16le_to_utf8_with_errors(
      reinterpret_cast<const char16_t *>(input), length, output));
}
simdutf_result
simdutf_convert_utf16be_to_utf8_with_errors(const char16_t *input,
                                            size_t length, char *output) {
  return to_c_result(simdutf::convert_utf16be_to_utf8_with_errors(
      reinterpret_cast<const char16_t *>(input), length, output));
}

size_t simdutf_convert_utf16le_to_utf8(const char16_t *input, size_t length,
                                       char *output) {
  return simdutf::convert_utf16le_to_utf8(
      reinterpret_cast<const char16_t *>(input), length, output);
}
size_t simdutf_convert_utf16be_to_utf8(const char16_t *input, size_t length,
                                       char *output) {
  return simdutf::convert_utf16be_to_utf8(
      reinterpret_cast<const char16_t *>(input), length, output);
}

size_t simdutf_convert_valid_utf16_to_utf8(const char16_t *input, size_t length,
                                           char *output) {
  return simdutf::convert_valid_utf16_to_utf8(
      reinterpret_cast<const char16_t *>(input), length, output);
}
size_t simdutf_convert_valid_utf16_to_latin1(const char16_t *input,
                                             size_t length, char *output) {
  return simdutf::convert_valid_utf16_to_latin1(
      reinterpret_cast<const char16_t *>(input), length, output);
}
size_t simdutf_convert_valid_utf16le_to_latin1(const char16_t *input,
                                               size_t length, char *output) {
  return simdutf::convert_valid_utf16le_to_latin1(
      reinterpret_cast<const char16_t *>(input), length, output);
}
size_t simdutf_convert_valid_utf16be_to_latin1(const char16_t *input,
                                               size_t length, char *output) {
  return simdutf::convert_valid_utf16be_to_latin1(
      reinterpret_cast<const char16_t *>(input), length, output);
}

size_t simdutf_convert_valid_utf16le_to_utf8(const char16_t *input,
                                             size_t length, char *output) {
  return simdutf::convert_valid_utf16le_to_utf8(
      reinterpret_cast<const char16_t *>(input), length, output);
}
size_t simdutf_convert_valid_utf16be_to_utf8(const char16_t *input,
                                             size_t length, char *output) {
  return simdutf::convert_valid_utf16be_to_utf8(
      reinterpret_cast<const char16_t *>(input), length, output);
}

/* UTF-16 <-> UTF-32 conversions */
size_t simdutf_convert_utf16_to_utf32(const char16_t *input, size_t length,
                                      char32_t *output) {
  return simdutf::convert_utf16_to_utf32(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output));
}
size_t simdutf_convert_utf16le_to_utf32(const char16_t *input, size_t length,
                                        char32_t *output) {
  return simdutf::convert_utf16le_to_utf32(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output));
}
size_t simdutf_convert_utf16be_to_utf32(const char16_t *input, size_t length,
                                        char32_t *output) {
  return simdutf::convert_utf16be_to_utf32(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output));
}
simdutf_result simdutf_convert_utf16_to_utf32_with_errors(const char16_t *input,
                                                          size_t length,
                                                          char32_t *output) {
  return to_c_result(simdutf::convert_utf16_to_utf32_with_errors(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output)));
}
simdutf_result
simdutf_convert_utf16le_to_utf32_with_errors(const char16_t *input,
                                             size_t length, char32_t *output) {
  return to_c_result(simdutf::convert_utf16le_to_utf32_with_errors(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output)));
}
simdutf_result
simdutf_convert_utf16be_to_utf32_with_errors(const char16_t *input,
                                             size_t length, char32_t *output) {
  return to_c_result(simdutf::convert_utf16be_to_utf32_with_errors(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output)));
}

/* Valid UTF-16 conversions */
size_t simdutf_convert_valid_utf16_to_utf32(const char16_t *input,
                                            size_t length, char32_t *output) {
  return simdutf::convert_valid_utf16_to_utf32(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output));
}
size_t simdutf_convert_valid_utf16le_to_utf32(const char16_t *input,
                                              size_t length, char32_t *output) {
  return simdutf::convert_valid_utf16le_to_utf32(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output));
}
size_t simdutf_convert_valid_utf16be_to_utf32(const char16_t *input,
                                              size_t length, char32_t *output) {
  return simdutf::convert_valid_utf16be_to_utf32(
      reinterpret_cast<const char16_t *>(input), length,
      reinterpret_cast<char32_t *>(output));
}

/* UTF-32 -> ... conversions */
size_t simdutf_convert_utf32_to_utf8(const char32_t *input, size_t length,
                                     char *output) {
  return simdutf::convert_utf32_to_utf8(
      reinterpret_cast<const char32_t *>(input), length, output);
}
simdutf_result simdutf_convert_utf32_to_utf8_with_errors(const char32_t *input,
                                                         size_t length,
                                                         char *output) {
  return to_c_result(simdutf::convert_utf32_to_utf8_with_errors(
      reinterpret_cast<const char32_t *>(input), length, output));
}
size_t simdutf_convert_valid_utf32_to_utf8(const char32_t *input, size_t length,
                                           char *output) {
  return simdutf::convert_valid_utf32_to_utf8(
      reinterpret_cast<const char32_t *>(input), length, output);
}

size_t simdutf_convert_utf32_to_utf16(const char32_t *input, size_t length,
                                      char16_t *output) {
  return simdutf::convert_utf32_to_utf16(
      reinterpret_cast<const char32_t *>(input), length,
      reinterpret_cast<char16_t *>(output));
}
size_t simdutf_convert_utf32_to_utf16le(const char32_t *input, size_t length,
                                        char16_t *output) {
  return simdutf::convert_utf32_to_utf16le(
      reinterpret_cast<const char32_t *>(input), length,
      reinterpret_cast<char16_t *>(output));
}
size_t simdutf_convert_utf32_to_utf16be(const char32_t *input, size_t length,
                                        char16_t *output) {
  return simdutf::convert_utf32_to_utf16be(
      reinterpret_cast<const char32_t *>(input), length,
      reinterpret_cast<char16_t *>(output));
}
simdutf_result
simdutf_convert_utf32_to_latin1_with_errors(const char32_t *input,
                                            size_t length, char *output) {
  return to_c_result(simdutf::convert_utf32_to_latin1_with_errors(
      reinterpret_cast<const char32_t *>(input), length, output));
}

/* --- find helpers --- */
const char *simdutf_find(const char *start, const char *end, char character) {
  return simdutf::find(start, end, character);
}
const char16_t *simdutf_find_utf16(const char16_t *start, const char16_t *end,
                                   char16_t character) {
  return simdutf::find(start, end, character);
}

/* --- base64 helpers --- */
size_t simdutf_maximal_binary_length_from_base64(const char *input,
                                                 size_t length) {
  return simdutf::maximal_binary_length_from_base64(input, length);
}
size_t simdutf_maximal_binary_length_from_base64_utf16(const char16_t *input,
                                                       size_t length) {
  return simdutf::maximal_binary_length_from_base64(input, length);
}

simdutf_result simdutf_base64_to_binary(
    const char *input, size_t length, char *output,
    simdutf_base64_options options,
    simdutf_last_chunk_handling_options last_chunk_options) {
  return to_c_result(simdutf::base64_to_binary(
      input, length, output, static_cast<simdutf::base64_options>(options),
      static_cast<simdutf::last_chunk_handling_options>(last_chunk_options)));
}
simdutf_result simdutf_base64_to_binary_utf16(
    const char16_t *input, size_t length, char *output,
    simdutf_base64_options options,
    simdutf_last_chunk_handling_options last_chunk_options) {
  return to_c_result(simdutf::base64_to_binary(
      input, length, output, static_cast<simdutf::base64_options>(options),
      static_cast<simdutf::last_chunk_handling_options>(last_chunk_options)));
}

size_t simdutf_base64_length_from_binary(size_t length,
                                         simdutf_base64_options options) {
  return simdutf::base64_length_from_binary(
      length, static_cast<simdutf::base64_options>(options));
}
size_t simdutf_base64_length_from_binary_with_lines(
    size_t length, simdutf_base64_options options, size_t line_length) {
  return simdutf::base64_length_from_binary_with_lines(
      length, static_cast<simdutf::base64_options>(options), line_length);
}

size_t simdutf_binary_to_base64(const char *input, size_t length, char *output,
                                simdutf_base64_options options) {
  return simdutf::binary_to_base64(
      input, length, output, static_cast<simdutf::base64_options>(options));
}
size_t simdutf_binary_to_base64_with_lines(const char *input, size_t length,
                                           char *output, size_t line_length,
                                           simdutf_base64_options options) {
  return simdutf::binary_to_base64_with_lines(
      input, length, output, line_length,
      static_cast<simdutf::base64_options>(options));
}

simdutf_result simdutf_base64_to_binary_safe(
    const char *input, size_t length, char *output, size_t *outlen,
    simdutf_base64_options options,
    simdutf_last_chunk_handling_options last_chunk_options,
    bool decode_up_to_bad_char) {
  size_t local_out = outlen ? *outlen : 0;
  simdutf::result r = simdutf::base64_to_binary_safe(
      input, length, output, local_out,
      static_cast<simdutf::base64_options>(options),
      static_cast<simdutf::last_chunk_handling_options>(last_chunk_options),
      decode_up_to_bad_char);
  if (outlen)
    *outlen = local_out;
  return to_c_result(r);
}
simdutf_result simdutf_base64_to_binary_safe_utf16(
    const char16_t *input, size_t length, char *output, size_t *outlen,
    simdutf_base64_options options,
    simdutf_last_chunk_handling_options last_chunk_options,
    bool decode_up_to_bad_char) {
  size_t local_out = outlen ? *outlen : 0;
  simdutf::result r = simdutf::base64_to_binary_safe(
      input, length, output, local_out,
      static_cast<simdutf::base64_options>(options),
      static_cast<simdutf::last_chunk_handling_options>(last_chunk_options),
      decode_up_to_bad_char);
  if (outlen)
    *outlen = local_out;
  return to_c_result(r);
}

} // extern "C"
