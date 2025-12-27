#include "simdutf.h"

#include <array>

#include <tests/helpers/compiletime_conversions.h>
#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness BE = simdutf::endianness::BIG;

using simdutf::tests::helpers::transcode_latin1_to_utf16_test_base;

} // namespace

TEST_LOOP(convert_all_latin) {
  // range for 2 UTF-16 bytes
  simdutf::tests::helpers::RandomIntRanges random({{0x00, 0xff}}, seed);

  auto procedure = [&implementation](const char *latin1, size_t size,
                                     char16_t *utf16) -> size_t {
    return implementation.convert_latin1_to_utf16be(latin1, size, utf16);
  };
  auto size_procedure =
      [&implementation](simdutf_maybe_unused const char *latin1,
                        size_t size) -> size_t {
    return implementation.utf16_length_from_latin1(size);
  };
  for (size_t size : input_size) {
    transcode_latin1_to_utf16_test_base test(BE, random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_convert_latin1_to_utf16be) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = "hello"_latin1;
  constexpr auto expected = u"hello"_utf16be;
  constexpr auto output = latin1_to_utf16<std::endian::big>(input);
  static_assert(output == expected);
}

#endif

TEST_MAIN
