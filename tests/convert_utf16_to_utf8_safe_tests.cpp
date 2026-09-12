#include "simdutf.h"

#include <array>
#include <vector>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/reference/validate_utf16.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
#if SIMDUTF_IS_BIG_ENDIAN
constexpr simdutf::endianness BE = simdutf::endianness::BIG;
#else
constexpr simdutf::endianness BE = simdutf::endianness::LITTLE;
#endif
using simdutf::tests::helpers::transcode_utf16_to_utf8_test_base;

} // namespace

inline void verify_subset(std::vector<char16_t> &utf16,
                          std::vector<char> &utf8) {
  size_t max_budget =
      simdutf::utf8_length_from_utf16(utf16.data(), utf16.size());
  std::vector<char> output_utf8(max_budget, ' ');
  size_t previous_size = 0;
  size_t i = 0;
  for (; i < max_budget; i += 7) {
    size_t ret = simdutf::convert_utf16_to_utf8_safe(
        utf16.data(), utf16.size(), output_utf8.data(), max_budget);
    ASSERT_TRUE(ret <= max_budget);
    ASSERT_TRUE(ret >= previous_size);
    for (size_t j = 0; j < ret; j++) {
      ASSERT_EQUAL(output_utf8[j], utf8[j]);
    }
    previous_size = ret;
  }
  for (; i < max_budget; i++) {
    size_t ret = simdutf::convert_utf16_to_utf8_safe(
        utf16.data(), utf16.size(), output_utf8.data(), max_budget);
    ASSERT_TRUE(ret <= max_budget);
    ASSERT_TRUE(ret >= previous_size);
    for (size_t j = 0; j < ret; j++) {
      ASSERT_EQUAL(output_utf8[j], utf8[j]);
    }
    previous_size = ret;
  }
  {
    size_t ret = simdutf::convert_utf16_to_utf8_safe(
        utf16.data(), utf16.size(), output_utf8.data(), max_budget);
    ASSERT_EQUAL(ret, max_budget);
    for (size_t j = 0; j < max_budget; j++) {
      ASSERT_EQUAL(output_utf8[j], utf8[j]);
    }
  }
}

TEST(issue911) {
  char16_t input[] = {0x00E9, 'A'};
  char output[2];
  size_t written = simdutf::convert_utf16_to_utf8_safe(input, 2, output, 2);
  ASSERT_TRUE(written <= 2);
}

TEST(safe_with_details_mixed_width) {
  const std::vector<char16_t> input{u'A',
                                    char16_t(0x00e9),
                                    char16_t(0x20ac),
                                    char16_t(0xd83d),
                                    char16_t(0xde00),
                                    u'B'};
  const size_t utf8_length =
      simdutf::utf8_length_from_utf16(input.data(), input.size());
  std::vector<char> expected(utf8_length);
  ASSERT_EQUAL(simdutf::convert_utf16_to_utf8(input.data(), input.size(),
                                              expected.data()),
               utf8_length);

  for (size_t output_size = 0; output_size <= utf8_length; output_size++) {
    std::vector<char> output(output_size);
    std::vector<char> legacy_output(output_size);
    const simdutf::full_result result =
        simdutf::convert_utf16_to_utf8_safe_with_details(
            input.data(), input.size(), output.data(), output.size());
    const size_t legacy_result = simdutf::convert_utf16_to_utf8_safe(
        input.data(), input.size(), legacy_output.data(), legacy_output.size());

    size_t expected_input_count = 0;
    size_t expected_output_count = 0;
    while (expected_input_count < input.size()) {
      const uint16_t word = input[expected_input_count];
      size_t input_width = 1;
      size_t output_width;
      if (word < 0x80) {
        output_width = 1;
      } else if (word < 0x800) {
        output_width = 2;
      } else if (word < 0xd800 || word > 0xdfff) {
        output_width = 3;
      } else {
        input_width = 2;
        output_width = 4;
      }
      if (expected_output_count + output_width > output_size) {
        break;
      }
      expected_input_count += input_width;
      expected_output_count += output_width;
    }

    ASSERT_EQUAL(result.input_count, expected_input_count);
    ASSERT_EQUAL(result.output_count, expected_output_count);
    ASSERT_EQUAL(legacy_result, result.output_count);
    ASSERT_EQUAL(result.error,
                 expected_input_count == input.size()
                     ? simdutf::error_code::SUCCESS
                     : simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL);
    ASSERT_TRUE(std::equal(output.begin(), output.begin() + result.output_count,
                           expected.begin()));
    ASSERT_TRUE(std::equal(legacy_output.begin(),
                           legacy_output.begin() + legacy_result,
                           output.begin()));
  }
}

