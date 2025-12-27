#include "simdutf.h"

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

namespace {
using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;

} // namespace

TEST_LOOP(convert_all_latin1) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t { return counter++ & 0xFF; };

  auto procedure = [&implementation](const char *latin1, size_t size,
                                     char32_t *utf32) -> size_t {
    return implementation.convert_latin1_to_utf32(latin1, size, utf32);
  };
  auto size_procedure =
      [&implementation](simdutf_maybe_unused const char *latin1,
                        size_t size) -> size_t {
    return implementation.utf32_length_from_latin1(size);
  };
  // Check varying length inputs for upto 16 bytes
  for (size_t i = 240; i <= 256; i++) {
    simdutf::tests::helpers::transcode_latin1_to_utf32_test_base test(generator,
                                                                      i);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

#if SIMDUTF_CPLUSPLUS23

namespace {

template <auto input> constexpr auto convert() {
  using namespace simdutf::tests::helpers;
  CTString<char32_t, input.size()> tmp;
  auto N = simdutf::convert_latin1_to_utf32(input, tmp);
  if (N != input.size()) {
    throw "oops";
  }
  return tmp;
}

} // namespace

TEST(compile_time_convert_latin1_to_utf32) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = "hello"_latin1;
  constexpr auto expected = U"hello"_utf32;
  constexpr auto output = convert<input>();
  static_assert(output == expected);
}

TEST(compile_time_utf32_length_from_latin1) {
  static_assert(simdutf::utf32_length_from_latin1(42) == 42);
}

#endif

TEST_MAIN
