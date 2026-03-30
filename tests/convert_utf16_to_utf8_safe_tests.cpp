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
  constexpr auto N = std::min(actual.size(), expected.size());
  static_assert(expected.shrink<N>() == actual.shrink<N>());
}

#endif

TEST_MAIN
