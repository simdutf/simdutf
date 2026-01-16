#include "simdutf.h"
#include <iostream>
#include <array>
#include <tests/helpers/test.h>

#if SIMDUTF_CPLUSPLUS23

consteval simdutf::encoding_type get_encoding(std::string_view s) {
  return simdutf::autodetect_encoding(s);
}

consteval int get_encodings(std::string_view s) {
  return simdutf::detect_encodings(s);
}

TEST(constexpr_autodetect) {
  constexpr std::u8string_view hello = u8"Hello world";
  static_assert(simdutf::autodetect_encoding(hello) ==
                simdutf::encoding_type::UTF8);
  // BOM checks
  // UTF-8 BOM
  static constexpr char utf8_bom[] = "\xEF\xBB\xBF"
                                     "abc";
  constexpr std::string_view utf8_bom_sv(utf8_bom, 6);

  static_assert(simdutf::autodetect_encoding(utf8_bom_sv) ==
                simdutf::encoding_type::UTF8);

  // UTF-16LE BOM
  // 0xFF 0xFE is LE BOM. 'a' is 0x61. 0x00. -> 0x0061 (a).
  static constexpr char utf16le_bom_char[] = "\xFF\xFE\x61\x00";
  constexpr std::string_view utf16le_bom_sv(utf16le_bom_char, 4);
  static_assert(simdutf::autodetect_encoding(utf16le_bom_sv) ==
                simdutf::encoding_type::UTF16_LE);

  // UTF-32LE BOM
  // FF FE 00 00
  static constexpr char utf32le_bom_char[] = "\xFF\xFE\x00\x00\x61\x00\x00\x00";
  constexpr std::string_view utf32le_bom_sv(utf32le_bom_char, 8);
  static_assert(simdutf::autodetect_encoding(utf32le_bom_sv) ==
                simdutf::encoding_type::UTF32_LE);
}

TEST(constexpr_detect_encodings) {
  constexpr std::u8string_view hello = u8"Hello world";
  static_assert((simdutf::detect_encodings(hello) &
                 simdutf::encoding_type::UTF8) == simdutf::encoding_type::UTF8);
}
#else
TEST(constexpr_autodetect_dummy) {
  std::cout << "SIMDUTF_CPLUSPLUS: " << SIMDUTF_CPLUSPLUS << std::endl;
  ASSERT_TRUE(true);
}
#endif

TEST_MAIN
