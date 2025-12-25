#include "simdutf.h"

#include <array>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

namespace {
std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

using simdutf::tests::helpers::transcode_utf8_to_latin1_test_base;

} // namespace

TEST_LOOP(convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint8_t { return counter++ & 0x7f; };

  auto procedure = [&implementation](const char *utf8, size_t size,
                                     char *latin1) -> size_t {
    return implementation.convert_valid_utf8_to_latin1(utf8, size, latin1);
  };
  auto size_procedure = [&implementation](const char *utf8,
                                          size_t size) -> size_t {
    return implementation.latin1_length_from_utf8(utf8, size);
  };

  for (size_t size : input_size) {
    transcode_utf8_to_latin1_test_base test(generator, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(convert_1_or_2_valid_UTF8_bytes_to_latin1) {
  simdutf::tests::helpers::RandomInt random(
      0x0000, 0x0ff, seed); // range for 1 or 2 UTF-8 bytes

  auto procedure = [&implementation](const char *utf8, size_t size,
                                     char *latin1) -> size_t {
    return implementation.convert_valid_utf8_to_latin1(utf8, size, latin1);
  };
  auto size_procedure = [&implementation](const char *utf8,
                                          size_t size) -> size_t {
    return implementation.latin1_length_from_utf8(utf8, size);
  };
  for (size_t size : input_size) {
    transcode_utf8_to_latin1_test_base test(random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

#if SIMDUTF_CPLUSPLUS23

namespace {
template <auto input> constexpr auto length() {
  return simdutf::latin1_length_from_utf8(input);
}
template <auto input> constexpr auto convert() {
  using namespace simdutf::tests::helpers;
  CTString<char, length<input>()> tmp;
  auto ret = simdutf::convert_valid_utf8_to_latin1(input, tmp);
  if (ret != tmp.size()) {
    throw "oops";
  }
  return tmp;
}
} // namespace

TEST(compile_time_convert_valid_utf8_to_latin1) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = u8"köttbulle"_utf8;
  constexpr auto expected = "k\xF6ttbulle"_latin1;
  constexpr auto actual = convert<input>();
  static_assert(actual == expected);
}

#endif

TEST_MAIN
