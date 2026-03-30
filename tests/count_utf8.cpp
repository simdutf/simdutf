#include "simdutf.h"

#include <array>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/random_utf8.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

namespace {
std::array<size_t, 10> input_size{7,
                                  12,
                                  16,
                                  64,
                                  67,
                                  128,
                                  256
#if !SIMDUTF_FAST_TEST
                                  ,
                                  511,
                                  1000,
                                  2000
#endif
};

using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;
} // namespace

TEST_LOOP(count_pure_ASCII) {
  simdutf::tests::helpers::random_utf8 random(seed, 1, 0, 0, 0);

  for (size_t size : input_size) {
    auto generated = random.generate_counted(size);
    ASSERT_EQUAL(
        implementation.count_utf8(
            reinterpret_cast<const char *>(generated.first.data()), size),
        generated.second);
  }
}

TEST_LOOP(count_1_or_2_UTF8_bytes) {
  simdutf::tests::helpers::random_utf8 random(seed, 1, 1, 0, 0);

  for (size_t size : input_size) {
    auto generated = random.generate_counted(size);
    ASSERT_EQUAL(
        implementation.count_utf8(
            reinterpret_cast<const char *>(generated.first.data()), size),
        generated.second);
  }
}

TEST_LOOP(count_1_or_2_or_3_UTF8_bytes) {
  simdutf::tests::helpers::random_utf8 random(seed, 1, 1, 1, 0);

  for (size_t size : input_size) {
    auto generated = random.generate_counted(size);
    ASSERT_EQUAL(
        implementation.count_utf8(
            reinterpret_cast<const char *>(generated.first.data()), size),
        generated.second);
  }
}

TEST_LOOP(count_1_2_3_or_4_UTF8_bytes) {
  simdutf::tests::helpers::random_utf8 random(seed, 1, 1, 1, 1);

  for (size_t size : input_size) {
    auto generated = random.generate_counted(size);
    ASSERT_EQUAL(
        implementation.count_utf8(
            reinterpret_cast<const char *>(generated.first.data()), size),
        generated.second);
  }
}

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_count_utf8) {
  using namespace simdutf::tests::helpers;

  static_assert(simdutf::count_utf8(u8"köttbulle"_utf8) == 9);
}
#endif

TEST_MAIN
