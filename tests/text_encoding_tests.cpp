#include "simdutf.h"

#include <tests/helpers/test.h>

#ifdef SIMDUTF_HAS_STD_TEXT_ENCODING

TEST(to_std_encoding_unspecified) {
  constexpr auto enc = simdutf::to_std_encoding(simdutf::unspecified);
  static_assert(enc.mib() == std::text_encoding::id::unknown,
                "unexpected unknown encoding");
}

TEST(from_std_encoding_unsupported) {
  constexpr std::text_encoding enc(std::text_encoding::id::ShiftJIS);
  constexpr auto result = simdutf::from_std_encoding(enc);
  static_assert(result == simdutf::unspecified,
                "unsupported encodings map to unspecified");
}

TEST(roundtrip_conversions) {
  static_assert(simdutf::from_std_encoding(
                    simdutf::to_std_encoding(simdutf::UTF8)) == simdutf::UTF8,
                "UTF8 roundtrip failed");
  static_assert(simdutf::from_std_encoding(simdutf::to_std_encoding(
                    simdutf::UTF16_LE)) == simdutf::UTF16_LE,
                "UTF16_LE roundtrip failed");
  static_assert(simdutf::from_std_encoding(simdutf::to_std_encoding(
                    simdutf::UTF16_BE)) == simdutf::UTF16_BE,
                "UTF16_BE roundtrip failed");
  static_assert(simdutf::from_std_encoding(simdutf::to_std_encoding(
                    simdutf::UTF32_LE)) == simdutf::UTF32_LE,
                "UTF32_LE roundtrip failed");
  static_assert(simdutf::from_std_encoding(simdutf::to_std_encoding(
                    simdutf::UTF32_BE)) == simdutf::UTF32_BE,
                "UTF32_BE roundtrip failed");
  static_assert(simdutf::from_std_encoding(simdutf::to_std_encoding(
                    simdutf::Latin1)) == simdutf::Latin1,
                "Latin1 roundtrip failed");
}

TEST(from_std_encoding_native_utf16) {
  constexpr std::text_encoding enc(std::text_encoding::id::UTF16);
  constexpr auto result = simdutf::from_std_encoding_native(enc);
  #if SIMDUTF_IS_BIG_ENDIAN
  static_assert(result == simdutf::UTF16_BE, "native UTF16 mapping mismatch");
  #else
  static_assert(result == simdutf::UTF16_LE, "native UTF16 mapping mismatch");
  #endif
}

TEST(from_std_encoding_native_utf32) {
  constexpr std::text_encoding enc(std::text_encoding::id::UTF32);
  constexpr auto result = simdutf::from_std_encoding_native(enc);
  #if SIMDUTF_IS_BIG_ENDIAN
  static_assert(result == simdutf::UTF32_BE, "native UTF32 mapping mismatch");
  #else
  static_assert(result == simdutf::UTF32_LE, "native UTF32 mapping mismatch");
  #endif
}

TEST(from_std_encoding_native_explicit_endian) {
  constexpr std::text_encoding enc_le(std::text_encoding::id::UTF16LE);
  constexpr std::text_encoding enc_be(std::text_encoding::id::UTF16BE);
  static_assert(simdutf::from_std_encoding_native(enc_le) == simdutf::UTF16_LE,
                "UTF16LE mapping mismatch");
  static_assert(simdutf::from_std_encoding_native(enc_be) == simdutf::UTF16_BE,
                "UTF16BE mapping mismatch");
}

#else
TEST(text_encoding_not_available) {
  // this test passes when c++26 not available
  ASSERT_TRUE(true);
}
#endif // SIMDUTF_HAS_STD_TEXT_ENCODING

TEST_MAIN
