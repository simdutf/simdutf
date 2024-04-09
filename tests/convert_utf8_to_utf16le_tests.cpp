#include "simdutf.h"

#include <array>
#include <iostream>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <memory>
#include <tests/helpers/random_utf8.h>

namespace {
  std::array<size_t, 9> input_size{7, 12, 16, 64, 67, 128, 256, 511, 1000};

  using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;

  constexpr size_t trials = 10000;
}


#include "reference/validate_utf8.h"

TEST(convert_check_validation) {
  fflush(NULL);
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf8 gen_1_2_3_4(seed, 1, 1, 1, 1);
  size_t total = 1000;
  for (size_t i = 0; i < total; i++) {
    auto UTF8 = gen_1_2_3_4.generate(rand() % 256);
    std::unique_ptr<char16_t[]> buffer(new char16_t[UTF8.size()]);
    ASSERT_TRUE(implementation.convert_utf8_to_utf16le((const char *)UTF8.data(), UTF8.size(), buffer.get()) > 0);
    for (size_t flip = 0; flip < 1000; ++flip) {
      // we are going to hack the string as long as it is UTF-8
      const int bitflip{1 << (rand() % 8)};
      UTF8[rand() % UTF8.size()] = uint8_t(bitflip); // we flip exactly one bit
      bool is_ok =
          (implementation.convert_utf8_to_utf16le((const char *)UTF8.data(), UTF8.size(), buffer.get()) > 0);
      bool is_ok_reference =
          simdutf::tests::reference::validate_utf8((const char *)UTF8.data(), UTF8.size());
      ASSERT_EQUAL(is_ok, is_ok_reference);
    }
  }
}

TEST(convert_check_validation_examples) {
  const char *goodsequences[] = {"a",
                                 "\xc3\xb1",
                                 "\xe2\x82\xa1",
                                 "\xf0\x90\x8c\xbc",
                                 "\xc2\x80",         // 6.7.2
                                 "\xf0\x90\x80\x80", // 6.7.4
                                 "\xee\x80\x80",     // 6.11.2
                                 "\xef\xbb\xbf"};
  const char *badsequences[] = {
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
      "\x25\x5b\x6e\x2c\x32\x2c\x5b\x5b\x33\x2c\x34\x2c\x05\x29\x2c\x33\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x35\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x20\x01\x01\x01\x01\x01\x02\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x23\x0a\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x7e\x7e\x0a\x0a\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x01\x01\x80\x01\x01\x01\x79\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01",
      "[[[[[[[[[[[[[[[\x80\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x010\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01",
      "\x20\x0b\x01\x01\x01\x64\x3a\x64\x3a\x64\x3a\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x30\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x80\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"};

  for (size_t i = 0; i < sizeof(goodsequences)/sizeof(goodsequences[0]); i++) {
    size_t len = std::strlen(goodsequences[i]);
    std::unique_ptr<char16_t[]> buffer(new char16_t[len]);
    ASSERT_TRUE(implementation.convert_utf8_to_utf16le(goodsequences[i], len, buffer.get()) > 0);
  }
  for (size_t i = 0; i < sizeof(badsequences)/sizeof(badsequences[0]); i++) {
    size_t len = std::strlen(badsequences[i]);
    std::unique_ptr<char16_t[]> buffer(new char16_t[len]);
    ASSERT_EQUAL(implementation.convert_utf8_to_utf16le(badsequences[i], len, buffer.get()), 0);
  }
}


TEST_LOOP(trials, convert_pure_ASCII) {
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
      return counter++ & 0x7f;
    };

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return implementation.convert_utf8_to_utf16le(utf8, size, utf16);
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
      return implementation.convert_utf8_to_utf16le(utf8, size, utf16);
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
      return implementation.convert_utf8_to_utf16le(utf8, size, utf16);
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

TEST_LOOP(trials, convert_3_UTF8_bytes) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0800, 0xd800-1}}, seed); // range for 3 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return implementation.convert_utf8_to_utf16le(utf8, size, utf16);
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
      return implementation.convert_utf8_to_utf16le(utf8, size, utf16);
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

TEST_LOOP(trials, convert_null_4_UTF8_bytes) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0x00000},
                                                     {0x10000, 0x10ffff}}, seed); // range for 3 or 4 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return implementation.convert_utf8_to_utf16le(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
}

