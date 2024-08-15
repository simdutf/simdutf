#include "simdutf.h"

#include <array>
#include <memory>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;

  constexpr size_t trials = 10000;
}

TEST(issue_514)
{
    alignas(1) const unsigned char data[]
        = {0x20, 0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20,
           0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x94, 0x9f,
           0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20,
           0x94, 0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20,
           0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x94, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x9f,
           0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20, 0x20, 0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe6, 0x94, 0x94,
           0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x20, 0x20, 0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20, 0x20, 0xe6, 0x94, 0x9f, 0x9f,
           0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x20, 0xe7, 0x94, 0x9f, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x20, 0xa3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe6, 0x94,
           0x94, 0x9f, 0x20, 0x20, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x20, 0xe7, 0x94, 0x9f, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x94,
           0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe6, 0x94, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20,
           0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x94, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x20, 0x9f, 0x20, 0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x20, 0xa3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
           0x20, 0x94, 0x9f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xa3, 0x20, 0x20,
           0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
    constexpr std::size_t data_len_bytes = sizeof(data);
    constexpr std::size_t data_len = data_len_bytes / sizeof(char);
    const auto validation1 = implementation.validate_utf8_with_errors((const char *) data, data_len);
    ASSERT_EQUAL(validation1.count, 4);
    ASSERT_EQUAL(validation1.error, simdutf::error_code::TOO_LONG);

    const bool validation2 = implementation.validate_utf8((const char *) data, data_len);
    ASSERT_EQUAL(validation1.error == simdutf::error_code::SUCCESS, validation2);

    const auto outlen = implementation.utf16_length_from_utf8((const char *) data, data_len);
    ASSERT_EQUAL(outlen, 300);
    std::vector<char16_t> output(outlen);
    const auto r = implementation.convert_utf8_to_utf16be((const char *) data,
                                                          data_len,
                                                          output.data());
    ASSERT_EQUAL(r, 0);
}

TEST(issue_convert_valid_utf8_to_utf16le_91498ee0f0fe77dd) {
    const unsigned char data[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                                  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                                  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                                  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                                  0x20, 0x20, 0xc0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                                  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
    constexpr std::size_t data_len_bytes = sizeof(data);
    constexpr std::size_t data_len = data_len_bytes / sizeof(char);
    const auto validation1 = implementation.validate_utf8_with_errors((const char *) data, data_len);
    ASSERT_EQUAL(validation1.count, 46);
    ASSERT_EQUAL(validation1.error, simdutf::error_code::TOO_SHORT);
    // strictly speaking, utf16_length_from_utf8 is not defined for invalid UTF-8
    const auto outlen = implementation.utf16_length_from_utf8((const char *) data, data_len);
    ASSERT_EQUAL(outlen, 64);
}

TEST_LOOP(trials, convert_pure_ASCII) {
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
      return counter++ & 0x7f;
    };

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16le) -> size_t {
      std::vector<char16_t> utf16be(2*size);  // Assume each UTF-8 byte is converted into two UTF-16 bytes
      size_t len = implementation.convert_utf8_to_utf16be(utf8, size, utf16be.data());
      implementation.change_endianness_utf16(utf16be.data(), len, utf16le);
      return len;
    };
    auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
      return implementation.utf16_length_from_utf8(utf8, size);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(generator, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
}

TEST_LOOP(trials, convert_1_or_2_UTF8_bytes) {
    simdutf::tests::helpers::RandomInt random(0x0000, 0x07ff, seed); // range for 1 or 2 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16le) -> size_t {
      std::vector<char16_t> utf16be(2*size);  // Assume each UTF-8 byte is converted into two UTF-16 bytes
      size_t len = implementation.convert_utf8_to_utf16be(utf8, size, utf16be.data());
      implementation.change_endianness_utf16(utf16be.data(), len, utf16le);
      return len;
    };
    auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
      return implementation.utf16_length_from_utf8(utf8, size);
    };
    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
}

TEST_LOOP(trials, convert_1_or_2_or_3_UTF8_bytes) {
    // range for 1, 2 or 3 UTF-8 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd7ff},
                                                     {0xe000, 0xffff}}, seed);

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16le) -> size_t {
      std::vector<char16_t> utf16be(2*size);  // Assume each UTF-8 byte is converted into two UTF-16 bytes
      size_t len = implementation.convert_utf8_to_utf16be(utf8, size, utf16be.data());
      implementation.change_endianness_utf16(utf16be.data(), len, utf16le);
      return len;
    };
    auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
      return implementation.utf16_length_from_utf8(utf8, size);
    };
    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
}

TEST_LOOP(trials, convert_3_or_4_UTF8_bytes) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed); // range for 3 or 4 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16le) -> size_t {
      std::vector<char16_t> utf16be(size);
      size_t len = implementation.convert_utf8_to_utf16be(utf8, size, utf16be.data());
      implementation.change_endianness_utf16(utf16be.data(), len, utf16le);
      return len;
    };
    auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
      return implementation.utf16_length_from_utf8(utf8, size);
    };
    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
}

TEST(special_cases) {
  const uint8_t utf8[] = {0xC2, 0xA9}; // copyright sign
  const uint8_t expected[] = {0x00, 0xA9}; // expected UTF-16BE
  size_t utf16len = implementation.utf16_length_from_utf8((const char*)utf8, 2);
  ASSERT_EQUAL(utf16len, 1);
  std::unique_ptr<char16_t[]> utf16(new char16_t[utf16len]);
  size_t utf16size = implementation.convert_utf8_to_utf16be((const char*)utf8, 2, utf16.get());
  ASSERT_EQUAL(utf16size, utf16len);
  ASSERT_EQUAL(memcmp((const char*)utf16.get(), expected, 2), 0);
}

TEST_MAIN
