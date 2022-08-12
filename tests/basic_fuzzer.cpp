#include "simdutf.h"

#include <array>
#include <iostream>

#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

/**
 * We should be able to receive random data without any problem
 * when using the validating transcoder. It is difficult to test
 * extensively, but it is easy to try many thousands of random test
 * cases.
 */

namespace {
std::array<size_t, 10> input_size{7, 16, 12, 64, 67, 128, 129, 256, 1024, 1025};
}

struct buffer {
  std::vector<char> input;
  std::vector<char> output;
  buffer(size_t size) : input(size), output(4 * size) {}
  void print_input() const {
    for (size_t i = 0; i < input.size(); i++) {
      printf("%x ", uint8_t(input[i]));
    }
    printf("\n");
  }
  void randomize(std::mt19937 &gen) {
    for (size_t i = 0; i < input.size(); i++) {
      input[i] = char(gen());
    }
  }
};

TEST(basic_fuzz) {
  std::vector<buffer> buffers;
  for (size_t size : input_size) {
    buffers.emplace_back(size);
  }
  std::mt19937 gen(124); // We want deterministic results.
  size_t counter{0};
  while (counter < 100000) {
    for (buffer &buf : buffers) {
      buf.randomize(gen);
      counter++;
      if ((counter % 10000) == 0) {
        printf("-");
        fflush(NULL);
      }
      bool is_ok_utf8 =
          implementation.validate_utf8(buf.input.data(), buf.input.size());
      bool is_ok_utf16 = implementation.validate_utf16le(
          reinterpret_cast<char16_t *>(buf.input.data()),
          buf.input.size() / sizeof(char16_t));
      bool is_ok_utf32 = implementation.validate_utf32(
          reinterpret_cast<char32_t *>(buf.input.data()),
          buf.input.size() / sizeof(char32_t));
      size_t utf8_to_utf16 = implementation.convert_utf8_to_utf16le(
          buf.input.data(), buf.input.size(),
          reinterpret_cast<char16_t *>(buf.output.data()));
      size_t utf8_to_utf32 = implementation.convert_utf8_to_utf32(
          buf.input.data(), buf.input.size(),
          reinterpret_cast<char32_t *>(buf.output.data()));
      size_t utf16_to_utf8 = implementation.convert_utf16le_to_utf8(
          reinterpret_cast<char16_t *>(buf.input.data()),
          buf.input.size() / sizeof(char16_t), buf.output.data());
      size_t utf16_to_utf32 = implementation.convert_utf16le_to_utf32(
          reinterpret_cast<char16_t *>(buf.input.data()),
          buf.input.size() / sizeof(char16_t), reinterpret_cast<char32_t *>(buf.output.data()));
      size_t utf32_to_utf8 = implementation.convert_utf32_to_utf8(
          reinterpret_cast<char32_t *>(buf.input.data()),
          buf.input.size() / sizeof(char32_t), buf.output.data());
      size_t utf32_to_utf16 = implementation.convert_utf32_to_utf16le(
          reinterpret_cast<char32_t *>(buf.input.data()),
          buf.input.size() / sizeof(char32_t), reinterpret_cast<char16_t *>(buf.output.data()));
      if(is_ok_utf8 ? (utf8_to_utf16 == 0 || utf8_to_utf32 == 0) : (utf8_to_utf16 > 0 || utf8_to_utf32 > 0)) {
        std::cout << (is_ok_utf8 ? "UTF-8 is ok" : "UTF-8 is not ok") << std::endl;
        std::cout << " size = " << buf.input.size() << std::endl;
        std::cout << "  implementation.convert_utf8_to_utf16 return " << utf8_to_utf16 << std::endl;
        std::cout << "  implementation.convert_utf8_to_utf32 return " << utf8_to_utf32 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf8 ? (utf8_to_utf16 > 0 && utf8_to_utf32 > 0) : (utf8_to_utf16 == 0 && utf8_to_utf32 == 0));
      if(is_ok_utf16 ? (utf16_to_utf8 == 0 || utf16_to_utf32 == 0) : (utf16_to_utf8 > 0 || utf16_to_utf32 > 0)) {
        std::cout << (is_ok_utf8 ? "UTF-16 is ok" : "UTF-16 is not ok") << std::endl;
        std::cout << " size = " << buf.input.size() / sizeof(char16_t) << std::endl;
        std::cout << "  implementation.convert_utf16_to_utf8 return " << utf16_to_utf8 << std::endl;
        std::cout << "  implementation.convert_utf16_to_utf32 return " << utf16_to_utf32 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf16 ? (utf16_to_utf8 > 0 && utf16_to_utf32 > 0) : (utf16_to_utf8 == 0 && utf16_to_utf32 == 0));
      if(is_ok_utf32 ? (utf32_to_utf8 == 0 || utf32_to_utf16 == 0) : (utf32_to_utf8 > 0 || utf32_to_utf16 > 0)) {
        std::cout << (is_ok_utf8 ? "UTF-16 is ok" : "UTF-16 is not ok") << std::endl;
        std::cout << " size = " << buf.input.size() / sizeof(char32_t) << std::endl;
        std::cout << "  implementation.convert_utf32_to_utf8 return " << utf32_to_utf8 << std::endl;
        std::cout << "  implementation.convert_utf32_to_utf16 return " << utf32_to_utf16 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf32 ? (utf32_to_utf8 > 0 && utf32_to_utf16 > 0) : (utf32_to_utf8 == 0 && utf32_to_utf16 == 0));
    }
  }
}
int main(int argc, char *argv[]) { return simdutf::test::main(argc, argv); }
