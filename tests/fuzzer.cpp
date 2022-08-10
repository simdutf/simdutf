#include "simdutf.h"

#include <array>
#include <iostream>

#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

#include "reference/encode_utf8.h"

/**
 * We should be able to receive random data without any problem
 * when using the validating transcoder. It is difficult to test
 * extensively, but it is easy to try many thousands of random test
 * cases.
 */

namespace {
std::array<size_t, 10> input_size{7, 16, 12, 64, 67, 128, 129, 256, 1024, 1025};
}

using GenerateCodepoint = std::function<uint32_t()>;

struct dyn_buffer {
  std::vector<char> input;
  std::vector<char16_t> output;
  dyn_buffer(GenerateCodepoint generate, size_t size) : input(size) {
    regenerate(generate);
  }
  void print_input() const {
    for (size_t i = 0; i < input.size(); i++) {
      printf("%x ", uint8_t(input[i]));
    }
    printf("\n");
  }
  void regenerate(GenerateCodepoint generate) {
    const size_t size = input.size();
    input.clear();
    while (input.size() < size) {
      const uint32_t codepoint = generate();
      simdutf::tests::reference::utf8::encode(codepoint, [this](uint8_t byte) {
        this->input.push_back(byte);
      });
    }
    size_t output_length = simdutf::utf16_length_from_utf8(reinterpret_cast<const char *>(input.data()), input.size());
    output.resize(output_length);
    ASSERT_TRUE(output.capacity() == output_length);
  }
};

TEST(from_utf8_basic_fuzz) {
  uint32_t seed{124};
  simdutf::tests::helpers::RandomIntRanges random({{0x0, 0xd800-1},
                                                  {0xe000, 0x10ffff}}, seed);
  std::vector<dyn_buffer> buffers;
  for (size_t size : input_size) {
    buffers.emplace_back(random, size);
  }
  size_t counter{0};
  while (counter < 100000) {
    for (dyn_buffer &buf : buffers) {
      buf.regenerate(random);
      counter++;
      if ((counter % 10000) == 0) {
        printf("-");
        fflush(NULL);
      }
      size_t sizeutf8 = implementation.convert_utf8_to_utf16le(
          buf.input.data(), buf.input.size(),
          reinterpret_cast<char16_t *>(buf.output.data()));
      ASSERT_TRUE(sizeutf8 == buf.output.size());
    }
  }
}

int main(int argc, char *argv[]) { return simdutf::test::main(argc, argv); }
