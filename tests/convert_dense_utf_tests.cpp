#include "simdutf.h"

#include <algorithm>
#include <string>
#include <vector>

#include <tests/helpers/test.h>

namespace {

char16_t as_endian(uint16_t native, bool big) {
  if (!big) {
    return char16_t(native);
  }
  return char16_t(uint16_t((native >> 8) | (native << 8)));
}

void expect_equal_u16(const std::vector<char16_t> &got,
                      const std::vector<char16_t> &expected, size_t n) {
  ASSERT_EQUAL(n, expected.size());
  for (size_t i = 0; i < n; i++) {
    ASSERT_EQUAL(uint16_t(got[i]), uint16_t(expected[i]));
  }
  ASSERT_EQUAL(uint16_t(got[n]), uint16_t(0xA5A5));
}

void expect_equal_u8(const std::vector<char> &got,
                     const std::string &expected) {
  ASSERT_EQUAL(got.size(), expected.size() + 1);
  for (size_t i = 0; i < expected.size(); i++) {
    ASSERT_EQUAL(uint8_t(got[i]), uint8_t(expected[i]));
  }
  ASSERT_EQUAL(uint8_t(got.back()), uint8_t(0xA5));
}

std::string repeat_utf8(const std::string &seq, size_t n) {
  std::string out;
  out.reserve(seq.size() * n);
  for (size_t i = 0; i < n; i++) {
    out += seq;
  }
  return out;
}

std::vector<char16_t> repeat_units(uint16_t a, uint16_t b, size_t pairs,
                                   bool big) {
  std::vector<char16_t> out;
  out.reserve(pairs * 2);
  for (size_t i = 0; i < pairs; i++) {
    out.push_back(as_endian(a, big));
    out.push_back(as_endian(b, big));
  }
  return out;
}

std::vector<char16_t> repeat_bmp(uint16_t u, size_t n, bool big) {
  std::vector<char16_t> out(n);
  for (size_t i = 0; i < n; i++) {
    out[i] = as_endian(u, big);
  }
  return out;
}

size_t utf8_to_utf16(const simdutf::implementation &impl, bool big,
                     const char *in, size_t len, char16_t *out) {
  return big ? impl.convert_utf8_to_utf16be(in, len, out)
             : impl.convert_utf8_to_utf16le(in, len, out);
}

size_t valid_utf8_to_utf16(const simdutf::implementation &impl, bool big,
                           const char *in, size_t len, char16_t *out) {
  return big ? impl.convert_valid_utf8_to_utf16be(in, len, out)
             : impl.convert_valid_utf8_to_utf16le(in, len, out);
}

simdutf::result utf8_to_utf16_err(const simdutf::implementation &impl, bool big,
                                  const char *in, size_t len, char16_t *out) {
  return big ? impl.convert_utf8_to_utf16be_with_errors(in, len, out)
             : impl.convert_utf8_to_utf16le_with_errors(in, len, out);
}

size_t utf16_to_utf8(const simdutf::implementation &impl, bool big,
                     const char16_t *in, size_t len, char *out) {
  return big ? impl.convert_utf16be_to_utf8(in, len, out)
             : impl.convert_utf16le_to_utf8(in, len, out);
}

size_t valid_utf16_to_utf8(const simdutf::implementation &impl, bool big,
                           const char16_t *in, size_t len, char *out) {
  return big ? impl.convert_valid_utf16be_to_utf8(in, len, out)
             : impl.convert_valid_utf16le_to_utf8(in, len, out);
}

simdutf::result utf16_to_utf8_err(const simdutf::implementation &impl, bool big,
                                  const char16_t *in, size_t len, char *out) {
  return big ? impl.convert_utf16be_to_utf8_with_errors(in, len, out)
             : impl.convert_utf16le_to_utf8_with_errors(in, len, out);
}

size_t utf16_length(const simdutf::implementation &impl, const char *in,
                    size_t len) {
  return impl.utf16_length_from_utf8(in, len);
}

size_t utf8_length(const simdutf::implementation &impl, bool big,
                   const char16_t *in, size_t len) {
  return big ? impl.utf8_length_from_utf16be(in, len)
             : impl.utf8_length_from_utf16le(in, len);
}

void check_emoji_utf8_to_utf16(const simdutf::implementation &impl, bool big,
                               size_t n) {
  const std::string utf8 = repeat_utf8("\xF0\x9F\x98\x80", n);
  std::vector<char16_t> expected = repeat_units(0xD83D, 0xDE00, n, big);
  const size_t need = utf16_length(impl, utf8.data(), utf8.size());
  ASSERT_EQUAL(need, expected.size());
  std::vector<char16_t> out(need + 1, char16_t(0xA5A5));
  const size_t got =
      utf8_to_utf16(impl, big, utf8.data(), utf8.size(), out.data());
  expect_equal_u16(out, expected, got);

  std::fill(out.begin(), out.end(), char16_t(0xA5A5));
  const size_t got_valid =
      valid_utf8_to_utf16(impl, big, utf8.data(), utf8.size(), out.data());
  expect_equal_u16(out, expected, got_valid);

  std::fill(out.begin(), out.end(), char16_t(0xA5A5));
  const simdutf::result res =
      utf8_to_utf16_err(impl, big, utf8.data(), utf8.size(), out.data());
  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  expect_equal_u16(out, expected, res.count);
}

void check_pairs_utf16_to_utf8(const simdutf::implementation &impl, bool big,
                               size_t pairs, uint16_t hi, uint16_t lo,
                               const std::string &seq) {
  const std::vector<char16_t> utf16 = repeat_units(hi, lo, pairs, big);
  const std::string expected = repeat_utf8(seq, pairs);
  const size_t need = utf8_length(impl, big, utf16.data(), utf16.size());
  ASSERT_EQUAL(need, expected.size());
  std::vector<char> out(need + 1, char(0xA5));
  const size_t got =
      utf16_to_utf8(impl, big, utf16.data(), utf16.size(), out.data());
  ASSERT_EQUAL(got, expected.size());
  expect_equal_u8(out, expected);

  std::fill(out.begin(), out.end(), char(0xA5));
  const size_t got_valid =
      valid_utf16_to_utf8(impl, big, utf16.data(), utf16.size(), out.data());
  ASSERT_EQUAL(got_valid, expected.size());
  expect_equal_u8(out, expected);

  std::fill(out.begin(), out.end(), char(0xA5));
  const simdutf::result res =
      utf16_to_utf8_err(impl, big, utf16.data(), utf16.size(), out.data());
  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(res.count, expected.size());
  expect_equal_u8(out, expected);
}

void check_bmp_both(const simdutf::implementation &impl, bool big, size_t n,
                    uint16_t u, const std::string &seq) {
  const std::string utf8 = repeat_utf8(seq, n);
  const std::vector<char16_t> utf16 = repeat_bmp(u, n, big);
  const size_t need16 = utf16_length(impl, utf8.data(), utf8.size());
  ASSERT_EQUAL(need16, utf16.size());
  std::vector<char16_t> out16(need16 + 1, char16_t(0xA5A5));
  const size_t got16 =
      utf8_to_utf16(impl, big, utf8.data(), utf8.size(), out16.data());
  expect_equal_u16(out16, utf16, got16);

  std::fill(out16.begin(), out16.end(), char16_t(0xA5A5));
  const size_t got16_valid =
      valid_utf8_to_utf16(impl, big, utf8.data(), utf8.size(), out16.data());
  expect_equal_u16(out16, utf16, got16_valid);

  const size_t need8 = utf8_length(impl, big, utf16.data(), utf16.size());
  ASSERT_EQUAL(need8, utf8.size());
  std::vector<char> out8(need8 + 1, char(0xA5));
  const size_t got8 =
      utf16_to_utf8(impl, big, utf16.data(), utf16.size(), out8.data());
  ASSERT_EQUAL(got8, utf8.size());
  expect_equal_u8(out8, utf8);

  std::fill(out8.begin(), out8.end(), char(0xA5));
  const simdutf::result res =
      utf16_to_utf8_err(impl, big, utf16.data(), utf16.size(), out8.data());
  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(res.count, utf8.size());
  expect_equal_u8(out8, utf8);
}

} // namespace

