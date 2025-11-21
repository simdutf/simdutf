// The goal of these tests is to trigger the case when a low surrogate (the
// first one) is the last char handled by a vectorized code, and the remaining
// single one char16_t is passed to scalar code.

#include "simdutf.h"

#include <tests/helpers/test.h>

TEST(utf16le_surrogate_pair) {
  for (size_t size = 0; size < 512; size++) {
    std::vector<uint8_t> input(size * 2, 0);

    // low surrogate
    input.push_back(0x01);
    input.push_back(0xd8);

    // high surrogate
    input.push_back(0x01);
    input.push_back(0xdc);

    const size_t want = size + 4;
    const size_t got = implementation.utf8_length_from_utf16le(
        reinterpret_cast<const char16_t *>(input.data()), input.size() / 2);

    ASSERT_EQUAL(want, got);

    const simdutf::result got_with_replacement =
        implementation.utf8_length_from_utf16le_with_replacement(
            reinterpret_cast<const char16_t *>(input.data()), input.size() / 2);
    ASSERT_EQUAL(want, got_with_replacement.count);
    ASSERT_EQUAL(simdutf::SURROGATE, got_with_replacement.error);
  }
}

TEST(utf16be_surrogate_pair) {
  for (size_t size = 0; size < 512; size++) {
    std::vector<uint8_t> input(size * 2, 0);

    // low surrogate
    input.push_back(0xd8);
    input.push_back(0x01);

    // high surrogate
    input.push_back(0xdc);
    input.push_back(0x01);

    const size_t want = size + 4;
    const size_t got = implementation.utf8_length_from_utf16be(
        reinterpret_cast<const char16_t *>(input.data()), input.size() / 2);

    ASSERT_EQUAL(want, got);

    const simdutf::result got_with_replacement =
        implementation.utf8_length_from_utf16be_with_replacement(
            reinterpret_cast<const char16_t *>(input.data()), input.size() / 2);
    ASSERT_EQUAL(want, got_with_replacement.count);
    ASSERT_EQUAL(simdutf::SURROGATE, got_with_replacement.error);
  }
}

TEST(issue001) {
  // There are surrogates but they are well formed.
  std::vector<char16_t> input = {0x004e, 0x000e, 0xdbba, 0xdd90,
                                 0x030b, 0x0035, 0x004f, 0x0045};
#if SIMDUTF_BIG_ENDIAN
  const size_t standard =
      implementation.utf8_length_from_utf16be(input.data(), input.size());
  ASSERT_EQUAL(standard, 11);
  const auto result1 = implementation.utf8_length_from_utf16be_with_replacement(
      input.data(), input.size());
  ASSERT_EQUAL(result1.count, 11);
  ASSERT_EQUAL(simdutf::SURROGATE, result1.error);
#else
  const size_t standard =
      implementation.utf8_length_from_utf16le(input.data(), input.size());
  ASSERT_EQUAL(standard, 11);
  const auto result2 = implementation.utf8_length_from_utf16le_with_replacement(
      input.data(), input.size());
  ASSERT_EQUAL(result2.count, 11);
  ASSERT_EQUAL(simdutf::SURROGATE, result2.error);
#endif
}


TEST(issue002) {
  // There are surrogates but they are well formed.
  std::vector<char16_t> input = {0xd950, 0xdd9a, 0x002d};
#if SIMDUTF_BIG_ENDIAN
  const size_t standard =
      implementation.utf8_length_from_utf16be(input.data(), input.size());
  ASSERT_EQUAL(standard, 5);
  const auto result1 = implementation.utf8_length_from_utf16be_with_replacement(
      input.data(), input.size());
  ASSERT_EQUAL(result1.count, 5);
  ASSERT_EQUAL(simdutf::SURROGATE, result1.error);
#else
  const size_t standard =
      implementation.utf8_length_from_utf16le(input.data(), input.size());
  ASSERT_EQUAL(standard, 5);
  const auto result2 = implementation.utf8_length_from_utf16le_with_replacement(
      input.data(), input.size());
  ASSERT_EQUAL(result2.count, 5);
  ASSERT_EQUAL(simdutf::SURROGATE, result2.error);
#endif
}

TEST_MAIN
