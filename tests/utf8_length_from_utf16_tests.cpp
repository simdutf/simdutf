// The goal of these tests is to trigger the case when a low surrogate (the
// first one) is the last char handled by a vectorized code, and the remaining
// single one char16_t is passed to scalar code.

#include "simdutf.h"

#include <tests/helpers/compiletime_conversions.h>
#include <tests/helpers/fixed_string.h>
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
#if SIMDUTF_IS_BIG_ENDIAN
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
#if SIMDUTF_IS_BIG_ENDIAN
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

TEST(bug_found_in_release_7_7_0) {

  // this is invalid input in native endian, such that
  // utf8_length_from_utf16_with_replacement happens to give a different answer
  // than utf8_length_from_utf16. It is implementation defined what
  // utf8_length_from_utf16 gives, but it is sufficient to demonstrate the bug
  // to prove the bug in the current implementation.

  const std::vector<char16_t> input = {0xD800, 0xDC00, 0xDFFF, 0xD800, 0xDC00};
  const bool valid = simdutf::validate_utf16(input.data(), input.size());
  ASSERT_FALSE(valid);

  const auto native_length =
      simdutf::utf8_length_from_utf16(input.data(), input.size());
  const auto be_length =
      simdutf::utf8_length_from_utf16be(input.data(), input.size());
  const auto le_length =
      simdutf::utf8_length_from_utf16le(input.data(), input.size());
#if SIMDUTF_IS_BIG_ENDIAN
  ASSERT_EQUAL(native_length, be_length);
  (void)le_length;
#else
  (void)be_length;
  ASSERT_EQUAL(native_length, le_length);
#endif
}

#if SIMDUTF_CPLUSPLUS23

namespace {
// makes a malformed string in the requested endianness
template <simdutf::endianness e> constexpr auto make_malformed() {
  simdutf::tests::helpers::CTString<
      char16_t, 5,
      e == simdutf::endianness::BIG ? std::endian::big : std::endian::little>
      data{};
  data[2] = simdutf::scalar::utf16::swap_if_needed<e>(0xD800);
  return data;
}
} // namespace

TEST(compile_time_utf8_length_from_utf16_with_replacement) {
  using namespace simdutf::tests::helpers;
  using enum simdutf::endianness;

  {
    constexpr auto malformed = make_malformed<NATIVE>();
    constexpr simdutf::result utf8_length =
        simdutf::utf8_length_from_utf16_with_replacement(malformed);
    static_assert(utf8_length.count == malformed.size() + 2);
    static_assert(utf8_length.error == simdutf::SURROGATE);
    constexpr auto wellformed = to_wellformed(malformed);
    constexpr size_t utf8_length_check =
        simdutf::utf8_length_from_utf16(wellformed);
    static_assert(utf8_length.count == utf8_length_check);
  }
}

TEST(compile_time_utf8_length_from_utf16le_with_replacement) {
  using namespace simdutf::tests::helpers;
  using enum simdutf::endianness;

  {
    constexpr auto malformed = make_malformed<LITTLE>();
    constexpr simdutf::result utf8_length =
        simdutf::utf8_length_from_utf16le_with_replacement(malformed);
    static_assert(utf8_length.count == malformed.size() + 2);
    static_assert(utf8_length.error == simdutf::SURROGATE);
    constexpr auto wellformed = to_wellformed(malformed);
    constexpr size_t utf8_length_check =
        simdutf::utf8_length_from_utf16le(wellformed);
    static_assert(utf8_length.count == utf8_length_check);
  }
}

TEST(compile_time_utf8_length_from_utf16be_with_replacement) {
  using namespace simdutf::tests::helpers;
  using enum simdutf::endianness;

  {
    constexpr auto malformed = make_malformed<BIG>();
    constexpr simdutf::result utf8_length =
        simdutf::utf8_length_from_utf16be_with_replacement(malformed);
    static_assert(utf8_length.count == malformed.size() + 2);
    static_assert(utf8_length.error == simdutf::SURROGATE);
    constexpr auto wellformed = to_wellformed(malformed);
    constexpr size_t utf8_length_check =
        simdutf::utf8_length_from_utf16be(wellformed);
    static_assert(utf8_length.count == utf8_length_check);
  }
}

#endif

TEST_MAIN
