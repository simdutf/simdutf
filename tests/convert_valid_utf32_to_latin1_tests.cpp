#include "simdutf.h"

#include <array>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;
} // namespace

TEST(convert_latin1_only) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t { return counter++ & 0xFF; };

  auto procedure = [&implementation](const char32_t *utf32, size_t size,
                                     char *latin1) -> size_t {
    return implementation.convert_valid_utf32_to_latin1(utf32, size, latin1);
  };
  auto size_procedure =
      [&implementation](simdutf_maybe_unused const char32_t *utf32,
                        size_t size) -> size_t {
    return implementation.latin1_length_from_utf32(size);
  };
  for (size_t size : input_size) {
    simdutf::tests::helpers::transcode_utf32_to_latin1_test_base test(generator,
                                                                      size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_MAIN