TEST(dense_emoji_utf8_to_utf16) {
  const size_t counts[] = {1, 7, 8, 9, 16, 17, 24, 100, 128, 144, 256, 1024};
  for (bool big : {false, true}) {
    for (size_t n : counts) {
      check_emoji_utf8_to_utf16(implementation, big, n);
    }
  }
}

TEST(dense_emoji_then_error) {
  for (bool big : {false, true}) {
    const std::string good = repeat_utf8("\xF0\x9F\x98\x80", 128);
    for (unsigned char bad : {0xFFu, 0x80u}) {
      std::string utf8 = good;
      utf8.push_back(char(bad));
      const size_t need =
          utf16_length(implementation, utf8.data(), utf8.size());
      std::vector<char16_t> out(need + 1, char16_t(0xA5A5));
      const simdutf::result res = utf8_to_utf16_err(
          implementation, big, utf8.data(), utf8.size(), out.data());
      ASSERT_EQUAL(res.error == simdutf::error_code::SUCCESS, false);
      ASSERT_EQUAL(res.count, good.size());
      ASSERT_EQUAL(uint16_t(out[need]), uint16_t(0xA5A5));
      const size_t conv = utf8_to_utf16(implementation, big, utf8.data(),
                                        utf8.size(), out.data());
      ASSERT_EQUAL(conv, 0);
    }
  }
}