TEST(safe_with_details_unpaired_surrogate) {
  std::vector<char16_t> input(64, u'A');
  input.push_back(char16_t(0xdc00));
  input.push_back(u'B');
  std::vector<char> output(128);

  const simdutf::full_result result =
      simdutf::convert_utf16_to_utf8_safe_with_details(
          input.data(), input.size(), output.data(), output.size());
  ASSERT_EQUAL(result.error, simdutf::error_code::SURROGATE);
  ASSERT_EQUAL(result.input_count, 64);
  ASSERT_EQUAL(result.output_count, 64);
  ASSERT_EQUAL(simdutf::convert_utf16_to_utf8_safe(
                   input.data(), input.size(), output.data(), output.size()),
               0);

  const simdutf::full_result full_output =
      simdutf::convert_utf16_to_utf8_safe_with_details(
          input.data(), input.size(), output.data(), 64);
  ASSERT_EQUAL(full_output.error, simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL);
  ASSERT_EQUAL(full_output.input_count, 64);
  ASSERT_EQUAL(full_output.output_count, 64);
}

TEST(safe_with_details_empty) {
  const simdutf::full_result result =
      simdutf::convert_utf16_to_utf8_safe_with_details(nullptr, 0, nullptr, 0);
  ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(result.input_count, 0);
  ASSERT_EQUAL(result.output_count, 0);
}

TEST(convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t { return counter++ & 0x7f; };

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return simdutf::convert_utf16_to_utf8_safe(
        utf16, size, utf8, simdutf::utf8_length_from_utf16(utf16, size));
  };
  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return simdutf::utf8_length_from_utf16(utf16, size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, generator, size);
    verify_subset(test.input_utf16, test.reference_output_utf8);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(convert_into_1_or_2_UTF8_bytes) {
  simdutf::tests::helpers::RandomInt random(
      0x0000, 0x07ff, seed); // range for 1 or 2 UTF-8 bytes

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return simdutf::convert_utf16_to_utf8_safe(
        utf16, size, utf8, simdutf::utf8_length_from_utf16(utf16, size));
  };
  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return simdutf::utf8_length_from_utf16(utf16, size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    verify_subset(test.input_utf16, test.reference_output_utf8);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(convert_into_1_or_2_or_3_UTF8_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0x007f}, {0x0080, 0x07ff}, {0x0800, 0xd7ff}, {0xe000, 0xffff}},
      seed);
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return simdutf::convert_utf16_to_utf8_safe(
        utf16, size, utf8, simdutf::utf8_length_from_utf16(utf16, size));
  };
  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return simdutf::utf8_length_from_utf16(utf16, size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    verify_subset(test.input_utf16, test.reference_output_utf8);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(convert_into_3_or_4_UTF8_bytes) {
  // range for 3 or 4 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}, {0xe000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return simdutf::convert_utf16_to_utf8_safe(
        utf16, size, utf8, simdutf::utf8_length_from_utf16(utf16, size));
  };
  auto size_procedure = [&implementation](const char16_t *utf16,
                                          size_t size) -> size_t {
    return simdutf::utf8_length_from_utf16(utf16, size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    verify_subset(test.input_utf16, test.reference_output_utf8);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

#if SIMDUTF_CPLUSPLUS23

namespace {
template <auto input> constexpr auto convert() {
  using namespace simdutf::tests::helpers;
  constexpr auto Noutput = simdutf::utf8_length_from_utf16(input);
  CTString<char8_t, Noutput> output{};
  const auto ret = simdutf::convert_utf16_to_utf8_safe(input, output);
  if (ret == 0) {
    throw "failed conversion";
  }
  if (ret != Noutput) {
    throw "mismatch in write length";
  }
  return output;
}
} // namespace

TEST(compile_time_convert_utf16_to_utf8_safe) {
  using namespace simdutf::tests::helpers;
  constexpr auto input = u"köttbulle"_utf16;
  constexpr auto expected = u8"köttbulle"_utf8;
  constexpr auto actual = convert<input>();
  static_assert(actual == expected);
}

namespace {
template <auto input, std::size_t buflen>
constexpr auto convert_insufficient_buf() {
  using namespace simdutf::tests::helpers;
  constexpr auto Noutput = simdutf::utf8_length_from_utf16(input);
  CTString<char8_t, buflen> output{};
  const auto ret = simdutf::convert_utf16_to_utf8_safe(input, output);
  if (ret == 0) {
    throw "failed conversion";
  }
  return output;
}
} // namespace
TEST(compile_time_check_of_issue_911) {
  using namespace simdutf::tests::helpers;
  constexpr auto input = u"\u00E9A"_utf16;
  constexpr auto expected = u8"\u00E9A"_utf8;
  constexpr auto actual = convert_insufficient_buf<input, 2>();
  constexpr auto N = simdutf::detail::min(actual.size(), expected.size());
  static_assert(expected.shrink<N>() == actual.shrink<N>());
}

TEST(compile_time_convert_utf16_to_utf8_safe_with_details) {
  using namespace simdutf::tests::helpers;
  constexpr auto result = []() {
    constexpr auto input = u"\u00E9A"_utf16;
    CTString<char8_t, 2> output{};
    return simdutf::convert_utf16_to_utf8_safe_with_details(input, output);
  }();
  static_assert(result.error == simdutf::OUTPUT_BUFFER_TOO_SMALL);
  static_assert(result.input_count == 1);
  static_assert(result.output_count == 2);
}

#endif

TEST_MAIN
