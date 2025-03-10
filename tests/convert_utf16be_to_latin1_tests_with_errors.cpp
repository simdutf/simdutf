#include "simdutf.h"

#include <array>
#include <vector>

#include <tests/reference/validate_utf16.h>
#include <tests/reference/decode_utf16.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness BE = simdutf::endianness::BIG;

using simdutf::tests::helpers::transcode_utf16_to_latin1_test_base;

constexpr int trials = 1000;
} // namespace

TEST(issue_convert_utf16be_to_latin1_with_errors_461) {
  const unsigned char data[] = {0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
                                0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
                                0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
  constexpr std::size_t data_len_bytes = sizeof(data);
  constexpr std::size_t data_len = data_len_bytes / sizeof(char16_t);
  std::vector<char> output(4 * data_len);
  const auto r = implementation.convert_utf16be_to_latin1_with_errors(
      (const char16_t *)data, data_len, output.data());
  /*
  got return [count=13, error=TOO_LARGE] from implementation icelake
  got return [count=13, error=TOO_LARGE] from implementation haswell
  got return [count=13, error=TOO_LARGE] from implementation westmere
  got return [count=16, error=SUCCESS] from implementation fallback
  */
  ASSERT_EQUAL(r.count, 13);
  ASSERT_EQUAL(r.error, simdutf::error_code::TOO_LARGE);
}

TEST(issue_convert_utf16be_to_latin1_with_errors_cbf29ce484222384) {
  const unsigned char data[] = {0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
                                0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00};
  constexpr std::size_t data_len_bytes = sizeof(data);
  constexpr std::size_t data_len = data_len_bytes / sizeof(char16_t);
  std::vector<char> output(4 * data_len);
  const auto r = implementation.convert_utf16be_to_latin1_with_errors(
      (const char16_t *)data, data_len, output.data());
  /*
  got return [count=0, error=TOO_LARGE] from implementation icelake
  got return [count=0, error=TOO_LARGE] from implementation haswell
  got return [count=8, error=SUCCESS] from implementation westmere
  got return [count=0, error=TOO_LARGE] from implementation fallback
  */

  ASSERT_EQUAL(r.count, 0);
  ASSERT_EQUAL(r.error, simdutf::error_code::TOO_LARGE);
}

TEST_LOOP(trials, convert_2_UTF16_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0x00ff}}, seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *latin1) -> size_t {
    const simdutf::result res =
        implementation.convert_utf16be_to_latin1_with_errors(utf16, size,
                                                             latin1);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };
  auto size_procedure =
      [&implementation]([[maybe_unused]] const char16_t *utf16,
                        size_t size) -> size_t {
    return implementation.latin1_length_from_utf16(size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_latin1_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST(convert_fails_if_input_too_large) {
  uint32_t seed{1234};
  simdutf::tests::helpers::RandomInt generator(0xff, 0xffff, seed);

  const size_t size = 64;
  transcode_utf16_to_latin1_test_base test(BE, []() { return '*'; }, size + 32);

  for (size_t j = 0; j < 1000; j++) {
    const auto wrong_value = to_utf16be(generator());
    for (size_t i = 0; i < size; i++) {
      auto procedure = [&implementation, &i](const char16_t *utf16, size_t size,
                                             char *latin1) -> size_t {
        const simdutf::result res =
            implementation.convert_utf16be_to_latin1_with_errors(utf16, size,
                                                                 latin1);
        ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LARGE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };

      const auto old = test.input_utf16[i];
      test.input_utf16[i] = wrong_value;
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}

TEST_MAIN
