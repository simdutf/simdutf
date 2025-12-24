#include "simdutf.h"

#include <tests/helpers/test.h>

#ifdef SIMDUTF_HAS_STD_TEXT_ENCODING

TEST(to_std_encoding_unspecified) {
  auto enc = simdutf::to_std_encoding(simdutf::unspecified);
  ASSERT_TRUE(enc.mib() == std::text_encoding::id::unknown);
}

TEST(from_std_encoding_unsupported) {
  std::text_encoding enc(std::text_encoding::id::ShiftJIS);
  ASSERT_EQUAL(simdutf::from_std_encoding(enc), simdutf::unspecified);
}

TEST(roundtrip_conversions) {
  const simdutf::encoding_type encodings[] = {
      simdutf::UTF8,    simdutf::UTF16_LE, simdutf::UTF16_BE,
      simdutf::UTF32_LE, simdutf::UTF32_BE, simdutf::Latin1};

  for (auto enc : encodings) {
    auto std_enc = simdutf::to_std_encoding(enc);
    auto back = simdutf::from_std_encoding(std_enc);
    ASSERT_EQUAL(back, enc);
  }
}

TEST(native_utf16_encoding) {
  auto native = simdutf::native_utf16_encoding();
#if SIMDUTF_IS_BIG_ENDIAN
  ASSERT_EQUAL(native, simdutf::UTF16_BE);
#else
  ASSERT_EQUAL(native, simdutf::UTF16_LE);
#endif
}

TEST(native_utf32_encoding) {
  auto native = simdutf::native_utf32_encoding();
#if SIMDUTF_IS_BIG_ENDIAN
  ASSERT_EQUAL(native, simdutf::UTF32_BE);
#else
  ASSERT_EQUAL(native, simdutf::UTF32_LE);
#endif
}

TEST(from_std_encoding_native_utf16) {
  std::text_encoding enc(std::text_encoding::id::UTF16);
  auto result = simdutf::from_std_encoding_native(enc);
#if SIMDUTF_IS_BIG_ENDIAN
  ASSERT_EQUAL(result, simdutf::UTF16_BE);
#else
  ASSERT_EQUAL(result, simdutf::UTF16_LE);
#endif
}

TEST(from_std_encoding_native_utf32) {
  std::text_encoding enc(std::text_encoding::id::UTF32);
  auto result = simdutf::from_std_encoding_native(enc);
#if SIMDUTF_IS_BIG_ENDIAN
  ASSERT_EQUAL(result, simdutf::UTF32_BE);
#else
  ASSERT_EQUAL(result, simdutf::UTF32_LE);
#endif
}

TEST(from_std_encoding_native_explicit_endian) {
  std::text_encoding enc_le(std::text_encoding::id::UTF16LE);
  std::text_encoding enc_be(std::text_encoding::id::UTF16BE);
  ASSERT_EQUAL(simdutf::from_std_encoding_native(enc_le), simdutf::UTF16_LE);
  ASSERT_EQUAL(simdutf::from_std_encoding_native(enc_be), simdutf::UTF16_BE);
}

TEST(constexpr_to_std_encoding) {
  constexpr auto enc = simdutf::to_std_encoding(simdutf::UTF8);
  static_assert(enc.mib() == std::text_encoding::id::UTF8, "UTF8 mismatch");
  ASSERT_TRUE(true);
}

TEST(constexpr_from_std_encoding) {
  constexpr std::text_encoding enc(std::text_encoding::id::UTF8);
  constexpr auto result = simdutf::from_std_encoding(enc);
  static_assert(result == simdutf::UTF8, "UTF8 roundtrip failed");
  ASSERT_TRUE(true);
}

TEST(constexpr_native_helpers) {
  constexpr auto native16 = simdutf::native_utf16_encoding();
  constexpr auto native32 = simdutf::native_utf32_encoding();
  static_assert(native16 == simdutf::UTF16_LE || native16 == simdutf::UTF16_BE, "Invalid native UTF16");
  static_assert(native32 == simdutf::UTF32_LE || native32 == simdutf::UTF32_BE, "Invalid native UTF32");
  ASSERT_TRUE(true);
}

#else
TEST(text_encoding_not_available) {
  // this test passes when c++26 not available
  ASSERT_TRUE(true);
}
#endif // SIMDUTF_HAS_STD_TEXT_ENCODING

int main(int argc, char *argv[]) { return simdutf::test::main(argc, argv); }
