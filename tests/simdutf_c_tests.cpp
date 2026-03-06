#include <tests/helpers/test.h>
#include <cstring>
#include "simdutf_c.h"

const char *hello = "hello";
const size_t hello_len = 5;

TEST(validate_utf8_c) {
  bool ok = simdutf_validate_utf8(hello, hello_len);
  ASSERT_TRUE(ok);
  simdutf_result r = simdutf_validate_utf8_with_errors(hello, hello_len);
  ASSERT_EQUAL(r.error, SIMDUTF_ERROR_SUCCESS);
  ASSERT_EQUAL(r.count, hello_len);
}

TEST(convert_utf8_to_utf16_c) {
  char16_t out[16];
  size_t n = simdutf_convert_utf8_to_utf16(hello, hello_len, out);
  ASSERT_EQUAL(n, hello_len);
  for (size_t i = 0; i < n; i++) {
    ASSERT_EQUAL(out[i], char16_t(hello[i]));
  }
}

TEST(convert_utf8_to_utf32_c) {
  char32_t out[16];
  size_t n = simdutf_convert_utf8_to_utf32(hello, hello_len, out);
  ASSERT_EQUAL(n, hello_len);
  for (size_t i = 0; i < n; i++) {
    ASSERT_EQUAL(out[i], char32_t(hello[i]));
  }
}

TEST(count_utf8_c) {
  size_t cnt = simdutf_count_utf8(hello, hello_len);
  ASSERT_EQUAL(cnt, hello_len);
}

TEST(find_c) {
  const char *f = simdutf_find(hello, hello + hello_len, 'e');
  ASSERT_EQUAL(f, hello + 1);
}

TEST(base64_c) {
  const char *b64 = "aGVsbG8="; // "hello"
  char binout[16] = {0};
  size_t outlen = sizeof(binout);
  simdutf_result br = simdutf_base64_to_binary_safe(
      b64, strlen(b64), binout, &outlen, SIMDUTF_BASE64_DEFAULT,
      SIMDUTF_LAST_CHUNK_LOOSE, true);
  ASSERT_EQUAL(br.error, SIMDUTF_ERROR_SUCCESS);
  ASSERT_EQUAL(outlen, hello_len);
  ASSERT_TRUE(std::memcmp(binout, "hello", hello_len) == 0);

  char b64out[32] = {0};
  size_t b64len = simdutf_binary_to_base64("hello", hello_len, b64out,
                                           SIMDUTF_BASE64_DEFAULT);
  ASSERT_TRUE(b64len >= 8);
}

TEST(ascii_and_detect_c) {
  ASSERT_TRUE(simdutf_validate_ascii(hello, hello_len));
  simdutf_result r = simdutf_validate_ascii_with_errors(hello, hello_len);
  ASSERT_EQUAL(r.error, SIMDUTF_ERROR_SUCCESS);

  simdutf_encoding_type et = simdutf_autodetect_encoding(hello, hello_len);
  ASSERT_EQUAL(et, SIMDUTF_ENCODING_UTF8);
  int encs = simdutf_detect_encodings(hello, hello_len);
  ASSERT_TRUE(encs >= 0);
}

TEST(lengths_and_conversions_c) {
  char latin_out[8] = {0};
  size_t latin_to_utf8 = simdutf_convert_latin1_to_utf8("abc", 3, latin_out);
  ASSERT_EQUAL(latin_to_utf8, 3);

  // prepare a UTF-16 sample
  char16_t u16[5] = {u'h', u'e', u'l', u'l', u'o'};
  size_t u16len = simdutf_utf8_length_from_utf16(u16, 5);

  // convert utf16->utf8 safe
  char out8[8] = {0};
  size_t safelen =
      simdutf_convert_utf16_to_utf8_safe(u16, 5, out8, sizeof(out8));
  ASSERT_EQUAL(safelen, u16len);

  // convert with errors
  simdutf_result cr = simdutf_convert_utf16_to_utf8_with_errors(u16, 5, out8);
  ASSERT_EQUAL(cr.error, SIMDUTF_ERROR_SUCCESS);
}

TEST(counts_and_find_utf16_c) {
  char16_t u16[5] = {u'h', u'e', u'l', u'l', u'o'};
  size_t c16 = simdutf_count_utf16(u16, 5);
  ASSERT_EQUAL(c16, 5);

  const char16_t *f16 = simdutf_find_utf16(u16, u16 + 5, u'e');
  if (f16 == nullptr) {
    ASSERT_TRUE(false); // should not be null
  } else {
    if (f16 != u16 + 1) {
      ASSERT_TRUE(false); // should point to second character
    }
  }
}

TEST(base64_length_helpers_c) {
  size_t maxbin = simdutf_maximal_binary_length_from_base64("aGVsbG8=", 8);
  ASSERT_TRUE(maxbin >= 5);

  size_t b64len = simdutf_base64_length_from_binary(5, SIMDUTF_BASE64_DEFAULT);
  ASSERT_TRUE(b64len >= 8);

  size_t with_lines = simdutf_base64_length_from_binary_with_lines(
      5, SIMDUTF_BASE64_DEFAULT, 4);
  ASSERT_TRUE(with_lines >= b64len);

  char out[64] = {0};
  size_t bl = simdutf_binary_to_base64_with_lines("hello", 5, out, 4,
                                                  SIMDUTF_BASE64_DEFAULT);
  ASSERT_TRUE(bl > 0);
}

TEST_MAIN
