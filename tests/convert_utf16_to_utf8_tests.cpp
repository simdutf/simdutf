#include "simdutf.h"

#include <array>
#include <algorithm>
#include <iostream>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>

#include "test_macros.h"

namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_utf16_to_utf8_test_base;

  constexpr int trials = 1000;
}

TEST(convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t {
    return counter++ & 0x7f;
  };

  auto procedure = [&implementation](const char16_t* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf16_to_utf8(utf8, size, utf16);
  };

  std::array<size_t, 1> input_size{16};
  for (size_t size: input_size) {
    transcode_utf16_to_utf8_test_base test(generator, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST(convert_into_1_or_2_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    simdutf::tests::helpers::RandomInt random(0x0000, 0x07ff); // range for 1 or 2 UTF-8 bytes

    auto procedure = [&implementation](const char16_t* utf8, size_t size, char* utf16) -> size_t {
      return implementation.convert_valid_utf16_to_utf8(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf16_to_utf8_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_into_1_or_2_or_3_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 1, 2 or 3 UTF-8 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0x007f},
                                                     {0x0080, 0x07ff},
                                                     {0x0800, 0xd7ff},
                                                     {0xe000, 0xffff}}, 0);

    auto procedure = [&implementation](const char16_t* utf8, size_t size, char* utf16) -> size_t {
      return implementation.convert_valid_utf16_to_utf8(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf16_to_utf8_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_into_3_or_4_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 3 or 4 UTF-8 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10'ffff}});

    auto procedure = [&implementation](const char16_t* utf8, size_t size, char* utf16) -> size_t {
      return implementation.convert_valid_utf16_to_utf8(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf16_to_utf8_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_fails_if_there_is_sole_low_surrogate) {
  auto procedure = [&implementation](const char16_t* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf16_to_utf8(utf8, size, utf16);
  };

  const size_t size = 64;
  transcode_utf16_to_utf8_test_base test([](){return '*';}, size);

  for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff; low_surrogate++) {
    for (size_t i=0; i < size; i++) {

      const auto old = test.input_utf16[i];
      test.input_utf16[i] = low_surrogate;
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}

TEST(convert_fails_if_there_is_sole_high_surrogate) {
  auto procedure = [&implementation](const char16_t* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf16_to_utf8(utf8, size, utf16);
  };

  const size_t size = 64;
  transcode_utf16_to_utf8_test_base test([](){return '*';}, size);

  for (char16_t high_surrogate = 0xdc00; high_surrogate <= 0xdfff; high_surrogate++) {
    for (size_t i=0; i < size; i++) {

      const auto old = test.input_utf16[i];
      test.input_utf16[i] = high_surrogate;
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}

TEST(convert_fails_if_there_is_low_surrogate_is_followed_by_another_low_surrogate) {
  auto procedure = [&implementation](const char16_t* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf16_to_utf8(utf8, size, utf16);
  };

  const size_t size = 64;
  transcode_utf16_to_utf8_test_base test([](){return '*';}, size);

  for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff; low_surrogate++) {
    for (size_t i=0; i < size - 1; i++) {

      const auto old0 = test.input_utf16[i + 0];
      const auto old1 = test.input_utf16[i + 1];
      test.input_utf16[i + 0] = low_surrogate;
      test.input_utf16[i + 1] = low_surrogate;
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i + 0] = old0;
      test.input_utf16[i + 1] = old1;
    }
  }
}

TEST(convert_fails_if_there_is_surrogate_pair_is_followed_by_high_surrogate) {
  auto procedure = [&implementation](const char16_t* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf16_to_utf8(utf8, size, utf16);
  };

  const size_t size = 64;
  transcode_utf16_to_utf8_test_base test([](){return '*';}, size);

  const char16_t low_surrogate = 0xd801;
  const char16_t high_surrogate = 0xdc02;
  for (size_t i=0; i < size - 2; i++) {

    const auto old0 = test.input_utf16[i + 0];
    const auto old1 = test.input_utf16[i + 1];
    const auto old2 = test.input_utf16[i + 2];
    test.input_utf16[i + 0] = low_surrogate;
    test.input_utf16[i + 1] = high_surrogate;
    test.input_utf16[i + 2] = high_surrogate;
    ASSERT_TRUE(test(procedure));
    test.input_utf16[i + 0] = old0;
    test.input_utf16[i + 1] = old1;
    test.input_utf16[i + 2] = old2;
  }
}

int main() {
  for (const auto& implementation: simdutf::available_implementations) {
    if (implementation == nullptr) {
      puts("SIMDUTF implementation is null");
      abort();
    }

    const simdutf::implementation& impl = *implementation;
    if (implementation->name() != "westmere")
      continue;

    printf("Checking implementation %s\n", implementation->name().c_str());

    for (auto test: test_procedures())
      if (test.name.find("convert_into_1_or_2_or_3_UTF8_bytes") != std::string::npos)
        test(*implementation);
  }
}
