#include "simdutf.h"

#include <array>

#include <tests/helpers/compiletime_conversions.h>
#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness LE = simdutf::endianness::LITTLE;

using simdutf::tests::helpers::transcode_utf16_to_latin1_test_base;

} // namespace

TEST_LOOP(convert_2_UTF16_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomInt random(0x0000, 0x00ff, seed);

  auto procedure = [&implementation](const char16_t *utf16, size_t size,
                                     char *latin1) -> size_t {
    return implementation.convert_valid_utf16le_to_latin1(utf16, size, latin1);
  };
  auto size_procedure =
      [&implementation](simdutf_maybe_unused const char16_t *utf16,
                        size_t size) -> size_t {
    return implementation.latin1_length_from_utf16(size);
  };
  for (size_t size : input_size) {
    transcode_utf16_to_latin1_test_base test(LE, random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

#if SIMDUTF_CPLUSPLUS23

namespace {
template <auto input> constexpr auto size() {
  return simdutf::latin1_length_from_utf16(input.size());
}
template <auto input> constexpr auto convert() {
  using namespace simdutf::tests::helpers;
  CTString<char, size<input>()> tmp;
  const auto ret = simdutf::convert_valid_utf16_to_latin1(input, tmp);
  if (ret != tmp.size()) {
    throw "unexpected write size";
  }
  return tmp;
}
} // namespace

TEST(compile_time_test) {
  using namespace simdutf::tests::helpers;

  constexpr auto expected = "hello!"_latin1;

  static_assert(to_latin1(u"hello!"_utf16) == expected);
  static_assert(to_latin1(u"hello!"_utf16le) == expected);
  static_assert(to_latin1(u"hello!"_utf16be) == expected);
  static_assert(u"hello!"_utf16le[0] != u"hello!"_utf16be[0]);

  static_assert(convert<u"köttbulle"_utf16>() == "k\xF6ttbulle"_latin1);
}

#endif

TEST_MAIN
