#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "simdutf_c.h"

#define ASSERT_TRUE(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      return 1;                                                                \
    }                                                                          \
  } while (0)
#define ASSERT_EQUAL_SIZE_T(a, b)                                              \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      fprintf(stderr, "FAIL %s:%d: %s != %s (got %zu vs %zu)\n", __FILE__,     \
              __LINE__, #a, #b, (size_t)(a), (size_t)(b));                     \
      return 1;                                                                \
    }                                                                          \
  } while (0)
#define ASSERT_EQUAL_INT(a, b)                                                 \
  do {                                                                         \
    if ((int)(a) != (int)(b)) {                                                \
      fprintf(stderr, "FAIL %s:%d: %s != %s (got %d vs %d)\n", __FILE__,       \
              __LINE__, #a, #b, (int)(a), (int)(b));                           \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static const char *hello = "hello";
static const size_t hello_len = 5;

static int test_validate_utf8_c(void) {
  bool ok = simdutf_validate_utf8(hello, hello_len);
  ASSERT_TRUE(ok);
  simdutf_result r = simdutf_validate_utf8_with_errors(hello, hello_len);
  ASSERT_EQUAL_INT(r.error, SIMDUTF_ERROR_SUCCESS);
  ASSERT_EQUAL_SIZE_T(r.count, hello_len);
  return 0;
}

static int test_convert_utf8_to_utf16_c(void) {
  char16_t out[16];
  size_t n = simdutf_convert_utf8_to_utf16(hello, hello_len, out);
  ASSERT_EQUAL_SIZE_T(n, hello_len);
  for (size_t i = 0; i < n; i++) {
    ASSERT_TRUE(out[i] == (char16_t)hello[i]);
  }
  return 0;
}

static int test_convert_utf8_to_utf32_c(void) {
  char32_t out[16];
  size_t n = simdutf_convert_utf8_to_utf32(hello, hello_len, out);
  ASSERT_EQUAL_SIZE_T(n, hello_len);
  for (size_t i = 0; i < n; i++) {
    ASSERT_TRUE(out[i] == (char32_t)hello[i]);
  }
  return 0;
}

static int test_count_utf8_c(void) {
  size_t cnt = simdutf_count_utf8(hello, hello_len);
  ASSERT_EQUAL_SIZE_T(cnt, hello_len);
  return 0;
}

static int test_find_c(void) {
  const char *f = simdutf_find(hello, hello + hello_len, 'e');
  ASSERT_TRUE(f == hello + 1);
  return 0;
}

static int test_base64_c(void) {
  const char *b64 = "aGVsbG8="; /* "hello" */
  char binout[16] = {0};
  size_t outlen = sizeof(binout);
  simdutf_result br = simdutf_base64_to_binary_safe(
      b64, strlen(b64), binout, &outlen, SIMDUTF_BASE64_DEFAULT,
      SIMDUTF_LAST_CHUNK_LOOSE, true);
  ASSERT_EQUAL_INT(br.error, SIMDUTF_ERROR_SUCCESS);
  ASSERT_EQUAL_SIZE_T(outlen, hello_len);
  ASSERT_TRUE(memcmp(binout, "hello", hello_len) == 0);

  char b64out[32] = {0};
  size_t b64len = simdutf_binary_to_base64("hello", hello_len, b64out,
                                           SIMDUTF_BASE64_DEFAULT);
  ASSERT_TRUE(b64len >= 8);
  return 0;
}

static int test_ascii_and_detect_c(void) {
  ASSERT_TRUE(simdutf_validate_ascii(hello, hello_len));
  simdutf_result r = simdutf_validate_ascii_with_errors(hello, hello_len);
  ASSERT_EQUAL_INT(r.error, SIMDUTF_ERROR_SUCCESS);

  simdutf_encoding_type et = simdutf_autodetect_encoding(hello, hello_len);
  ASSERT_EQUAL_INT(et, SIMDUTF_ENCODING_UTF8);
  int encs = simdutf_detect_encodings(hello, hello_len);
  ASSERT_TRUE(encs >= 0);
  return 0;
}

static int test_lengths_and_conversions_c(void) {
  char latin_out[8] = {0};
  size_t latin_to_utf8 = simdutf_convert_latin1_to_utf8("abc", 3, latin_out);
  ASSERT_EQUAL_SIZE_T(latin_to_utf8, 3);

  /* prepare a UTF-16 sample */
  char16_t u16[5] = {(char16_t)'h', (char16_t)'e', (char16_t)'l', (char16_t)'l',
                     (char16_t)'o'};
  size_t u16len = simdutf_utf8_length_from_utf16(u16, 5);

  /* convert utf16->utf8 safe */
  char out8[8] = {0};
  size_t safelen =
      simdutf_convert_utf16_to_utf8_safe(u16, 5, out8, sizeof(out8));
  ASSERT_EQUAL_SIZE_T(safelen, u16len);

  /* convert with errors */
  simdutf_result cr = simdutf_convert_utf16_to_utf8_with_errors(u16, 5, out8);
  ASSERT_EQUAL_INT(cr.error, SIMDUTF_ERROR_SUCCESS);
  return 0;
}

static int test_counts_and_find_utf16_c(void) {
  char16_t u16[5] = {(char16_t)'h', (char16_t)'e', (char16_t)'l', (char16_t)'l',
                     (char16_t)'o'};
  size_t c16 = simdutf_count_utf16(u16, 5);
  ASSERT_EQUAL_SIZE_T(c16, 5);

  const char16_t *f16 = simdutf_find_utf16(u16, u16 + 5, (char16_t)'e');
  ASSERT_TRUE(f16 != NULL);
  ASSERT_TRUE(f16 == u16 + 1);
  return 0;
}

static int test_base64_length_helpers_c(void) {
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
  return 0;
}

int main(void) {
  struct {
    const char *name;
    int (*fn)(void);
  } tests[] = {{"validate_utf8_c", test_validate_utf8_c},
               {"convert_utf8_to_utf16_c", test_convert_utf8_to_utf16_c},
               {"convert_utf8_to_utf32_c", test_convert_utf8_to_utf32_c},
               {"count_utf8_c", test_count_utf8_c},
               {"find_c", test_find_c},
               {"base64_c", test_base64_c},
               {"ascii_and_detect_c", test_ascii_and_detect_c},
               {"lengths_and_conversions_c", test_lengths_and_conversions_c},
               {"counts_and_find_utf16_c", test_counts_and_find_utf16_c},
               {"base64_length_helpers_c", test_base64_length_helpers_c},
               {NULL, NULL}};

  for (int i = 0; tests[i].name != NULL; i++) {
    printf("%s... ", tests[i].name);
    fflush(stdout);
    int r = tests[i].fn();
    if (r != 0) {
      printf("FAILED\n");
      return 1;
    }
    printf("OK\n");
  }

  printf("All tests passed.\n");
  return 0;
}
