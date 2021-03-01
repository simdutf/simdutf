#include "simdutf.h"

#include <array>
#include <random>
#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>

#include "test_macros.h"

namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;
}

TEST(convert_pure_ASCII) {
  for(size_t trial = 0; trial < 10000; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
      return counter++ & 0x7f;
    };

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return implementation.convert_utf8_to_utf16(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(generator, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_1_or_2_UTF8_bytes) {
  for(size_t trial = 0; trial < 10000; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    simdutf::tests::helpers::RandomInt random(0x0000, 0x07ff); // range for 1 or 2 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return implementation.convert_utf8_to_utf16(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_1_or_2_or_3_UTF8_bytes) {
  for(size_t trial = 0; trial < 10000; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 1, 2 or 3 UTF-8 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd7ff},
                                                     {0xe000, 0xffff}});

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return implementation.convert_utf8_to_utf16(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_3_or_4_UTF8_bytes) {
  for(size_t trial = 0; trial < 10000; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    simdutf::tests::helpers::RandomIntRanges random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10'ffff}}); // range for 3 or 4 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return implementation.convert_utf8_to_utf16(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

int main() {
  for (const auto& implementation: simdutf::available_implementations) {
    if (implementation == nullptr) {
      puts("SIMDUTF implementation is null");
      abort();
    }

    const simdutf::implementation& impl = *implementation;
    printf("Checking implementation %s\n", implementation->name().c_str());

    for (auto test: test_procedures())
      test(*implementation);
  }
}
