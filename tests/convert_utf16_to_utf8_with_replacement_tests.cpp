#include "simdutf.h"

#include <cstring>
#include <random>
#include <vector>

#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>
#include <tests/helpers/utf16.h>

#if SIMDUTF_CPLUSPLUS23
  #include <tests/helpers/fixed_string.h>
#endif

// U+FFFD in UTF-8 is 0xEF 0xBF 0xBD
constexpr char fffd_utf8[] = {char(0xef), char(0xbf), char(0xbd)};

// Test: valid UTF-16 should produce the same output as convert_utf16_to_utf8
TEST(valid_utf16le_roundtrip) {
  // ASCII + BMP characters
  std::vector<char16_t> input = {u'H',      u'e', u'l', u'l', u'o', u' ',
                                 u'\u00e9',  // e-acute (2-byte UTF-8)
                                 u'\u4e16',  // CJK character (3-byte UTF-8)
                                 u'\u754c'}; // CJK character (3-byte UTF-8)
  size_t expected_len =
      simdutf::utf8_length_from_utf16le(input.data(), input.size());
  std::vector<char> expected(expected_len);
  size_t written_expected = implementation.convert_utf16le_to_utf8(
      input.data(), input.size(), expected.data());

  std::vector<char> actual(expected_len + 10);
  size_t written_actual =
      implementation.convert_utf16le_to_utf8_with_replacement(
          input.data(), input.size(), actual.data());

  ASSERT_EQUAL(written_expected, written_actual);
  ASSERT_TRUE(std::memcmp(expected.data(), actual.data(), written_actual) == 0);
}

// Test: valid surrogate pairs should produce correct 4-byte UTF-8
TEST(valid_surrogate_pair_le) {
  // U+1F600 (grinning face) = D83D DE00
  std::vector<char16_t> input = {to_utf16le(char16_t(0xD83D)),
                                 to_utf16le(char16_t(0xDE00))};
  std::vector<char> output(8);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());
  ASSERT_EQUAL(written, size_t(4));
  // U+1F600 in UTF-8: F0 9F 98 80
  ASSERT_EQUAL(uint8_t(output[0]), uint8_t(0xF0));
  ASSERT_EQUAL(uint8_t(output[1]), uint8_t(0x9F));
  ASSERT_EQUAL(uint8_t(output[2]), uint8_t(0x98));
  ASSERT_EQUAL(uint8_t(output[3]), uint8_t(0x80));
}

// Test: unpaired high surrogate should be replaced with U+FFFD
TEST(unpaired_high_surrogate_le) {
  // D800 followed by a regular character
  std::vector<char16_t> input = {to_utf16le(char16_t(0xD800)),
                                 to_utf16le(u'A')};
  std::vector<char> output(8);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());
  // U+FFFD (3 bytes) + 'A' (1 byte) = 4
  ASSERT_EQUAL(written, size_t(4));
  ASSERT_TRUE(std::memcmp(output.data(), fffd_utf8, 3) == 0);
  ASSERT_EQUAL(output[3], 'A');
}

// Test: unpaired low surrogate should be replaced with U+FFFD
TEST(unpaired_low_surrogate_le) {
  // DC00 (low surrogate without preceding high surrogate)
  std::vector<char16_t> input = {to_utf16le(u'B'), to_utf16le(char16_t(0xDC00)),
                                 to_utf16le(u'C')};
  std::vector<char> output(16);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());
  // 'B' (1) + U+FFFD (3) + 'C' (1) = 5
  ASSERT_EQUAL(written, size_t(5));
  ASSERT_EQUAL(output[0], 'B');
  ASSERT_TRUE(std::memcmp(output.data() + 1, fffd_utf8, 3) == 0);
  ASSERT_EQUAL(output[4], 'C');
}

// Test: high surrogate at end of string
TEST(high_surrogate_at_end_le) {
  std::vector<char16_t> input = {to_utf16le(u'X'),
                                 to_utf16le(char16_t(0xD800))};
  std::vector<char> output(8);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());
  // 'X' (1) + U+FFFD (3) = 4
  ASSERT_EQUAL(written, size_t(4));
  ASSERT_EQUAL(output[0], 'X');
  ASSERT_TRUE(std::memcmp(output.data() + 1, fffd_utf8, 3) == 0);
}

// Test: two consecutive unpaired high surrogates
TEST(consecutive_unpaired_high_surrogates_le) {
  std::vector<char16_t> input = {to_utf16le(char16_t(0xD800)),
                                 to_utf16le(char16_t(0xD801))};
  std::vector<char> output(16);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());
  // Two U+FFFD = 6 bytes
  ASSERT_EQUAL(written, size_t(6));
  ASSERT_TRUE(std::memcmp(output.data(), fffd_utf8, 3) == 0);
  ASSERT_TRUE(std::memcmp(output.data() + 3, fffd_utf8, 3) == 0);
}

// Test: low surrogate followed by high surrogate (reversed pair)
TEST(reversed_surrogate_pair_le) {
  std::vector<char16_t> input = {to_utf16le(char16_t(0xDC00)),
                                 to_utf16le(char16_t(0xD800))};
  std::vector<char> output(16);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());
  // Both unpaired: two U+FFFD = 6 bytes
  ASSERT_EQUAL(written, size_t(6));
  ASSERT_TRUE(std::memcmp(output.data(), fffd_utf8, 3) == 0);
  ASSERT_TRUE(std::memcmp(output.data() + 3, fffd_utf8, 3) == 0);
}

// Test: empty input
TEST(empty_input_le) {
  std::vector<char> output(8);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      nullptr, 0, output.data());
  ASSERT_EQUAL(written, size_t(0));
}

