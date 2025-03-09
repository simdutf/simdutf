#include "simdutf.h"

#include <array>
#include <vector>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness BE = simdutf::endianness::BIG;

using simdutf::tests::helpers::transcode_utf16_to_utf8_test_base;

constexpr int trials = 1000;
} // namespace

TEST(allow_empty_input) {
  std::vector<char16_t> emptydata;
  std::vector<char32_t> output(10);

  auto ret = implementation.convert_utf16be_to_utf32_with_errors(
      emptydata.data(), emptydata.size(), output.data());
  ASSERT_EQUAL(ret.error, simdutf::error_code::SUCCESS);
}

TEST(convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t { return counter++ & 0x7f; };

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    simdutf::result res =
        implementation.convert_utf16be_to_utf8_with_errors(utf16, size, utf8);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };

  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_utf16be(utf16, size);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, generator, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(trials, convert_into_1_or_2_UTF8_bytes) {
  simdutf::tests::helpers::RandomInt random(
      0x0000, 0x07ff, seed); // range for 1 or 2 UTF-8 bytes

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    simdutf::result res =
        implementation.convert_utf16be_to_utf8_with_errors(utf16, size, utf8);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };

  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_utf16be(utf16, size);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(trials, convert_into_1_or_2_or_3_UTF8_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0x007f}, {0x0080, 0x07ff}, {0x0800, 0xd7ff}, {0xe000, 0xffff}},
      seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    simdutf::result res =
        implementation.convert_utf16be_to_utf8_with_errors(utf16, size, utf8);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };
  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_utf16be(utf16, size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(trials, convert_into_3_or_4_UTF8_bytes) {
  // range for 3 or 4 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}, {0xe000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    const simdutf::result res =
        implementation.convert_utf16be_to_utf8_with_errors(utf16, size, utf8);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };
  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_utf16be(utf16, size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST(convert_fails_if_there_is_sole_low_surrogate) {
  const size_t size = 64;
  transcode_utf16_to_utf8_test_base test(BE, []() { return '*'; }, size + 32);

  for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff;
       low_surrogate++) {
    for (size_t i = 0; i < size; i++) {
      auto procedure = [&implementation, i](const char16_t *utf16, size_t size,
                                            char *utf8) -> size_t {
        simdutf::result res =
            implementation.convert_utf16be_to_utf8_with_errors(utf16, size,
                                                               utf8);
        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };
      const auto old = test.input_utf16[i];
      test.input_utf16[i] = to_utf16be(low_surrogate);
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}

TEST(convert_fails_if_there_is_sole_high_surrogate) {
  const size_t size = 64;
  transcode_utf16_to_utf8_test_base test(BE, []() { return '*'; }, size + 32);

  for (char16_t high_surrogate = 0xdc00; high_surrogate <= 0xdfff;
       high_surrogate++) {
    for (size_t i = 0; i < size; i++) {
      auto procedure = [&implementation, &i](const char16_t *utf16, size_t size,
                                             char *utf8) -> size_t {
        simdutf::result res =
            implementation.convert_utf16be_to_utf8_with_errors(utf16, size,
                                                               utf8);
        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };
      const auto old = test.input_utf16[i];
      test.input_utf16[i] = to_utf16be(high_surrogate);
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}

TEST(
    convert_fails_if_there_is_low_surrogate_is_followed_by_another_low_surrogate) {
  const size_t size = 64;
  transcode_utf16_to_utf8_test_base test(BE, []() { return '*'; }, size + 32);

  for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff;
       low_surrogate++) {
    for (size_t i = 0; i < size - 1; i++) {
      auto procedure = [&implementation, &i](const char16_t *utf16, size_t size,
                                             char *utf8) -> size_t {
        const simdutf::result res =
            implementation.convert_utf16be_to_utf8_with_errors(utf16, size,
                                                               utf8);
        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };
      const auto old0 = test.input_utf16[i + 0];
      const auto old1 = test.input_utf16[i + 1];
      test.input_utf16[i + 0] = to_utf16be(low_surrogate);
      test.input_utf16[i + 1] = to_utf16be(low_surrogate);
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i + 0] = old0;
      test.input_utf16[i + 1] = old1;
    }
  }
}

TEST(convert_fails_if_there_is_surrogate_pair_is_followed_by_high_surrogate) {
  const size_t size = 64;
  transcode_utf16_to_utf8_test_base test(BE, []() { return '*'; }, size + 32);

  const char16_t low_surrogate = to_utf16be(0xd801);
  const char16_t high_surrogate = to_utf16be(0xdc02);
  for (size_t i = 0; i < size - 2; i++) {
    auto procedure = [&implementation, &i](const char16_t *utf16, size_t size,
                                           char *utf8) -> size_t {
      const simdutf::result res =
          implementation.convert_utf16be_to_utf8_with_errors(utf16, size, utf8);
      ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
      ASSERT_EQUAL(res.count, i + 2);
      return 0;
    };
    const auto old0 = test.input_utf16[i + 0];
    const auto old1 = test.input_utf16[i + 1];
    const auto old2 = test.input_utf16[i + 2];
    test.input_utf16[i + 0] = low_surrogate;
    test.input_utf16[i + 1] = high_surrogate;
    test.input_utf16[i + 2] = high_surrogate;
    ASSERT_TRUE(test(procedure));
    test.input_utf16[i + 0] = old0;
    test.input_utf16[i + 1] = old1;
    test.input_utf16[i + 2] = old2;
  }
}

TEST(issue_445) {
  alignas(2) const unsigned char crash[] = {0x20, 0x20, 0xdd, 0x20};
  const unsigned int crash_len = 4;
  std::vector<char> output(4 * crash_len);
  const auto r = implementation.convert_utf16be_to_utf8_with_errors(
      (const char16_t *)crash, crash_len / sizeof(char16_t), output.data());
  ASSERT_EQUAL(r.count, 1);
  ASSERT_EQUAL(r.error, simdutf::error_code::SURROGATE);
}

TEST_MAIN
