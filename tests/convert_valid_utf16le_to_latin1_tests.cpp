#include "simdutf.h"

#include <array>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
constexpr std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};
constexpr simdutf::endianness LE = simdutf::endianness::LITTLE;

using simdutf::tests::helpers::transcode_utf16_to_latin1_test_base;

constexpr int trials = 1000;
} // namespace

TEST_LOOP(trials, convert_2_UTF16_bytes) {
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

TEST_MAIN
