/**
 * Special tests. Specific cases.
 */
#include "simdutf.h"

#include <memory>
#include <tests/helpers/test.h>

TEST(special_cases_utf8_utf16_roundtrip) {
  std::string cases[] = {
    "\x05\x0A\x0A\x01\x0C\x00\x00\x0A\x0C\x00\x00\x00\x00\x00\x0A\x0A\x0A\xF0"
    "\x93\x93\x93\x01\x00\x00\x00\xE2\xBB\x9A\xEF\x9B\xBB\xEE\x81\x81\x05\x2D"
    "\x01\x7B\x00\x00\xE2\xBB\x9A\xEF\x9B\xBB\xEE\x81\x81\x05\x2D\x01\x7B\x00"
    "\x00\x00\x00\x0A\x00\x2A\x0A\x7E\x0A\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x20\x20\xEF\xBB\x8A\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
    "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
  };
  for (std::string source : cases) {
    bool validutf8 = simdutf::validate_utf8(source.c_str(), source.size());
    if (!validutf8) {
      continue;
    } // we allow it.
      // We need a buffer of size where to write the UTF-16LE words.
    std::cout << "it is valid!" << std::endl;

    size_t expected_utf16words =
        simdutf::utf16_length_from_utf8(source.c_str(), source.size());
    std::cout << "I am going to need " << expected_utf16words << " words"
              << std::endl;

    std::unique_ptr<char16_t[]> utf16_output{
      new char16_t[expected_utf16words]
    };
    // convert to UTF-16LE
    size_t utf16words = simdutf::convert_utf8_to_utf16le(
        source.c_str(), source.size(), utf16_output.get());
    // It wrote utf16words * sizeof(char16_t) bytes.
    bool validutf16 = simdutf::validate_utf16le(utf16_output.get(), utf16words);
    ASSERT_TRUE(validutf16);
    // convert it back:
    // We need a buffer of size where to write the UTF-8 words.
    size_t expected_utf8words =
        simdutf::utf8_length_from_utf16le(utf16_output.get(), utf16words);
    ASSERT_TRUE(expected_utf8words == source.size());
    std::unique_ptr<char[]> utf8_output{ new char[expected_utf8words] };
    // convert to UTF-8
    size_t utf8words = simdutf::convert_utf16le_to_utf8(
        utf16_output.get(), utf16words, utf8_output.get());
    ASSERT_TRUE(expected_utf8words == utf8words);
    std::string final_string(utf8_output.get(), utf8words);
    ASSERT_TRUE(final_string == source);
  }
}
int main(int argc, char *argv[]) { return simdutf::test::main(argc, argv); }