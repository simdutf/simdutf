#include "simdutf.h"

#include <vector>

#include <tests/helpers/compiletime_conversions.h>
#include <tests/helpers/fixed_string.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

namespace {
using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;

} // namespace

TEST_LOOP(convert_all_latin1) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint8_t { return counter++ & 0xFF; };

  auto procedure = [&implementation](const char *latin1, size_t size,
                                     char *utf8) -> size_t {
    return implementation.convert_latin1_to_utf8(latin1, size, utf8);
  };
  auto size_procedure = [&implementation](const char *latin1,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_latin1(latin1, size);
  };

  simdutf::tests::helpers::transcode_latin1_to_utf8_test_base test(generator,
                                                                   256);
  ASSERT_TRUE(test(procedure));
  ASSERT_TRUE(test.check_size(size_procedure));
}

TEST(convert_all_latin1_safe) {
  std::vector<char> latin1(1024);
  for (size_t i = 0; i < latin1.size(); i++) {
    latin1[i] = i & 0xff;
  }
  size_t utf8_length =
      implementation.utf8_length_from_latin1(latin1.data(), latin1.size());
  std::vector<char> utf8(utf8_length);
  const auto result = implementation.convert_latin1_to_utf8(
      latin1.data(), latin1.size(), utf8.data());
  ASSERT_EQUAL(result, utf8_length);
  for (size_t output_size = 0; output_size < utf8.size(); output_size++) {
    std::vector<char> utf8_buffer(output_size);
    size_t used_size = simdutf::convert_latin1_to_utf8_safe(
        latin1.data(), latin1.size(), utf8_buffer.data(), output_size);
    for (size_t i = 0; i < used_size; i++) {
      ASSERT_EQUAL(utf8_buffer[i], utf8[i]);
    }
    if (used_size < output_size) {
      ASSERT_EQUAL(used_size, output_size - 1);
      ASSERT_TRUE(uint8_t(utf8[used_size]) >= 0x80);
    }
  }
}

TEST(convert_all_latin1_safe_with_details) {
  std::vector<char> latin1(1024);
  for (size_t i = 0; i < latin1.size(); i++) {
    latin1[i] = i & 0xff;
  }
  const size_t utf8_length =
      simdutf::utf8_length_from_latin1(latin1.data(), latin1.size());
  std::vector<char> expected(utf8_length);
  ASSERT_EQUAL(simdutf::convert_latin1_to_utf8(latin1.data(), latin1.size(),
                                               expected.data()),
               utf8_length);

  for (size_t output_size = 0; output_size <= utf8_length; output_size++) {
    std::vector<char> output(output_size);
    std::vector<char> legacy_output(output_size);
    const simdutf::full_result result =
        simdutf::convert_latin1_to_utf8_safe_with_details(
            latin1.data(), latin1.size(), output.data(), output.size());
    const size_t legacy_result = simdutf::convert_latin1_to_utf8_safe(
        latin1.data(), latin1.size(), legacy_output.data(),
        legacy_output.size());

    size_t expected_input_count = 0;
    size_t expected_output_count = 0;
    while (expected_input_count < latin1.size()) {
      const size_t width = uint8_t(latin1[expected_input_count]) < 0x80 ? 1 : 2;
      if (expected_output_count + width > output_size) {
        break;
      }
      expected_input_count++;
      expected_output_count += width;
    }

    ASSERT_EQUAL(result.input_count, expected_input_count);
    ASSERT_EQUAL(result.output_count, expected_output_count);
    ASSERT_EQUAL(legacy_result, result.output_count);
    ASSERT_EQUAL(result.error,
                 expected_input_count == latin1.size()
                     ? simdutf::error_code::SUCCESS
                     : simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL);
    ASSERT_TRUE(std::equal(output.begin(), output.begin() + result.output_count,
                           expected.begin()));
    ASSERT_TRUE(std::equal(legacy_output.begin(),
                           legacy_output.begin() + legacy_result,
                           output.begin()));
  }
}

TEST(convert_latin1_to_utf8_safe_with_details_empty) {
  const simdutf::full_result result =
      simdutf::convert_latin1_to_utf8_safe_with_details(nullptr, 0, nullptr, 0);
  ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(result.input_count, 0);
  ASSERT_EQUAL(result.output_count, 0);
}

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_utf8_length_from_latin1) {
  using namespace simdutf::tests::helpers;
  static_assert(simdutf::utf8_length_from_latin1("x"_latin1) == 1);
  // swedish character "ö":
  static_assert(simdutf::utf8_length_from_latin1("\xF6"_latin1) == 2);
}

TEST(compile_time_convert_latin1_to_utf8) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = "I am a nice and wellbehaved string"_latin1;
  constexpr auto expected = u8"I am a nice and wellbehaved string"_utf8;
  static_assert(simdutf::utf8_length_from_latin1(input) == expected.size());
  constexpr auto converted = to_utf8<input>();
  static_assert(converted == expected);
}

TEST(compile_time_convert_latin1_to_utf8_harder) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = "k\xF6ttbulle"_latin1;
  constexpr auto expected = u8"köttbulle"_utf8;
  static_assert(simdutf::utf8_length_from_latin1(input) == expected.size());
  constexpr auto converted = to_utf8<input>();
  static_assert(converted == expected);
}

namespace {
template <auto input, std::size_t N> constexpr auto convert_safe() {
  simdutf::tests::helpers::CTString<char8_t, N> ret{};
  auto written = simdutf::convert_latin1_to_utf8_safe(input, ret);
  return std::tuple(written, ret);
}
} // namespace

TEST(compile_time_convert_latin1_to_utf8_safe) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = "k\xF6ttbulle"_latin1;
  constexpr auto expected = u8"köttbulle"_utf8;

  // convert using a too small buffer
  {
    constexpr auto small = convert_safe<input, 2>();
    constexpr auto written = std::get<0>(small);
    static_assert(written == 1);
  }

  // use a large enough buffer
  {
    constexpr auto large = convert_safe<input, 100>();
    constexpr auto written = std::get<0>(large);
    static_assert(written == expected.size());
    constexpr auto output = std::get<1>(large).shrink<written>();
    static_assert(output == expected);
  }
}

TEST(compile_time_convert_latin1_to_utf8_safe_with_details) {
  using namespace simdutf::tests::helpers;

  constexpr auto result = []() {
    constexpr auto input = "k\xF6ttbulle"_latin1;
    CTString<char8_t, 2> output{};
    return simdutf::convert_latin1_to_utf8_safe_with_details(input, output);
  }();
  static_assert(result.error == simdutf::OUTPUT_BUFFER_TOO_SMALL);
  static_assert(result.input_count == 1);
  static_assert(result.output_count == 1);
}
#endif

TEST_MAIN