TEST(dense_emoji_then_ascii) {
  for (bool big : {false, true}) {
    std::string utf8 = repeat_utf8("\xF0\x9F\x98\x80", 16);
    utf8 += "Abcd";
    std::vector<char16_t> expected = repeat_units(0xD83D, 0xDE00, 16, big);
    for (char c : {'A', 'b', 'c', 'd'}) {
      expected.push_back(as_endian(uint16_t(c), big));
    }
    std::vector<char16_t> out(expected.size() + 1, char16_t(0xA5A5));
    const size_t got = utf8_to_utf16(implementation, big, utf8.data(),
                                     utf8.size(), out.data());
    expect_equal_u16(out, expected, got);
  }
}

TEST(ascii_then_dense_emoji) {
  for (bool big : {false, true}) {
    std::string utf8(32, ' ');
    utf8 += repeat_utf8("\xF0\x9F\x98\x80", 16);
    std::vector<char16_t> expected(32, as_endian(0x20, big));
    auto pairs = repeat_units(0xD83D, 0xDE00, 16, big);
    expected.insert(expected.end(), pairs.begin(), pairs.end());
    std::vector<char16_t> out(expected.size() + 1, char16_t(0xA5A5));
    const size_t got = utf8_to_utf16(implementation, big, utf8.data(),
                                     utf8.size(), out.data());
    expect_equal_u16(out, expected, got);
  }
}

TEST(dense_bmp_both_ways) {
  const size_t counts[] = {1, 8, 9, 10, 16, 32, 64, 100};
  for (bool big : {false, true}) {
    for (size_t n : counts) {
      check_bmp_both(implementation, big, n, 0x4E00, "\xE4\xB8\x80");
      check_bmp_both(implementation, big, n, 0x0800, "\xE0\xA0\x80");
      check_bmp_both(implementation, big, n, 0xFFFF, "\xEF\xBF\xBF");
    }
  }
}

TEST(dense_cjk_then_emoji) {
  for (bool big : {false, true}) {
    std::string utf8 = repeat_utf8("\xE4\xB8\x80", 32);
    utf8 += repeat_utf8("\xF0\x9F\x98\x80", 16);
    std::vector<char16_t> expected = repeat_bmp(0x4E00, 32, big);
    auto pairs = repeat_units(0xD83D, 0xDE00, 16, big);
    expected.insert(expected.end(), pairs.begin(), pairs.end());
    std::vector<char16_t> out(expected.size() + 1, char16_t(0xA5A5));
    const size_t got = utf8_to_utf16(implementation, big, utf8.data(),
                                     utf8.size(), out.data());
    expect_equal_u16(out, expected, got);

    std::vector<char> back(utf8.size() + 1, char(0xA5));
    const size_t nbytes = utf16_to_utf8(implementation, big, expected.data(),
                                        expected.size(), back.data());
    ASSERT_EQUAL(nbytes, utf8.size());
    expect_equal_u8(back, utf8);
  }
}

