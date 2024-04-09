#include "simdutf.h"

#include <array>
#include <iostream>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/random_utf8.h>
#include <tests/helpers/test.h>
#include <memory>

namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;

  constexpr size_t trials = 1000;
  constexpr size_t fix_size = 512;
}

TEST(issue_213) {
  const char* buf = "\x01\x9a\x84";
  // We select the byte 0x84. It is a continuation byte so it is possible
  // that the predicted output might be zero.
  size_t expected_size = implementation.utf16_length_from_utf8(buf + 2, 1);
  std::unique_ptr<char16_t[]>buffer(new char16_t[expected_size]);
  simdutf::result r = simdutf::convert_utf8_to_utf16le_with_errors(buf + 2, 1, buffer.get());
  ASSERT_TRUE(r.error != simdutf::SUCCESS);
  // r.count: In case of error, indicates the position of the error in the input.
  // In case of success, indicates the number of code units validated/written.
  ASSERT_TRUE(r.count == 0);
}

TEST_LOOP(trials, convert_pure_ASCII) {
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
      return counter++ & 0x7f;
    };

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
      ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(res.count, size);
      return res.count;
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

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
      ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
      return res.count;
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

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
      ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
      return res.count;
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

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
      ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
      return res.count;
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

TEST_LOOP(trials, header_bits_error) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed);

    transcode_utf8_to_utf16_test_base test(random, fix_size);

    for (int i = 0; i < fix_size; i++) {
      if((test.input_utf8[i] & 0b11000000) != 0b10000000) {  // Only process leading bytes
        auto procedure = [&implementation, &i](const char* utf8, size_t size, char16_t* utf16) -> size_t {
          simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
          ASSERT_EQUAL(res.error, simdutf::error_code::HEADER_BITS);
          ASSERT_EQUAL(res.count, i);
          return 0;
        };
        const unsigned char old = test.input_utf8[i];
        test.input_utf8[i] = uint8_t(0b11111000);
        ASSERT_TRUE(test(procedure));
        test.input_utf8[i] = old;
      }
    }
}

TEST_LOOP(trials, too_short_error) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed);
    transcode_utf8_to_utf16_test_base test(random, fix_size);
    int leading_byte_pos = 0;
    for (int i = 0; i < fix_size; i++) {
      if((test.input_utf8[i] & 0b11000000) == 0b10000000) {  // Only process continuation bytes by making them leading bytes
        auto procedure = [&implementation, &leading_byte_pos](const char* utf8, size_t size, char16_t* utf16) -> size_t {
          simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
          ASSERT_EQUAL(res.error, simdutf::error_code::TOO_SHORT);
          ASSERT_EQUAL(res.count, leading_byte_pos);
          return 0;
        };
        const unsigned char old = test.input_utf8[i];
        test.input_utf8[i] = uint8_t(0b11100000);
        ASSERT_TRUE(test(procedure));
        test.input_utf8[i] = old;
      } else {
        leading_byte_pos = i;
      }
    }
}

TEST_LOOP(trials, too_long_error) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed);
    transcode_utf8_to_utf16_test_base test(random, fix_size);
    for (int i = 1; i < fix_size; i++) {
      if(((test.input_utf8[i] & 0b11000000) != 0b10000000)) {  // Only process leading bytes by making them continuation bytes
        auto procedure = [&implementation, &i](const char* utf8, size_t size, char16_t* utf16) -> size_t {
          simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
          ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LONG);
          ASSERT_EQUAL(res.count, i);
          return 0;
        };
        const unsigned char old = test.input_utf8[i];
        test.input_utf8[i] = uint8_t(0b10000000);
        ASSERT_TRUE(test(procedure));
        test.input_utf8[i] = old;
      }
    }
}

TEST_LOOP(trials, overlong_error) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed);
    transcode_utf8_to_utf16_test_base test(random, fix_size);
    for (int i = 1; i < fix_size; i++) {
      if((unsigned char)test.input_utf8[i] >= (unsigned char)0b11000000) { // Only non-ASCII leading bytes can be overlong
        auto procedure = [&implementation, &i](const char* utf8, size_t size, char16_t* utf16) -> size_t {
          simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
          ASSERT_EQUAL(res.error, simdutf::error_code::OVERLONG);
          ASSERT_EQUAL(res.count, i);
          return 0;
        };
        const unsigned char old = test.input_utf8[i];
        const unsigned char second_old = test.input_utf8[i+1];
        if ((old & 0b11100000) == 0b11000000) { // two-bytes case, change to a value less or equal than 0x7f
          test.input_utf8[i] = char(0b11000000);
        } else if ((old & 0b11110000) == 0b11100000) {  // three-bytes case, change to a value less or equal than 0x7ff
          test.input_utf8[i] = char(0b11100000);
          test.input_utf8[i+1] = test.input_utf8[i+1] & 0b11011111;
        } else {  // four-bytes case, change to a value less or equal than 0xffff
          test.input_utf8[i] = char(0b11110000);
          test.input_utf8[i+1] = test.input_utf8[i+1] & 0b11001111;
        }
        ASSERT_TRUE(test(procedure));
        test.input_utf8[i] = old;
        test.input_utf8[i+1] = second_old;
      }
    }
}

TEST_LOOP(trials, too_large_error) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed);
    transcode_utf8_to_utf16_test_base test(random, fix_size);
    for (int i = 1; i < fix_size; i++) {
      if((test.input_utf8[i] & 0b11111000) == 0b11110000) { // Can only have too large error in 4-bytes case
        auto procedure = [&implementation, &i](const char* utf8, size_t size, char16_t* utf16) -> size_t {
          simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
          ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LARGE);
          ASSERT_EQUAL(res.count, i);
          return 0;
        };
        test.input_utf8[i] += ((test.input_utf8[i] & 0b100) == 0b100) ? 0b10 : 0b100;   // Make sure we get too large error and not header bits error
        ASSERT_TRUE(test(procedure));
        test.input_utf8[i] -= 0b100;
      }
    }
}

TEST_LOOP(trials, surrogate_error) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed);
    transcode_utf8_to_utf16_test_base test(random, fix_size);
    for (int i = 1; i < fix_size; i++) {
      if((test.input_utf8[i] & 0b11110000) == 0b11100000) { // Can only have surrogate error in 3-bytes case
        auto procedure = [&implementation, &i](const char* utf8, size_t size, char16_t* utf16) -> size_t {
          simdutf::result res = implementation.convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
          ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
          ASSERT_EQUAL(res.count, i);
          return 0;
        };
        const unsigned char old = test.input_utf8[i];
        const unsigned char second_old = test.input_utf8[i+1];
        test.input_utf8[i] = char(0b11101101);
        for (int s = 0x8; s < 0xf; s++) {  // Modify second byte to create a surrogate codepoint
          test.input_utf8[i+1] = (test.input_utf8[i+1] & 0b11000011) | (s << 2);
          ASSERT_TRUE(test(procedure));
        }
        test.input_utf8[i] = old;
        test.input_utf8[i+1] = second_old;
      }
    }
}

TEST_MAIN
