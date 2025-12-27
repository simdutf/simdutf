#include "simdutf.h"

#include <array>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/reference/validate_utf16.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness LE = simdutf::endianness::LITTLE;

using simdutf::tests::helpers::transcode_utf16_to_utf32_test_base;

} // namespace

TEST_LOOP(convert_2_UTF16_bytes) {
  // range for 2-byte UTF-16 (no surrogate pairs)
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0x007f}, {0x0080, 0x07ff}, {0x0800, 0xd7ff}, {0xe000, 0xffff}},
      seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_valid_utf16le_to_utf32(utf16, size, utf32);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf32_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(convert_with_surrogate_pairs) {
  // some surrogate pairs
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}, {0xe000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_valid_utf16le_to_utf32(utf16, size, utf32);
  };

  for (size_t size : input_size) {
    transcode_utf16_to_utf32_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

#if 0 // XXX
TEST(all_possible_8_codepoint_combinations) {
  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_valid_utf16le_to_utf32(utf16, size, utf32);
  };

  std::vector<char> output_utf32(256, ' ');
  const auto &combinations = all_utf16_combinations(LE);
  for (const auto &input_utf16 : combinations) {
    if (simdutf::tests::reference::validate_utf16(LE, input_utf16.data(),
                                                  input_utf16.size())) {
      transcode_utf16_to_utf32_test_base test(LE, input_utf16);
      ASSERT_TRUE(test(procedure));
    }
  }
}
#endif

#if SIMDUTF_CPLUSPLUS23

namespace {
template <auto input> constexpr auto size() {
  return simdutf::utf32_length_from_utf16(input);
}

template <auto input> constexpr auto convert() {
  using namespace simdutf::tests::helpers;
  CTString<char32_t, size<input>()> tmp;
  const auto ret = simdutf::convert_valid_utf16_to_utf32(input, tmp);
  if (ret != tmp.size()) {
    throw "unexpected write size";
  }
  return tmp;
}
} // namespace

TEST(compile_time_convert_valid_utf16_to_utf32) {
  using namespace simdutf::tests::helpers;
  constexpr auto input = u"köttbulle"_utf16;
  constexpr auto expected = U"köttbulle"_utf32;
  constexpr auto output = convert<input>();
}

namespace {
template <auto input> constexpr auto size_le() {
  return simdutf::utf32_length_from_utf16le(input);
}

template <auto input> constexpr auto convert_le() {
  using namespace simdutf::tests::helpers;
  CTString<char32_t, size_le<input>()> tmp;
  const auto ret = simdutf::convert_valid_utf16le_to_utf32(input, tmp);
  if (ret != tmp.size()) {
    throw "unexpected write size";
  }
  return tmp;
}
} // namespace

TEST(compile_time_convert_valid_utf16le_to_utf32) {
  using namespace simdutf::tests::helpers;
  constexpr auto input = u"köttbulle"_utf16le;
  constexpr auto expected = U"köttbulle"_utf32;
  constexpr auto output = convert_le<input>();
  static_assert(output == expected);
}

#endif

TEST_MAIN
