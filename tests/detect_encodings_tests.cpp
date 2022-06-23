#include "simdutf.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include <tests/helpers/random_int.h>
#include <tests/helpers/random_utf8.h>
#include <tests/helpers/test.h>

namespace {
std::array<size_t, 7> input_size{8, 16, 12, 64, 68, 128, 256};
} // namespace

TEST(pure_utf8_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 1, 0, 0, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;    // 3
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(pure_utf16_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::RandomInt random(0,127, seed);

    for (size_t size : input_size) {
      std::vector<uint16_t> generated;
      for (int i = 0; i < size; i++) {
        generated.push_back(uint16_t(random()));
      }
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;    // 3
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(pure_utf32_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::RandomInt random(0,127, seed);

    for (size_t size : input_size) {
      std::vector<uint32_t> generated;
      for (int i = 0; i < size; i++) {
        generated.push_back(random());
      }
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE | simdutf::encoding_type::UTF32_LE;    // 11
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
