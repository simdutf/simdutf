#include "simdutf.h"

#include <array>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness LE = simdutf::endianness::LITTLE;

using simdutf::tests::helpers::transcode_utf32_to_utf16_test_base;

} // namespace

TEST_LOOP(convert_into_2_UTF16_bytes) {
  // range for 2 UTF-16 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0xd7ff}, {0xe000, 0xffff}}, seed);

  auto procedure = [&implementation](const char32_t *utf32, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf32_to_utf16le(utf32, size, utf16);
  };
  for (size_t size : input_size) {
    transcode_utf32_to_utf16_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(convert_into_4_UTF16_bytes) {
  // range for 4 UTF-16 bytes
  simdutf::tests::helpers::RandomIntRanges random({{0x10000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char32_t *utf32, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf32_to_utf16le(utf32, size, utf16);
  };
  for (size_t size : input_size) {
    transcode_utf32_to_utf16_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST_LOOP(convert_into_2_or_4_UTF16_bytes) {
  // range for 2 or 4 UTF-16 bytes (all codepoints)
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0xd7ff}, {0xe000, 0xffff}, {0x10000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char32_t *utf32, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_valid_utf32_to_utf16le(utf32, size, utf16);
  };
  for (size_t size : input_size) {
    transcode_utf32_to_utf16_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
  }
}

#if SIMDUTF_CPLUSPLUS23

namespace {
template <auto input> constexpr auto size() {
  return simdutf::utf16_length_from_utf32(input);
}
template <auto input> constexpr auto convert() {
  using namespace simdutf::tests::helpers;
  CTString<char16_t, size<input>()> tmp;
  const auto ret = simdutf::convert_valid_utf32_to_utf16(input, tmp);
  if (ret != tmp.size()) {
    throw "unexpected write size";
  }
  return tmp;
}
} // namespace

TEST(compile_time_convert_valid_utf32_to_utf16) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = U"köttbulle"_utf32;
  constexpr auto expected = u"köttbulle"_utf16;
  constexpr bool with_errors = true;
  constexpr auto output = convert<input>();
  static_assert(output == expected);
}

namespace {

template <auto input> constexpr auto convert_le() {
  using namespace simdutf::tests::helpers;
  CTString<char16_t, size<input>(), std::endian::little> tmp;
  const auto ret = simdutf::convert_valid_utf32_to_utf16le(input, tmp);
  if (ret != tmp.size()) {
    throw "unexpected write size";
  }
  return tmp;
}
} // namespace

TEST(compile_time_convert_valid_utf32_to_utf16le) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = U"köttbulle"_utf32;
  constexpr auto expected = u"köttbulle"_utf16le;
  constexpr bool with_errors = true;
  constexpr auto output = convert_le<input>();
  static_assert(output == expected);
}

#endif

TEST_MAIN
