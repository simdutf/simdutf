#include "simdutf.h"

#include <array>
#include <vector>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>

namespace {
std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

} // namespace

TEST_LOOP(count_just_one_word) {
  simdutf::tests::helpers::random_utf16 random(seed, 1, 0);

  for (size_t size : input_size) {
    const auto g = random.generate_counted_be(size);
    const auto &utf16 = g.first;
    const auto utf16_count = g.second;

    const size_t count = implementation.count_utf16be(utf16.data(), size);
    ASSERT_EQUAL(count, utf16_count);
  }
}

TEST_LOOP(count_1_or_2_UTF16_words) {
  simdutf::tests::helpers::random_utf16 random(seed, 1, 1);

  for (size_t size : input_size) {
    const auto g = random.generate_counted_be(size);
    const auto &utf16 = g.first;
    const auto utf16_count = g.second;

    const size_t count = implementation.count_utf16be(utf16.data(), size);
    ASSERT_EQUAL(count, utf16_count);
  }
}

TEST_LOOP(count_2_UTF16_words) {
  simdutf::tests::helpers::random_utf16 random(seed, 0, 1);

  for (size_t size : input_size) {
    const auto g = random.generate_counted_be(size);
    const auto &utf16 = g.first;
    const auto utf16_count = g.second;

    const size_t count = implementation.count_utf16be(utf16.data(), size);
    ASSERT_EQUAL(count, utf16_count);
  }
}

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_count_utf16be) {
  using namespace simdutf::tests::helpers;

  static_assert(simdutf::count_utf16be(u"köttbulle"_utf16be) == 9);
}
#endif

TEST_MAIN