#if SIMDUTF_IS_BIG_ENDIAN
// todo: port this test for big-endian platforms.
#else
TEST(issue111) {
  // We stick to ASCII for our source code given that there is no universal way to specify the character encoding of
  // the source files.
  char16_t input[] = u"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\u30b3aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  size_t utf16_len = sizeof(input) / sizeof(char16_t) - 1;
  ASSERT_TRUE(implementation.validate_utf16le(input, utf16_len));
  ASSERT_EQUAL(implementation.utf8_length_from_utf16le(input, utf16_len), 2 + utf16_len);
  size_t utf8_len = implementation.utf8_length_from_utf16le(input, utf16_len);
  std::unique_ptr<char[]> utf8_buffer{new char[utf8_len]};
  ASSERT_EQUAL(implementation.convert_utf16le_to_utf8(input, utf16_len, utf8_buffer.get()), utf8_len);

  std::unique_ptr<char16_t[]> utf16_buffer{new char16_t[utf16_len]};

  ASSERT_EQUAL(implementation.convert_utf8_to_utf16le(utf8_buffer.get(), utf8_len, utf16_buffer.get()), utf16_len);
  ASSERT_EQUAL(std::char_traits<char16_t>::compare(input, utf16_buffer.get(), utf16_len), 0);
}
#endif

TEST(special_cases) {
  const uint8_t utf8[] = {0xC2, 0xA9}; // copyright sign
  const uint8_t expected[] = {0xA9, 0x00}; // expected UTF-16LE
  size_t utf16len = implementation.utf16_length_from_utf8((const char*)utf8, 2);
  ASSERT_EQUAL(utf16len, 1);
  std::unique_ptr<char16_t[]> utf16(new char16_t[utf16len]);
  size_t utf16size = implementation.convert_utf8_to_utf16le((const char*)utf8, 2, utf16.get());
  ASSERT_EQUAL(utf16size, utf16len);
  ASSERT_EQUAL(memcmp((const char*)utf16.get(), expected, 2), 0);
}


template<typename T>
static void test_corrupt(T &implementation, uint32_t seed, simdutf::tests::helpers::random_utf8 gen_utf8) {
  std::mt19937 gen(seed);
  for (size_t i = 0; i < 10; i++) {
    auto UTF8 = gen_utf8.generate(1000);
    if (!implementation.validate_utf8((const char *)UTF8.data(), UTF8.size())) {
      std::cerr << "bug" << std::endl;
      ASSERT_TRUE(false);
    }
    std::unique_ptr<char16_t[]> buffer(new char16_t[UTF8.size()]);
    std::uniform_int_distribution<size_t> distIdx{0, UTF8.size()-1};
    for (size_t j = 0; j < 1000; ++j) {
      const size_t corrupt = distIdx(gen);
      uint8_t restore = UTF8[corrupt];
      UTF8[corrupt] = uint8_t(gen());
      bool is_ok =
          (implementation.convert_utf8_to_utf16le((const char *)UTF8.data(), UTF8.size(), buffer.get()) > 0);
      bool is_ok_basic =
          simdutf::tests::reference::validate_utf8((const char *)UTF8.data(), UTF8.size());
      if (is_ok != is_ok_basic) {
        std::cerr << "bug" << std::endl;
        ASSERT_TRUE(false);
      }
      UTF8[corrupt] = restore;
    }
  }
}

TEST(corrupt_1byte) {
  uint32_t seed{1234};
  test_corrupt(implementation, seed, simdutf::tests::helpers::random_utf8(seed, 1, 0, 0, 0));
}

TEST(corrupt_12byte) {
  uint32_t seed{1234};
  test_corrupt(implementation, seed, simdutf::tests::helpers::random_utf8(seed, 0, 1, 0, 0));
  test_corrupt(implementation, seed, simdutf::tests::helpers::random_utf8(seed, 1, 1, 0, 0));
}

TEST(corrupt_123byte) {
  uint32_t seed{1234};
  test_corrupt(implementation, seed, simdutf::tests::helpers::random_utf8(seed, 0, 0, 1, 0));
  test_corrupt(implementation, seed, simdutf::tests::helpers::random_utf8(seed, 0, 1, 1, 0));
  test_corrupt(implementation, seed, simdutf::tests::helpers::random_utf8(seed, 1, 0, 1, 0));
  test_corrupt(implementation, seed, simdutf::tests::helpers::random_utf8(seed, 1, 1, 1, 0));
}

TEST_MAIN
