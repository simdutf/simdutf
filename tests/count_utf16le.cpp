#include "simdutf.h"

#include <array>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>

namespace {
constexpr std::array<size_t, 9> input_size{7,   12,  16,  64,  67,
                                           128, 256, 511, 1000};

} // namespace

TEST_LOOP(count_just_one_word) {
  simdutf::tests::helpers::random_utf16 random(seed, 1, 0);

  for (size_t size : input_size) {
    const auto generated = random.generate_counted_le(size);
    ASSERT_EQUAL(implementation.count_utf16le(generated.first.data(), size),
                 generated.second);
  }
}

TEST_LOOP(count_1_or_2_UTF16_words) {
  simdutf::tests::helpers::random_utf16 random(seed, 1, 1);

  for (size_t size : input_size) {
    auto generated = random.generate_counted_le(size);
    ASSERT_EQUAL(implementation.count_utf16le(generated.first.data(), size),
                 generated.second);
  }
}

TEST_LOOP(count_2_UTF16_words) {
  simdutf::tests::helpers::random_utf16 random(seed, 0, 1);

  for (size_t size : input_size) {
    const auto generated = random.generate_counted_le(size);
    ASSERT_EQUAL(implementation.count_utf16le(generated.first.data(), size),
                 generated.second);
  }
}

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_count_utf16) {
  using namespace simdutf::tests::helpers;

  static_assert(simdutf::count_utf16(u"köttbulle"_utf16) == 9);
}

TEST(compile_time_count_utf16le) {
  using namespace simdutf::tests::helpers;

  static_assert(simdutf::count_utf16(u"köttbulle"_utf16le) == 9);
}
#endif

TEST_MAIN
