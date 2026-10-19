#include "simdutf.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/test.h>

// https://github.com/nodejs/node/issues/48995
TEST(node48995) {
  const char bad[1] = {'\x80'};
  size_t length = 1;
  ASSERT_FALSE(implementation.validate_utf8(bad, length));
}

TEST(copyright) {
  const char good[2] = {'\xC2', '\xA9'};
  size_t length = 2;
  ASSERT_TRUE(implementation.validate_utf8(good, length));
}

namespace {
// additional tests are from autobahn websocket testsuite
// https://github.com/crossbario/autobahn-testsuite/tree/master/autobahntestsuite/autobahntestsuite/case
constexpr const char *goodsequences[] = {"a",
                                         "\xc3\xb1",
                                         "\xe2\x82\xa1",
                                         "\xf0\x90\x8c\xbc",
                                         "\xc2\x80",         // 6.7.2
                                         "\xf0\x90\x80\x80", // 6.7.4
                                         "\xee\x80\x80",     // 6.11.2
                                         "\xef\xbb\xbf"};
constexpr const char *badsequences[] = {
    "\xc3\x28",                                 // 0
    "\xa0\xa1",                                 // 1
    "\xe2\x28\xa1",                             // 2
    "\xe2\x82\x28",                             // 3
    "\xf0\x28\x8c\xbc",                         // 4
    "\xf0\x90\x28\xbc",                         // 5
    "\xf0\x28\x8c\x28",                         // 6
    "\xc0\x9f",                                 // 7
    "\xf5\xff\xff\xff",                         // 8
    "\xed\xa0\x81",                             // 9
    "\xf8\x90\x80\x80\x80",                     // 10
    "123456789012345\xed",                      // 11
    "123456789012345\xf1",                      // 12
    "123456789012345\xc2",                      // 13
    "\xC2\x7F",                                 // 14
    "\xce",                                     // 6.6.1
    "\xce\xba\xe1",                             // 6.6.3
    "\xce\xba\xe1\xbd",                         // 6.6.4
    "\xce\xba\xe1\xbd\xb9\xcf",                 // 6.6.6
    "\xce\xba\xe1\xbd\xb9\xcf\x83\xce",         // 6.6.8
    "\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce", // 6.6.10
    "\xdf",                                     // 6.14.6
    "\xef\xbf",                                 // 6.14.7
    "\x80",
    "\x91\x85\x95\x9e",
    "\x6c\x02\x8e\x18",
    "\x25\x5b\x6e\x2c\x32\x2c\x5b\x5b\x33\x2c\x34\x2c\x05\x29\x2c\x33\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b"
    "\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x35\x2e\x33\x2c\x39\x2e\x33"
    "\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37"
    "\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d"
    "\x5d\x5d\x5d\x5d\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x20\x01\x01"
    "\x01\x01\x01\x02\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x23\x0a\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x7e\x7e\x0a\x0a\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x5b\x5b"
    "\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x37"
    "\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33"
    "\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d"
    "\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x01\x01\x80\x01\x01\x01\x79\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01",
    "[[[[[[[[[[[[[[["
    "\x80\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x010\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01",
    "\x20\x0b\x01\x01\x01\x64\x3a\x64\x3a\x64\x3a\x5b\x5b\x5b\x5b\x5b\x5b\x5b"
    "\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b"
    "\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x30\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x80\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01"};
} // namespace

TEST(good_bad_sequences) {

  for (size_t i = 0; i < sizeof(goodsequences) / sizeof(goodsequences[0]);
       i++) {
    size_t len = std::strlen(goodsequences[i]);
    if (!implementation.validate_utf8(goodsequences[i], len)) {
      printf("bug goodsequences[%zu]\n", i);
      ASSERT_TRUE(false);
    }
  }
  for (size_t i = 0; i < sizeof(badsequences) / sizeof(badsequences[0]); i++) {
    size_t len = std::strlen(badsequences[i]);
    if (implementation.validate_utf8(badsequences[i], len)) {
      printf("bug lookup2 badsequences[%zu]\n", i);
      ASSERT_TRUE(false);
    }
  }

  puts("OK");
}

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_validation_goodsequences) {
  static_assert(std::ranges::all_of(goodsequences, [](std::string_view v) {
    return simdutf::validate_utf8(v);
  }));
}

TEST(compile_time_validation_badsequences) {

  static_assert(!std::ranges::all_of(badsequences, [](std::string_view v) {
    return simdutf::validate_utf8(v);
  }));
}

TEST(compile_time_validation_casts_of_good) {

  using namespace simdutf::tests::helpers;
  constexpr auto good = u8"My favourite dish is köttbullar"_utf8;
  static_assert(simdutf::validate_utf8(good));
  static_assert(simdutf::validate_utf8(good.as_array<std::int8_t>()));
  static_assert(simdutf::validate_utf8(good.as_array<std::uint8_t>()));
  static_assert(simdutf::validate_utf8(good.as_array<std::byte>()));
}

