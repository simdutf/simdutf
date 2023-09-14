/**
 * Special tests. Specific cases.
 */
#include "simdutf.h"
#include <iostream>
#include <tests/helpers/test.h>

// this is a compile test
int main_demo() {
  const char *source = "1234";
  // 4 == strlen(source)
  bool validutf8 = simdutf::validate_utf8(source, 4);
  if (validutf8) {
    std::cout << "valid UTF-8" << std::endl;
  } else {
    std::cerr << "invalid UTF-8" << std::endl;
    return EXIT_FAILURE;
  }
  // We need a buffer of size where to write the UTF-16LE words.
  size_t expected_utf16words = simdutf::utf16_length_from_utf8(source, 4);
  std::unique_ptr<char16_t[]> utf16_output{new char16_t[expected_utf16words]};
  // convert to UTF-16LE
  size_t utf16words =
      simdutf::convert_utf8_to_utf16le(source, 4, utf16_output.get());
  std::cout << "wrote " << utf16words << " UTF-16LE words." << std::endl;
  // It wrote utf16words * sizeof(char16_t) bytes.
  bool validutf16 = simdutf::validate_utf16le(utf16_output.get(), utf16words);
  if (validutf16) {
    std::cout << "valid UTF-16LE" << std::endl;
  } else {
    std::cerr << "invalid UTF-16LE" << std::endl;
    return EXIT_FAILURE;
  }
  // convert it back:
  // We need a buffer of size where to write the UTF-8 words.
  size_t expected_utf8words =
      simdutf::utf8_length_from_utf16le(utf16_output.get(), utf16words);
  std::unique_ptr<char[]> utf8_output{new char[expected_utf8words]};
  // convert to UTF-8
  size_t utf8words = simdutf::convert_utf16le_to_utf8(
      utf16_output.get(), utf16words, utf8_output.get());
  std::cout << "wrote " << utf8words << " UTF-8 words." << std::endl;
  std::string final_string(utf8_output.get(), utf8words);
  std::cout << final_string << std::endl;
  if (final_string != source) {
    std::cerr << "bad conversion" << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cerr << "perfect round trip" << std::endl;
  }
  return EXIT_SUCCESS;
}



TEST(error_location_badascii) {
  // this ASCII string has a bad byte at index 5
  std::string bad_ascii = "\x20\x20\x20\x20\x20\xff\x20\x20\x20";
  simdutf::result res = implementation.validate_ascii_with_errors(bad_ascii.data(), bad_ascii.size());
  if(res.error != simdutf::error_code::SUCCESS) {
    std::cerr << "error at index " << res.count << std::endl;
  }
  ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LARGE);
  ASSERT_EQUAL(res.count, 5);
}

int main(int argc, char *argv[]) { return simdutf::test::main(argc, argv); }