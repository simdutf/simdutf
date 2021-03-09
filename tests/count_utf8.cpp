#include "simdutf.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include <tests/helpers/random_int.h>
#include <tests/helpers/transcode_test_base.h>

#include <tests/helpers/random_utf8.h>

#include "test_macros.h"

namespace {
std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;
} // namespace

TEST(count_pure_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    std::random_device rd{};

    simdutf::tests::helpers::RandomUTF8 random(rd, 1, 0, 0, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf8(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}

TEST(count_1_or_2_UTF8_bytes) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    std::random_device rd{};

    simdutf::tests::helpers::RandomUTF8 random(rd, 1, 1, 0, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf8(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}

TEST(count_1_or_2_or_3_UTF8_bytes) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    std::random_device rd{};

    simdutf::tests::helpers::RandomUTF8 random(rd, 1, 1, 1, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf8(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}

TEST(count_1_2_3_or_4_UTF8_bytes) {
  for (size_t trial = 0; trial < 10000; trial++) {
    std::random_device rd{};

    simdutf::tests::helpers::RandomUTF8 random(rd, 1, 1, 1, 1);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf8(
                      reinterpret_cast<const char *>(generated.first.data()),
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