TEST(compile_time_validation_casts_of_bad) {

  using namespace simdutf::tests::helpers;

  // this is the emoji U+1F373 COOKING
  constexpr auto good = u8"\U0001F373"_utf8;
  static_assert(simdutf::validate_utf8(good));

  // cut off the last byte to make it bad
  constexpr auto bad = good.shrink<good.size() - 1>();

  static_assert(not simdutf::validate_utf8(bad));
  static_assert(not simdutf::validate_utf8(bad.as_array<std::int8_t>()));
  static_assert(not simdutf::validate_utf8(bad.as_array<std::uint8_t>()));
  static_assert(not simdutf::validate_utf8(bad.as_array<std::byte>()));
}

#endif

namespace {
// The validators handle the bytes after the last full 64-byte block
// separately. These tests check every length of that tail against the scalar
// code. Each input is copied to a buffer of exactly its length, so that the
// address sanitizer reports a read past either end.
constexpr size_t tail_max_len = 200;

// Bytes that end an ASCII prefix: invalid bytes, complete characters and
// characters cut off by the end of the input.
constexpr std::string_view tail_patterns[] = {
    "\x80",         "\xff",     "\xc3\xa9",     "\xc3",
    "\xe4\xb8\xad", "\xe4\xb8", "\xed\xa0\x80", "\xf0\x9f\x98\x80",
    "\xf0\x9f\x98"};

void check_tail(const simdutf::implementation &implementation,
                const std::string &input) {
  const std::vector<char> buffer(input.begin(), input.end());
  const char *p = buffer.data();
  const size_t len = buffer.size();
  const bool valid = implementation.validate_utf8(p, len);
  const simdutf::result with_errors =
      implementation.validate_utf8_with_errors(p, len);
  const simdutf::result expected =
      simdutf::scalar::utf8::validate_with_errors(p, len);
  if (valid != (expected.error == simdutf::error_code::SUCCESS) ||
      with_errors.error != expected.error ||
      with_errors.count != expected.count) {
    std::printf("failing input, %zu bytes:", len);
    for (size_t i = 0; i < len; i++) {
      std::printf(" %02x", static_cast<unsigned char>(p[i]));
    }
    std::printf("\n");
  }
  ASSERT_EQUAL(valid, simdutf::scalar::utf8::validate(p, len));
  ASSERT_EQUAL(with_errors.error, expected.error);
  ASSERT_EQUAL(with_errors.count, expected.count);
}

// Repeats character to fill len bytes, cutting off the last one if needed.
std::string repeat(std::string_view character, size_t len) {
  std::string s;
  while (s.size() < len) {
    s += character;
  }
  s.resize(len);
  return s;
}
} // namespace

TEST(tail_ascii_every_length) {
  for (size_t len = 0; len <= tail_max_len; len++) {
    check_tail(implementation, std::string(len, 'a'));
  }
}

TEST(tail_non_ascii_at_every_position) {
  for (size_t len = 1; len <= tail_max_len; len++) {
    for (std::string_view pattern : tail_patterns) {
      for (size_t pos = 0; pos < len; pos++) {
        std::string s(len, 'a');
        s.replace(pos, std::min(pattern.size(), len - pos), pattern.data(),
                  std::min(pattern.size(), len - pos));
        check_tail(implementation, s);
      }
    }
  }
}

TEST(tail_multibyte_text_with_ascii_tail) {
  for (size_t len = 0; len <= tail_max_len; len++) {
    for (size_t tail = 0; tail <= len && tail <= 70; tail++) {
      const size_t text = (len - tail) / 3 * 3;
      check_tail(implementation,
                 repeat("\xe4\xb8\xad", text) + std::string(len - text, 'b'));
    }
  }
}

// Long inputs check their last 64 bytes in place, which overlaps the last full
// block. Multibyte text of each width makes the last full block end inside a
// character at every offset, and the patterns put an error or a cut-off
// character at every position.
TEST(tail_non_ascii_in_multibyte_text) {
  constexpr std::string_view characters[] = {"\xc3\xa9", "\xe4\xb8\xad",
                                             "\xf0\x9f\x98\x80"};
  for (size_t len = 64; len <= 160; len++) {
    for (std::string_view character : characters) {
      for (std::string_view pattern : tail_patterns) {
        for (size_t pos = 0; pos < len; pos++) {
          std::string s = repeat(character, len);
          s.replace(pos, std::min(pattern.size(), len - pos), pattern.data(),
                    std::min(pattern.size(), len - pos));
          check_tail(implementation, s);
        }
      }
    }
  }
}

TEST_MAIN