TEST(dense_surrogate_pairs_to_utf8) {
  const size_t counts[] = {1, 8, 15, 16, 17, 32, 64, 100, 128, 144, 288};
  for (bool big : {false, true}) {
    for (size_t n : counts) {
      check_pairs_utf16_to_utf8(implementation, big, n, 0xD83D, 0xDE00,
                                "\xF0\x9F\x98\x80");
      check_pairs_utf16_to_utf8(implementation, big, n, 0xD800, 0xDC00,
                                "\xF0\x90\x80\x80");
      check_pairs_utf16_to_utf8(implementation, big, n, 0xDBFF, 0xDFFF,
                                "\xF4\x8F\xBF\xBF");
    }
  }
}

TEST(dense_pairs_then_lone_surrogate) {
  for (bool big : {false, true}) {
    std::vector<char16_t> utf16 = repeat_units(0xD83D, 0xDE00, 128, big);
    utf16.push_back(as_endian(0xD800, big));
    const size_t need =
        utf8_length(implementation, big, utf16.data(), utf16.size());
    std::vector<char> out(need + 1, char(0xA5));
    const simdutf::result res = utf16_to_utf8_err(
        implementation, big, utf16.data(), utf16.size(), out.data());
    ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
    ASSERT_EQUAL(res.count, 256);
    ASSERT_EQUAL(uint8_t(out.back()), uint8_t(0xA5));
    const size_t conv = utf16_to_utf8(implementation, big, utf16.data(),
                                      utf16.size(), out.data());
    ASSERT_EQUAL(conv, 0);
  }
}

TEST(dense_rejected_utf8) {
  for (bool big : {false, true}) {
    const std::string cases[] = {
        repeat_utf8("\xF0\x8F\xBF\xBF", 16), // overlong
        repeat_utf8("\xF4\x90\x80\x80", 16), // too large
        repeat_utf8("\xED\xA0\x80", 10),     // surrogate
        repeat_utf8("\xE0\x80\x80", 10),     // overlong 3-byte
    };
    for (const std::string &utf8 : cases) {
      const size_t need =
          utf16_length(implementation, utf8.data(), utf8.size());
      std::vector<char16_t> out(need + 1, char16_t(0xA5A5));
      const simdutf::result res = utf8_to_utf16_err(
          implementation, big, utf8.data(), utf8.size(), out.data());
      ASSERT_EQUAL(res.error == simdutf::error_code::SUCCESS, false);
      ASSERT_EQUAL(res.count, 0);
      ASSERT_EQUAL(uint16_t(out[need]), uint16_t(0xA5A5));
    }
  }
}

TEST(mispaired_surrogates_do_not_enter_the_fast_path) {
  for (bool big : {false, true}) {
    std::vector<char16_t> utf16 = repeat_units(0xDC00, 0xD800, 16, big);
    const size_t need =
        utf8_length(implementation, big, utf16.data(), utf16.size());
    std::vector<char> out(need + 1, char(0xA5));
    const simdutf::result res = utf16_to_utf8_err(
        implementation, big, utf16.data(), utf16.size(), out.data());
    ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
    ASSERT_EQUAL(res.count, 0);
    ASSERT_EQUAL(uint8_t(out.back()), uint8_t(0xA5));
  }
}

TEST(truncated_quad_after_dense_prefix) {
  for (bool big : {false, true}) {
    // 127 emoji is 508 bytes: 480 consumed by 32-byte steps, then a short tail.
    std::string utf8 = repeat_utf8("\xF0\x9F\x98\x80", 127);
    utf8.push_back(char(0xF0));
    const size_t need = utf16_length(implementation, utf8.data(), utf8.size());
    std::vector<char16_t> out(need + 1, char16_t(0xA5A5));
    const simdutf::result res = utf8_to_utf16_err(
        implementation, big, utf8.data(), utf8.size(), out.data());
    ASSERT_EQUAL(res.error, simdutf::error_code::TOO_SHORT);
    ASSERT_EQUAL(res.count, 508);
    ASSERT_EQUAL(uint16_t(out[need]), uint16_t(0xA5A5));
  }
}

TEST_MAIN
