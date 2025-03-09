#include "simdutf.h"

#include <array>
#include <vector>

#include <tests/reference/validate_utf16.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness LE = simdutf::endianness::LITTLE;

using simdutf::tests::helpers::transcode_utf16_to_utf32_test_base;

constexpr int trials = 1000;
} // namespace

TEST_LOOP(trials, convert_2_UTF16_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0x007f}, {0x0080, 0x07ff}, {0x0800, 0xd7ff}, {0xe000, 0xffff}},
      seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_utf16le_to_utf32(utf16, size, utf32);
  };
  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return implementation.utf32_length_from_utf16le(utf16, size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_utf32_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(trials, convert_with_surrogates) {
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}, {0xe000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_utf16le_to_utf32(utf16, size, utf32);
  };
  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return implementation.utf32_length_from_utf16le(utf16, size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_utf32_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST(convert_fails_if_there_is_sole_low_surrogate) {
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_utf16le_to_utf32(utf16, size, utf32);
  };
  const size_t size = 64;
  transcode_utf16_to_utf32_test_base test(LE, []() { return '*'; }, size + 32);

  for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff;
       low_surrogate++) {
    for (size_t i = 0; i < size; i++) {
      const auto old = test.input_utf16[i];
      test.input_utf16[i] = to_utf16le(low_surrogate);
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}

TEST(convert_fails_if_there_is_sole_high_surrogate) {
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_utf16le_to_utf32(utf16, size, utf32);
  };

  const size_t size = 64;
  transcode_utf16_to_utf32_test_base test(LE, []() { return '*'; }, size + 32);

  for (char16_t high_surrogate = 0xd800; high_surrogate <= 0xdbff;
       high_surrogate++) {
    for (size_t i = 0; i < size; i++) {
      const auto old = test.input_utf16[i];
      test.input_utf16[i] = to_utf16le(high_surrogate);
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}

TEST(
    convert_fails_if_there_is_low_surrogate_followed_by_another_low_surrogate) {
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_utf16le_to_utf32(utf16, size, utf32);
  };

  const size_t size = 64;
  transcode_utf16_to_utf32_test_base test(LE, []() { return '*'; }, size + 32);

  for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff;
       low_surrogate++) {
    for (size_t i = 0; i < size - 1; i++) {
      const auto old0 = test.input_utf16[i + 0];
      const auto old1 = test.input_utf16[i + 1];
      test.input_utf16[i + 0] = to_utf16le(low_surrogate);
      test.input_utf16[i + 1] = to_utf16le(low_surrogate);
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i + 0] = old0;
      test.input_utf16[i + 1] = old1;
    }
  }
}

TEST(convert_fails_if_there_is_surrogate_pair_followed_by_high_surrogate) {
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_utf16le_to_utf32(utf16, size, utf32);
  };

  const size_t size = 64;
  transcode_utf16_to_utf32_test_base test(LE, []() { return '*'; }, size + 32);

  const char16_t low_surrogate = to_utf16le(0xd801);
  const char16_t high_surrogate = to_utf16le(0xdc02);
  for (size_t i = 0; i < size - 2; i++) {
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

TEST(all_possible_8_codepoint_combinations) {
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_utf16le_to_utf32(utf16, size, utf32);
  };

  std::vector<char32_t> output_utf32(256, ' ');
  const auto &combinations = all_utf16_combinations(LE);
  for (const auto &input_utf16 : combinations) {

    if (simdutf::tests::reference::validate_utf16(LE, input_utf16.data(),
                                                  input_utf16.size())) {
      transcode_utf16_to_utf32_test_base test(LE, input_utf16);
      ASSERT_TRUE(test(procedure));
    } else {
      ASSERT_FALSE(procedure(input_utf16.data(), input_utf16.size(),
                             output_utf32.data()));
    }
  }
}

TEST_MAIN