// Test: pure ASCII
TEST(pure_ascii_le) {
  std::vector<char16_t> input = {to_utf16le(u'H'), to_utf16le(u'e'),
                                 to_utf16le(u'l'), to_utf16le(u'l'),
                                 to_utf16le(u'o')};
  std::vector<char> output(8);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());
  ASSERT_EQUAL(written, size_t(5));
  ASSERT_TRUE(std::memcmp(output.data(), "Hello", 5) == 0);
}

// Test: consistency with utf8_length_from_utf16_with_replacement
TEST_LOOP(length_consistency_le) {
  const size_t length = 64 + (seed % 128);
  std::mt19937 gen(seed);
  std::uniform_int_distribution<uint16_t> dist(0, 0xFFFF);

  std::vector<char16_t> input(length);
  for (size_t i = 0; i < length; i++) {
    input[i] = static_cast<char16_t>(dist(gen));
  }

  simdutf::result len_result =
      simdutf::utf8_length_from_utf16le_with_replacement(input.data(),
                                                         input.size());

  std::vector<char> output(length * 3 + 1);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());

  ASSERT_EQUAL(len_result.count, written);
}

// Test: output matches to_well_formed_utf16 + convert_utf16_to_utf8 approach
TEST_LOOP(matches_well_formed_approach_le) {
  const size_t length = 32 + (seed % 64);
  std::mt19937 gen(seed);
  std::uniform_int_distribution<uint16_t> dist(0, 0xFFFF);

  std::vector<char16_t> input(length);
  for (size_t i = 0; i < length; i++) {
    input[i] = static_cast<char16_t>(dist(gen));
  }

  // Approach 1: to_well_formed_utf16 + convert_utf16_to_utf8
  std::vector<char16_t> well_formed(length);
  simdutf::to_well_formed_utf16le(input.data(), length, well_formed.data());
  size_t utf8_len =
      simdutf::utf8_length_from_utf16le(well_formed.data(), length);
  std::vector<char> expected(utf8_len + 1);
  size_t written_expected = simdutf::convert_utf16le_to_utf8(
      well_formed.data(), length, expected.data());

  // Approach 2: convert_utf16_to_utf8_with_replacement (new function)
  std::vector<char> actual(utf8_len + 1);
  size_t written_actual =
      implementation.convert_utf16le_to_utf8_with_replacement(
          input.data(), length, actual.data());

  ASSERT_EQUAL(written_expected, written_actual);
  ASSERT_TRUE(std::memcmp(expected.data(), actual.data(), written_actual) == 0);
}

// Test: output is always valid UTF-8
TEST_LOOP(output_is_valid_utf8_le) {
  const size_t length = 64 + (seed % 256);
  std::mt19937 gen(seed);
  std::uniform_int_distribution<uint16_t> dist(0, 0xFFFF);

  std::vector<char16_t> input(length);
  for (size_t i = 0; i < length; i++) {
    input[i] = static_cast<char16_t>(dist(gen));
  }

  std::vector<char> output(length * 3 + 1);
  size_t written = implementation.convert_utf16le_to_utf8_with_replacement(
      input.data(), input.size(), output.data());

  ASSERT_TRUE(simdutf::validate_utf8(output.data(), written));
}

#if SIMDUTF_CPLUSPLUS23

namespace {

// makes a UTF-16 string with an unpaired high surrogate: 'A', 0xD800, 'B'
template <simdutf::endianness e> constexpr auto make_input_with_unpaired() {
  simdutf::tests::helpers::CTString<
      char16_t, 3,
      e == simdutf::endianness::BIG ? std::endian::big : std::endian::little>
      data{};
  data[0] = simdutf::scalar::utf16::swap_if_needed<e>(u'A');
  data[1] = simdutf::scalar::utf16::swap_if_needed<e>(char16_t(0xD800));
  data[2] = simdutf::scalar::utf16::swap_if_needed<e>(u'B');
  return data;
}

// expected output: 'A' (1 byte) + U+FFFD (3 bytes: EF BF BD) + 'B' (1 byte)
constexpr auto make_expected_output() {
  simdutf::tests::helpers::CTString<char, 5> data{};
  data[0] = 'A';
  data[1] = char(0xEF);
  data[2] = char(0xBF);
  data[3] = char(0xBD);
  data[4] = 'B';
  return data;
}

template <auto input> constexpr auto invoke_convert_with_replacement() {
  // 'A' = 1 byte, unpaired surrogate = 3 bytes (U+FFFD), 'B' = 1 byte
  simdutf::tests::helpers::CTString<char, 5> output{};
  std::size_t written;
  if constexpr (decltype(input)::endianness == std::endian::little) {
    written = simdutf::convert_utf16le_to_utf8_with_replacement(input, output);
  } else {
    written = simdutf::convert_utf16be_to_utf8_with_replacement(input, output);
  }
  if (written != output.size()) {
    throw "unexpected length";
  }
  return output;
}

} // namespace

TEST(compile_time_convert_utf16le_to_utf8_with_replacement) {
  using enum simdutf::endianness;
  constexpr auto input = make_input_with_unpaired<LITTLE>();
  constexpr auto result = invoke_convert_with_replacement<input>();
  constexpr auto expected = make_expected_output();
  static_assert(result == expected);
}

TEST(compile_time_convert_utf16be_to_utf8_with_replacement) {
  using enum simdutf::endianness;
  constexpr auto input = make_input_with_unpaired<BIG>();
  constexpr auto result = invoke_convert_with_replacement<input>();
  constexpr auto expected = make_expected_output();
  static_assert(result == expected);
}

#endif // SIMDUTF_CPLUSPLUS23

TEST_MAIN
