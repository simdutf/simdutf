#include "simdutf.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include <tests/helpers/random_int.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>


namespace {
std::array<size_t, 9> input_size{7, 12, 16, 64, 67, 128, 256, 511, 1000};

using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;
} // namespace

TEST(count_just_one_word) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf16 random(seed, 1, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_EQUAL(implementation.count_utf16le(
                      reinterpret_cast<const char16_t *>(generated.first.data()),
                      size), generated.second);
    }
  }
}
TEST(count_1_or_2_UTF16_words) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf16 random(seed, 1, 1);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_EQUAL(implementation.count_utf16le(
                      reinterpret_cast<const char16_t *>(generated.first.data()),
                      size),generated.second);
    }
  }
}

TEST(count_2_UTF16_words) {
  for (size_t trial = 0; trial < 10000; trial++) {
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf16 random(seed, 0, 1);

    for (size_t size : input_size) {

      auto generated = random.generate_counted(size);
      ASSERT_EQUAL(implementation.count_utf16le(
                      reinterpret_cast<const char16_t *>(generated.first.data()),
                      size), generated.second);
    }
  }
}


int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
