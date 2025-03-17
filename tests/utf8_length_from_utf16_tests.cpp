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
  }
}

TEST_MAIN
