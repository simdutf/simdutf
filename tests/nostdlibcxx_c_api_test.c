/*
 * Smoke test for the simdutf C API against a build of simdutf that does not
 * depend on the C++ standard library. Compiled as C and linked with
 * -nostdlib++ against libsimdutf-nostdlibcxx.a.
 */

#include "simdutf_c.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define EXPECT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s (line %d)\n", #cond, __LINE__);                \
      failures++;                                                              \
    }                                                                          \
  } while (0)

int main(void) {
  /* --- UTF-8 validation --- */
  const char ascii[] = "Hello, simdutf!";
  EXPECT(simdutf_validate_utf8(ascii, sizeof(ascii) - 1));

  const char bad_utf8[] = "\xff\xfe\xfd";
  EXPECT(!simdutf_validate_utf8(bad_utf8, sizeof(bad_utf8) - 1));

  simdutf_result r = simdutf_validate_utf8_with_errors(ascii, sizeof(ascii) - 1);
  EXPECT(r.error == SIMDUTF_ERROR_SUCCESS);
  EXPECT(r.count == sizeof(ascii) - 1);

  /* --- Encoding detection --- */
  EXPECT(simdutf_autodetect_encoding(ascii, sizeof(ascii) - 1) ==
         SIMDUTF_ENCODING_UTF8);

  /* --- ASCII validation --- */
  EXPECT(simdutf_validate_ascii(ascii, sizeof(ascii) - 1));

  /* --- UTF-8 -> UTF-16LE transcoding --- */
  const char hello_u8[] = "Hello";
  char16_t u16[16];
  size_t n = simdutf_convert_utf8_to_utf16le(hello_u8, sizeof(hello_u8) - 1, u16);
  EXPECT(n == 5);
  EXPECT(u16[0] == 'H' && u16[4] == 'o');

  /* --- UTF-16 -> UTF-8 transcoding --- */
  char back[16];
  size_t m = simdutf_convert_utf16le_to_utf8(u16, n, back);
  EXPECT(m == 5);
  EXPECT(memcmp(back, hello_u8, 5) == 0);

  /* --- Base64 round-trip --- */
  const char binary_in[] = "simdutf rocks!";
  char b64[64];
  size_t b64_len = simdutf_binary_to_base64(binary_in, sizeof(binary_in) - 1,
                                            b64, SIMDUTF_BASE64_DEFAULT);
  EXPECT(b64_len > 0);

  char decoded[64];
  simdutf_result dr = simdutf_base64_to_binary(
      b64, b64_len, decoded, SIMDUTF_BASE64_DEFAULT, SIMDUTF_LAST_CHUNK_LOOSE);
  EXPECT(dr.error == SIMDUTF_ERROR_SUCCESS);
  EXPECT(dr.count == sizeof(binary_in) - 1);
  EXPECT(memcmp(decoded, binary_in, sizeof(binary_in) - 1) == 0);

  /* --- Length estimators --- */
  EXPECT(simdutf_utf16_length_from_utf8(hello_u8, sizeof(hello_u8) - 1) == 5);
  EXPECT(simdutf_count_utf8(hello_u8, sizeof(hello_u8) - 1) == 5);

  if (failures == 0) {
    printf("nostdlibcxx_c_api_test: all checks passed\n");
    return 0;
  }
  fprintf(stderr, "nostdlibcxx_c_api_test: %d failure(s)\n", failures);
  return 1;
}
