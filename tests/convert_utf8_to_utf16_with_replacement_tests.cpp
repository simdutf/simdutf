#include "simdutf.h"

#include <array>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include <tests/helpers/test.h>

#if SIMDUTF_CPLUSPLUS23
  #include <span>
#endif

namespace {

// Independent WHATWG decoder, not the library routine. Used as an oracle.
size_t oracle_whatwg(const char *buf, size_t len, char16_t *out) noexcept {
  const uint8_t *p = reinterpret_cast<const uint8_t *>(buf);
  char16_t *o = out;
  uint32_t cp = 0;
  int needed = 0;
  int seen = 0;
  uint8_t lo = 0x80;
  uint8_t hi = 0xBF;
  auto emit = [&](uint32_t c) {
    if (c >= 0x10000) {
      c -= 0x10000;
      *o++ = char16_t(0xD800 + (c >> 10));
      *o++ = char16_t(0xDC00 + (c & 0x3FF));
    } else {
      *o++ = char16_t(c);
    }
  };
  for (size_t i = 0; i < len;) {
    const uint8_t b = p[i];
    if (needed == 0) {
      i++;
      lo = 0x80;
      hi = 0xBF;
      if (b < 0x80) {
        *o++ = b;
      } else if (b >= 0xC2 && b <= 0xDF) {
        needed = 1;
        cp = b & 0x1F;
      } else if (b >= 0xE0 && b <= 0xEF) {
        if (b == 0xE0) {
          lo = 0xA0;
        }
        if (b == 0xED) {
          hi = 0x9F;
        }
        needed = 2;
        cp = b & 0xF;
      } else if (b >= 0xF0 && b <= 0xF4) {
        if (b == 0xF0) {
          lo = 0x90;
        }
        if (b == 0xF4) {
          hi = 0x8F;
        }
        needed = 3;
        cp = b & 0x7;
      } else {
        *o++ = 0xFFFD;
      }
      continue;
    }
    if (b < lo || b > hi) {
      cp = 0;
      needed = 0;
      seen = 0;
      lo = 0x80;
      hi = 0xBF;
      *o++ = 0xFFFD;
      continue;
    }
    i++;
    lo = 0x80;
    hi = 0xBF;
    cp = (cp << 6) | (b & 0x3F);
    if (++seen == needed) {
      emit(cp);
      cp = 0;
      needed = 0;
      seen = 0;
    }
  }
  if (needed != 0) {
    *o++ = 0xFFFD;
  }
  return size_t(o - out);
}

char16_t swapped(char16_t unit) {
  return char16_t((uint16_t(unit) << 8) | (uint16_t(unit) >> 8));
}

using convert_fn = size_t (*)(const char *, size_t, char16_t *);
using convert_with_locations_fn = size_t (*)(
    const char *, size_t, char16_t *, const simdutf::utf8_to_utf16_result &);

void check_units(const std::vector<char16_t> &out, size_t written,
                 const std::vector<char16_t> &oracle, bool swap) {
  ASSERT_EQUAL(written, oracle.size());
  for (size_t i = 0; i < oracle.size(); i++) {
    const char16_t expect = swap ? swapped(oracle[i]) : oracle[i];
    ASSERT_EQUAL(out[i], expect);
  }
  for (size_t i = written; i < out.size(); i++) {
    ASSERT_EQUAL(out[i], char16_t(0xCDCD));
  }
}

void check_convert(convert_fn convert, const char *input, size_t length,
                   const std::vector<char16_t> &oracle, bool swap) {
  std::vector<char16_t> out(oracle.size() + 32, char16_t(0xCDCD));
  const size_t written = convert(input, length, out.data());
  check_units(out, written, oracle, swap);
}

void check_convert_with_locations(
    convert_with_locations_fn convert, const char *input, size_t length,
    const std::vector<char16_t> &oracle, bool swap,
    const simdutf::utf8_to_utf16_result &locations) {
  std::vector<char16_t> out(oracle.size() + 32, char16_t(0xCDCD));
  const size_t written = convert(input, length, out.data(), locations);
  check_units(out, written, oracle, swap);
}

void check_locations(const char *input, size_t length,
                     const simdutf::utf8_to_utf16_result &length_result,
                     const simdutf::result &validation) {
  ASSERT_TRUE(length_result.error_count <=
              simdutf::utf8_to_utf16_result::max_errors);
  if (length_result.error == simdutf::error_code::SUCCESS) {
    ASSERT_EQUAL(length_result.error_count, size_t(0));
    ASSERT_FALSE(length_result.more_errors);
  } else if (length_result.error_count >= 1) {
    ASSERT_EQUAL(length_result.error_offset[0], validation.count);
  }
  if (length_result.error_count < simdutf::utf8_to_utf16_result::max_errors) {
    ASSERT_FALSE(length_result.more_errors);
  }
  for (size_t i = 0; i < length_result.error_count; i++) {
    ASSERT_TRUE(length_result.error_offset[i] < length);
    if (i > 0) {
      ASSERT_TRUE(length_result.error_offset[i] >
                  length_result.error_offset[i - 1]);
    }
  }
}

void check(const char *input, size_t length) {
  std::vector<char16_t> oracle(length + 1);
  const size_t oracle_n = oracle_whatwg(input, length, oracle.data());
  oracle.resize(oracle_n);

  const simdutf::utf8_to_utf16_result length_result =
      simdutf::utf16_length_from_utf8_with_replacement(input, length);
  ASSERT_EQUAL(length_result.count, oracle_n);
  const simdutf::result validation =
      simdutf::validate_utf8_with_errors(input, length);
  ASSERT_EQUAL(length_result.error, validation.error);
  check_locations(input, length, length_result, validation);

#if SIMDUTF_IS_BIG_ENDIAN
  check_convert(simdutf::convert_utf8_to_utf16le_with_replacement, input,
                length, oracle, true);
  check_convert(simdutf::convert_utf8_to_utf16be_with_replacement, input,
                length, oracle, false);
  check_convert(simdutf::convert_utf8_to_utf16_with_replacement, input, length,
                oracle, false);
  check_convert_with_locations(
      simdutf::convert_utf8_to_utf16le_with_replacement, input, length, oracle,
      true, length_result);
  check_convert_with_locations(
      simdutf::convert_utf8_to_utf16be_with_replacement, input, length, oracle,
      false, length_result);
  check_convert_with_locations(simdutf::convert_utf8_to_utf16_with_replacement,
                               input, length, oracle, false, length_result);
#else
  check_convert(simdutf::convert_utf8_to_utf16le_with_replacement, input,
                length, oracle, false);
  check_convert(simdutf::convert_utf8_to_utf16be_with_replacement, input,
                length, oracle, true);
  check_convert(simdutf::convert_utf8_to_utf16_with_replacement, input, length,
                oracle, false);
  check_convert_with_locations(
      simdutf::convert_utf8_to_utf16le_with_replacement, input, length, oracle,
      false, length_result);
  check_convert_with_locations(
      simdutf::convert_utf8_to_utf16be_with_replacement, input, length, oracle,
      true, length_result);
  check_convert_with_locations(simdutf::convert_utf8_to_utf16_with_replacement,
                               input, length, oracle, false, length_result);
#endif

  if (length_result.error == simdutf::error_code::SUCCESS) {
    ASSERT_EQUAL(simdutf::utf16_length_from_utf8(input, length), oracle_n);
    std::vector<char16_t> plain(oracle_n + 8, char16_t(0xCDCD));
    const size_t plain_n =
        simdutf::convert_utf8_to_utf16(input, length, plain.data());
    ASSERT_EQUAL(plain_n, oracle_n);
    for (size_t i = 0; i < oracle_n; i++) {
      ASSERT_EQUAL(plain[i], oracle[i]);
    }
  }
}

void check(const std::string &input) { check(input.data(), input.size()); }

void expect(const std::string &input, const std::vector<char16_t> &units,
            simdutf::error_code error) {
  check(input);
  const simdutf::utf8_to_utf16_result length_result =
      simdutf::utf16_length_from_utf8_with_replacement(input.data(),
                                                       input.size());
  ASSERT_EQUAL(length_result.error, error);
  ASSERT_EQUAL(length_result.count, units.size());
  std::vector<char16_t> out(units.size());
  const size_t written = simdutf::convert_utf8_to_utf16_with_replacement(
      input.data(), input.size(), out.data());
  ASSERT_EQUAL(written, units.size());
  for (size_t i = 0; i < units.size(); i++) {
    ASSERT_EQUAL(out[i], units[i]);
  }
}

const char16_t FFFD = 0xFFFD;

} // namespace

