#include "simdutf.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  const char *source = "1234";
  // 4 == strlen(source)
  bool validutf8 = simdutf::validate_utf8(source, 4);
  if (validutf8) {
    printf("valid UTF-8\n");
  } else {
    printf("invalid UTF-8\n");
    return EXIT_FAILURE;
  }
  // We need a buffer of size where to write the UTF-16LE words.
  size_t expected_utf16words = simdutf::utf16_length_from_utf8(source, 4);
  char16_t *utf16_output = (char16_t *)malloc(sizeof(char16_t) * expected_utf16words);
  // convert to UTF-16LE
  size_t utf16words = simdutf::convert_utf8_to_utf16le(source, 4, utf16_output);
  // It wrote utf16words * sizeof(char16_t) bytes.
  bool validutf16 = simdutf::validate_utf16le(utf16_output, utf16words);
  if (validutf16) {
    printf("valid UTF-16LE\n");
  } else {
    printf("invalid UTF-16LE\n");
    free(utf16_output);
    return EXIT_FAILURE;
  }
  // convert it back:
  // We need a buffer of size where to write the UTF-8 words.
  size_t expected_utf8words =
      simdutf::utf8_length_from_utf16le(utf16_output, utf16words);
  char *utf8_output = (char*) malloc(expected_utf8words);
  // convert to UTF-8
  size_t utf8words =
      simdutf::convert_utf16le_to_utf8(utf16_output, utf16words, utf8_output);
  if ((utf8words != 4) || (strncmp(utf8_output, source, utf8words) != 0)) {
    printf("bad conversion\n");
    free(utf8_output);
    return EXIT_FAILURE;
  } else {
    free(utf8_output);
    printf("perfect round trip\n");
  }
  return EXIT_SUCCESS;
}
