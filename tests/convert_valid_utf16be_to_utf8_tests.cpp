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
constexpr simdutf::endianness BE = simdutf::endianness::BIG;

using simdutf::tests::helpers::transcode_utf16_to_utf8_test_base;

} // namespace

TEST(convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t { return counter++ & 0x7f; };

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return implementation.convert_valid_utf16be_to_utf8(utf16, size, utf8);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, generator, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(convert_into_1_or_2_UTF8_bytes) {
  simdutf::tests::helpers::RandomInt random(
      0x0000, 0x07ff, seed); // range for 1 or 2 UTF-8 bytes

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return implementation.convert_valid_utf16be_to_utf8(utf16, size, utf8);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(convert_into_1_or_2_or_3_UTF8_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0x007f}, {0x0080, 0x07ff}, {0x0800, 0xd7ff}, {0xe000, 0xffff}},
      seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return implementation.convert_valid_utf16be_to_utf8(utf16, size, utf8);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(convert_into_3_or_4_UTF8_bytes) {
  // range for 3 or 4 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}, {0xe000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return implementation.convert_valid_utf16be_to_utf8(utf16, size, utf8);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf8_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST(all_possible_8_codepoint_combinations) {
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *utf8) -> size_t {
    return implementation.convert_valid_utf16be_to_utf8(utf16, size, utf8);
  };

  std::vector<char> output_utf8(256, ' ');
  const auto &combinations = all_utf16_combinations(BE);
  for (const auto &input_utf16 : combinations) {
    if (simdutf::tests::reference::validate_utf16(BE, input_utf16.data(),
                                                  input_utf16.size())) {
      transcode_utf16_to_utf8_test_base test(BE, input_utf16);
      ASSERT_TRUE(test(procedure));
    }
  }
}

#if SIMDUTF_CPLUSPLUS23

namespace {
template <auto input> constexpr auto size_be() {
  return simdutf::utf8_length_from_utf16be(input);
}
template <auto input> constexpr auto convert_be() {
  using namespace simdutf::tests::helpers;
  CTString<char8_t, size_be<input>()> tmp;
  const auto ret = simdutf::convert_valid_utf16be_to_utf8(input, tmp);
  if (ret != tmp.size()) {
    throw "unexpected write size";
  }
  return tmp;
}
} // namespace

TEST(compile_time_convert_utf16be_to_utf8_with_errors) {
  using namespace simdutf::tests::helpers;
  constexpr auto input = u"köttbulle"_utf16be;
  constexpr auto expected = u8"köttbulle"_utf8;
  constexpr auto output = convert_be<input>();
  static_assert(output == expected);
}

#endif
TEST_MAIN
