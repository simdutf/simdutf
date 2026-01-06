#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simdutf_c.h"

int main(void) {
  printf("SIMDUTF C API demo\n");
  const char *source = "1234";
  /* validate UTF-8 */
  if (!simdutf_validate_utf8(source, 4)) {
    puts("invalid UTF-8");
    return EXIT_FAILURE;
  }
  puts("valid UTF-8");

  /* Convert UTF-8 -> UTF-16LE */
  size_t expected_utf16 = simdutf_utf16_length_from_utf8(source, 4);
  char16_t *utf16_output =
      (char16_t *)malloc(expected_utf16 * sizeof(char16_t));
  if (!utf16_output)
    return EXIT_FAILURE;
  size_t utf16words = simdutf_convert_utf8_to_utf16le(source, 4, utf16_output);
  printf("wrote %zu UTF-16LE words.\n", utf16words);

  if (!simdutf_validate_utf16le(utf16_output, utf16words)) {
    puts("invalid UTF-16LE");
    free(utf16_output);
    return EXIT_FAILURE;
  }
  puts("valid UTF-16LE");

  /* Convert back UTF-16LE -> UTF-8 */
  size_t expected_utf8 =
      simdutf_utf8_length_from_utf16le(utf16_output, utf16words);
  char *utf8_output = (char *)malloc(expected_utf8 + 1);
  if (!utf8_output) {
    free(utf16_output);
    return EXIT_FAILURE;
  }
  size_t utf8words =
      simdutf_convert_utf16le_to_utf8(utf16_output, utf16words, utf8_output);
  utf8_output[utf8words] = '\0';
  printf("wrote %zu UTF-8 words.\n", utf8words);
  puts(utf8_output);

  if (strcmp(utf8_output, source) != 0) {
    puts("bad conversion");
    free(utf16_output);
    free(utf8_output);
    return EXIT_FAILURE;
  } else {
    puts("perfect round trip");
  }

  free(utf16_output);
  free(utf8_output);
  return EXIT_SUCCESS;
}