TEST(empty_input) {
  check("", 0);
  const simdutf::utf8_to_utf16_result length_result =
      simdutf::utf16_length_from_utf8_with_replacement("", 0);
  ASSERT_EQUAL(length_result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(length_result.count, size_t(0));
  ASSERT_EQUAL(length_result.error_count, size_t(0));
  ASSERT_FALSE(length_result.more_errors);
  ASSERT_EQUAL(simdutf::convert_utf8_to_utf16_with_replacement("", 0, nullptr),
               size_t(0));
}

TEST(explicit_sequences) {
  expect("AB", {u'A', u'B'}, simdutf::error_code::SUCCESS);
  expect(std::string("\xFF", 1), {FFFD}, simdutf::error_code::HEADER_BITS);
  expect(std::string("\x80", 1), {FFFD}, simdutf::error_code::TOO_LONG);
  expect(std::string("\xC2", 1), {FFFD}, simdutf::error_code::TOO_SHORT);
  expect(std::string("\xC2\xA9", 2), {0x00A9}, simdutf::error_code::SUCCESS);
  expect(std::string("\xCE\xB1", 2), {0x03B1}, simdutf::error_code::SUCCESS);
  expect(std::string("\xE2\x82\xAC", 3), {0x20AC},
         simdutf::error_code::SUCCESS);
  expect(std::string("\xF0\x9F\x98\x80", 4), {0xD83D, 0xDE00},
         simdutf::error_code::SUCCESS);
  expect(std::string("\xED\xA0\x80", 3), {FFFD, FFFD, FFFD},
         simdutf::error_code::SURROGATE);
  expect(std::string("\xE0\x80\x80", 3), {FFFD, FFFD, FFFD},
         simdutf::error_code::OVERLONG);
  expect(std::string("\xF4\x90\x80\x80", 4), {FFFD, FFFD, FFFD, FFFD},
         simdutf::error_code::TOO_LARGE);
  expect(std::string("\xC0\x80", 2), {FFFD, FFFD},
         simdutf::error_code::OVERLONG);
  expect(std::string("\xE2\x82", 2), {FFFD}, simdutf::error_code::TOO_SHORT);
  expect(std::string("\xE2\x82\x28", 3), {FFFD, u'('},
         simdutf::error_code::TOO_SHORT);
  expect(std::string("\xE0\xA0", 2), {FFFD}, simdutf::error_code::TOO_SHORT);
  expect(std::string("\xF0\x90\x80", 3), {FFFD},
         simdutf::error_code::TOO_SHORT);
  expect(std::string("\xED\x9F", 2), {FFFD}, simdutf::error_code::TOO_SHORT);
  expect(std::string("\xF4\x8F\xBF", 3), {FFFD},
         simdutf::error_code::TOO_SHORT);
  expect(std::string("\x61\xF1\x80\x80\xE1\x80\xC2\x62", 8),
         {u'a', FFFD, FFFD, FFFD, u'b'}, simdutf::error_code::TOO_SHORT);
  expect(std::string("A\xFF"
                     "B",
                     3),
         {u'A', FFFD, u'B'}, simdutf::error_code::HEADER_BITS);
  expect(std::string("A\xFF\x80", 3), {u'A', FFFD, FFFD},
         simdutf::error_code::HEADER_BITS);
}

TEST(long_valid_and_sparse_and_dense) {
  std::string valid;
  valid.reserve(5000);
  for (int i = 0; i < 500; i++) {
    valid += 'A';
    valid += "\xC2\xA9";
    valid += "\xE2\x82\xAC";
    valid += "\xF0\x9F\x98\x80";
  }
  check(valid);

  std::string one_error = valid;
  one_error[2000] = char(0xFF);
  check(one_error);

  std::string every_4k = valid;
  for (size_t i = 0; i < every_4k.size(); i += 4096) {
    every_4k[i] = char(0xFF);
  }
  check(every_4k);

  std::string every_64 = valid;
  for (size_t i = 0; i < every_64.size(); i += 64) {
    every_64[i] = char(0xFF);
  }
  check(every_64);

  const std::string many_ff(3000, char(0xFF));
  check(many_ff);
  const simdutf::utf8_to_utf16_result many =
      simdutf::utf16_length_from_utf8_with_replacement(many_ff.data(),
                                                       many_ff.size());
  ASSERT_EQUAL(many.error_count, simdutf::utf8_to_utf16_result::max_errors);
  ASSERT_TRUE(many.more_errors);
  ASSERT_EQUAL(many.count, many_ff.size());
  for (size_t i = 0; i < many.error_count; i++) {
    ASSERT_EQUAL(many.error_offset[i], i);
  }
  check(std::string(100, '\x80'));
}

TEST(random_bytes) {
  std::mt19937 rng(1234);
  std::uniform_int_distribution<int> length_dist(0, 280);
  std::uniform_int_distribution<int> byte_dist(0, 255);
  for (int trial = 0; trial < 400; trial++) {
    std::string input(size_t(length_dist(rng)), '\0');
    for (char &c : input) {
      c = char(byte_dist(rng));
    }
    check(input);
  }
}

#if SIMDUTF_CPLUSPLUS23

struct converted3 {
  size_t n;
  std::array<char16_t, 3> out{};
};

constexpr converted3 convert_bad_native() {
  constexpr std::array<char, 3> bad{'A', char(0xFF), 'B'};
  converted3 result{};
  result.n = simdutf::convert_utf8_to_utf16_with_replacement(
      std::span<const char>(bad.data(), bad.size()),
      std::span<char16_t>(result.out));
  return result;
}

constexpr converted3 convert_bad_be() {
  constexpr std::array<char, 3> bad{'A', char(0xFF), 'B'};
  converted3 result{};
  result.n = simdutf::convert_utf8_to_utf16be_with_replacement(
      std::span<const char>(bad.data(), bad.size()),
      std::span<char16_t>(result.out));
  return result;
}

constexpr converted3 convert_bad_native_with_locations() {
  constexpr std::array<char, 3> bad{'A', char(0xFF), 'B'};
  constexpr simdutf::utf8_to_utf16_result locations =
      simdutf::utf16_length_from_utf8_with_replacement(
          std::span<const char>(bad.data(), bad.size()));
  converted3 result{};
  result.n = simdutf::convert_utf8_to_utf16_with_replacement(
      std::span<const char>(bad.data(), bad.size()),
      std::span<char16_t>(result.out), locations);
  return result;
}

TEST(constexpr_length_and_convert) {
  constexpr std::array<char, 3> bad{'A', char(0xFF), 'B'};
  constexpr simdutf::utf8_to_utf16_result bad_length =
      simdutf::utf16_length_from_utf8_with_replacement(
          std::span<const char>(bad.data(), bad.size()));
  static_assert(bad_length.count == 3);
  static_assert(bad_length.error == simdutf::error_code::HEADER_BITS);
  static_assert(bad_length.error_count == 1);
  static_assert(bad_length.error_offset[0] == 1);
  static_assert(!bad_length.more_errors);

  constexpr converted3 converted = convert_bad_native();
  static_assert(converted.n == 3);
  static_assert(converted.out[0] == u'A');
  static_assert(converted.out[1] == char16_t(0xFFFD));
  static_assert(converted.out[2] == u'B');

  constexpr converted3 converted_be = convert_bad_be();
  static_assert(converted_be.n == 3);
  #if SIMDUTF_IS_BIG_ENDIAN
  static_assert(converted_be.out[0] == u'A');
  static_assert(converted_be.out[1] == char16_t(0xFFFD));
  static_assert(converted_be.out[2] == u'B');
  #else
  static_assert(converted_be.out[0] == char16_t(0x4100));
  static_assert(converted_be.out[1] == char16_t(0xFDFF));
  static_assert(converted_be.out[2] == char16_t(0x4200));
  #endif

  constexpr std::array<char, 4> emoji{char(0xF0), char(0x9F), char(0x98),
                                      char(0x80)};
  constexpr simdutf::utf8_to_utf16_result emoji_length =
      simdutf::utf16_length_from_utf8_with_replacement(
          std::span<const char>(emoji.data(), emoji.size()));
  static_assert(emoji_length.count == 2);
  static_assert(emoji_length.error == simdutf::error_code::SUCCESS);
  static_assert(emoji_length.error_count == 0);
  static_assert(!emoji_length.more_errors);

  constexpr simdutf::utf8_to_utf16_result empty_length =
      simdutf::utf16_length_from_utf8_with_replacement(std::span<const char>{});
  static_assert(empty_length.count == 0);
  static_assert(empty_length.error == simdutf::error_code::SUCCESS);
  static_assert(empty_length.error_count == 0);

  constexpr converted3 converted_with_locations =
      convert_bad_native_with_locations();
  static_assert(converted_with_locations.n == 3);
  static_assert(converted_with_locations.out[0] == u'A');
  static_assert(converted_with_locations.out[1] == char16_t(0xFFFD));
  static_assert(converted_with_locations.out[2] == u'B');

  const simdutf::utf8_to_utf16_result runtime =
      simdutf::utf16_length_from_utf8_with_replacement(bad.data(), bad.size());
  ASSERT_EQUAL(runtime.count, bad_length.count);
  ASSERT_EQUAL(runtime.error, bad_length.error);
  ASSERT_EQUAL(runtime.error_count, bad_length.error_count);
  ASSERT_EQUAL(runtime.error_offset[0], bad_length.error_offset[0]);
  ASSERT_EQUAL(runtime.more_errors, bad_length.more_errors);
}

#endif

TEST_MAIN
