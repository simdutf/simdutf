#include "simdutf.h"

#include <array>
#include <vector>

#include <tests/reference/validate_utf16.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness BE = simdutf::endianness::BIG;

using simdutf::tests::helpers::transcode_utf16_to_utf32_test_base;

constexpr int trials = 1000;
} // namespace

TEST_LOOP(trials, convert_2_UTF16_bytes) {
  // range for 2-byte UTF-16 (no surrogate pairs)
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0x007f}, {0x0080, 0x07ff}, {0x0800, 0xd7ff}, {0xe000, 0xffff}},
      seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_valid_utf16be_to_utf32(utf16, size, utf32);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf32_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(trials, convert_with_surrogate_pairs) {
  // some surrogate pairs
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}, {0xe000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_valid_utf16be_to_utf32(utf16, size, utf32);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf32_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST(all_possible_8_codepoint_combinations) {
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_valid_utf16be_to_utf32(utf16, size, utf32);
  };

  std::vector<char> output_utf32(256, ' ');
  const auto &combinations = all_utf16_combinations(BE);
  for (const auto &input_utf16 : combinations) {
    if (simdutf::tests::reference::validate_utf16(BE, input_utf16.data(),
                                                  input_utf16.size())) {
      transcode_utf16_to_utf32_test_base test(BE, input_utf16);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST_MAIN
