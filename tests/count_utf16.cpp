#include "simdutf.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include <tests/helpers/random_int.h>
#include <tests/helpers/transcode_test_base.h>

#include <tests/helpers/random_utf16.h>

#include "test_macros.h"

namespace {
std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;
} // namespace

TEST(count_just_one_word) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    std::random_device rd{};

    simdutf::tests::helpers::random_utf16 random(rd, 1, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf16(
                      reinterpret_cast<const char16_t *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}
TEST(count_1_or_2_UTF16_words) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    std::random_device rd{};

    simdutf::tests::helpers::random_utf16 random(rd, 1, 1);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf16(
                      reinterpret_cast<const char16_t *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}




TEST(count_2_UTF16_words) {
  for (size_t trial = 0; trial < 10000; trial++) {
    std::random_device rd{};

    simdutf::tests::helpers::random_utf16 random(rd, 0, 1);

    for (size_t size : input_size) {

      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf16(
                      reinterpret_cast<const char16_t *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}

int main() {
  for (const auto &implementation : simdutf::available_implementations) {
    if (implementation == nullptr) {
      puts("SIMDUTF implementation is null");
      abort();
    }

    const simdutf::implementation &impl = *implementation;
    printf("Checking implementation %s\n", implementation->name().c_str());

    for (auto test : test_procedures())
      test(*implementation);
  }
}
