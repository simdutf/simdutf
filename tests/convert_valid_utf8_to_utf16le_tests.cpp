#include "simdutf.h"

#include <array>
#include <memory>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
constexpr std::array<size_t, 9> input_size{7,   12,  16,  64,  67,
                                           128, 256, 511, 1000};
constexpr simdutf::endianness LE = simdutf::endianness::LITTLE;

using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;

constexpr size_t trials = 10000;
} // namespace

TEST(issue_641) {
  alignas(1) const unsigned char data[] = {
      0x20, 0x20, 0x20, 0x20, 0x20, 0xe4, 0xac, 0xa4, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe4, 0xac,
      0xa4, 0x20, 0x20, 0x20, 0xf2, 0x81, 0xaa, 0xa5, 0x20, 0x20};
  constexpr std::size_t data_len_bytes = sizeof(data);
  constexpr std::size_t data_len = data_len_bytes / sizeof(char);
  const auto validation1 =
      implementation.validate_utf8_with_errors((const char *)data, data_len);
  ASSERT_EQUAL(validation1.count, 32);
  ASSERT_EQUAL(validation1.error, simdutf::error_code::SUCCESS);

  const bool validation2 =
      implementation.validate_utf8((const char *)data, data_len);
  ASSERT_EQUAL(validation1.error == simdutf::error_code::SUCCESS, validation2);

  if (validation1.error != simdutf::error_code::SUCCESS) {
    return;
  }
  const auto outlen =
      implementation.utf16_length_from_utf8((const char *)data, data_len);
  ASSERT_EQUAL(outlen, 26);
  std::vector<char16_t> output(outlen);
  const auto r = implementation.convert_valid_utf8_to_utf16le(
      (const char *)data, data_len, output.data());
  ASSERT_EQUAL(r, 26);
  const std::vector<char16_t> expected_out{
#if SIMDUTF_IS_BIG_ENDIAN
      0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x244B, 0x2000, 0x2000, // 0-7
      0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, // 8-15
      0x2000, 0x2000, 0x244B, 0x2000, 0x2000, 0x2000, 0xC6D9, 0xA5DE, // 16-23
      0x2000, 0x2000                                                  // 24-25
#else
      0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x4B24, 0x0020, 0x0020, // 0-7
      0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, // 8-15
      0x0020, 0x0020, 0x4B24, 0x0020, 0x0020, 0x0020, 0xD9C6, 0xDEA5, // 16-23
      0x0020, 0x0020                                                  // 24-25
#endif
  };
  ASSERT_EQUAL(output.size(), expected_out.size());
  bool good = true;
  for (std::size_t i = 0; i < output.size(); ++i) {
    if (output.at(i) != expected_out.at(i)) {
      std::printf("pos %ld %ld!=%ld (actual vs expected)\n",
                  static_cast<long>(i), static_cast<long>(output.at(i)),
                  static_cast<long>(expected_out.at(i)));
      good = false;
    }
  };
  ASSERT_TRUE(good);
}

TEST_LOOP(trials, convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t { return counter++ & 0x7f; };

  auto procedure = [&implementation](const char *utf8, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16le(utf8, size, utf16);
  };

  for (size_t size : input_size) {
    transcode_utf8_to_utf16_test_base test(LE, generator, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(trials, convert_1_or_2_UTF8_bytes) {
  simdutf::tests::helpers::RandomInt random(
      0x0000, 0x07ff, seed); // range for 1 or 2 UTF-8 bytes

  auto procedure = [&implementation](const char *utf8, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16le(utf8, size, utf16);
  };

  for (size_t size : input_size) {
    transcode_utf8_to_utf16_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(trials, convert_1_or_2_or_3_UTF8_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0xd7ff}, {0xe000, 0xffff}}, seed);

  auto procedure = [&implementation](const char *utf8, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16le(utf8, size, utf16);
  };

  for (size_t size : input_size) {
    transcode_utf8_to_utf16_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(trials, convert_3_UTF8_bytes) {
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}}, seed); // range for 3 UTF-8 bytes

  auto procedure = [&implementation](const char *utf8, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16le(utf8, size, utf16);
  };

  for (size_t size : input_size) {
    transcode_utf8_to_utf16_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(trials, convert_3_or_4_UTF8_bytes) {
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}, {0xe000, 0x10ffff}},
      seed); // range for 3 or 4 UTF-8 bytes

  auto procedure = [&implementation](const char *utf8, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16le(utf8, size, utf16);
  };

  for (size_t size : input_size) {
    transcode_utf8_to_utf16_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(trials, convert_null_4_UTF8_bytes) {
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0x00000}, {0x10000, 0x10ffff}},
      seed); // range for 3 or 4 UTF-8 bytes

  auto procedure = [&implementation](const char *utf8, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16le(utf8, size, utf16);
  };

  for (size_t size : input_size) {
    transcode_utf8_to_utf16_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST(issue111) {
  // We stick to ASCII for our source code given that there is no universal way
  // to specify the character encoding of the source files.
  char16_t input[] =
      u"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\u30b3aa"
      u"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  const size_t utf16_len = sizeof(input) / sizeof(char16_t);
  to_utf16le_inplace(input, utf16_len);
  ASSERT_TRUE(implementation.validate_utf16le(input, utf16_len));
  ASSERT_EQUAL(implementation.utf8_length_from_utf16le(input, utf16_len),
               2 + utf16_len);
  size_t utf8_len = implementation.utf8_length_from_utf16le(input, utf16_len);
  std::unique_ptr<char[]> utf8_buffer{new char[utf8_len]};
  ASSERT_EQUAL(implementation.convert_valid_utf16le_to_utf8(input, utf16_len,
                                                            utf8_buffer.get()),
               utf8_len);

  std::unique_ptr<char16_t[]> utf16_buffer{new char16_t[utf16_len]};

  ASSERT_EQUAL(implementation.convert_valid_utf8_to_utf16le(
                   utf8_buffer.get(), utf8_len, utf16_buffer.get()),
               utf16_len);

  ASSERT_EQUAL(
      std::char_traits<char16_t>::compare(input, utf16_buffer.get(), utf16_len),
      0);
}

TEST(special_cases) {
  const uint8_t utf8[] = {0xC2, 0xA9};     // copyright sign
  const uint8_t expected[] = {0xA9, 0x00}; // expected UTF-16LE
  size_t utf16len =
      implementation.utf16_length_from_utf8((const char *)utf8, 2);
  ASSERT_EQUAL(utf16len, 1);
  std::unique_ptr<char16_t[]> utf16(new char16_t[utf16len]);
  size_t utf16size = implementation.convert_valid_utf8_to_utf16le(
      (const char *)utf8, 2, utf16.get());
  ASSERT_EQUAL(utf16size, utf16len);
  ASSERT_EQUAL(memcmp((const char *)utf16.get(), expected, 2), 0);
}

TEST_MAIN
