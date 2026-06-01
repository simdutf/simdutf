#include "simdutf.h"

#include <tests/helpers/random_utf8.h>
#include <tests/reference/validate_utf8.h>
#include <tests/helpers/test.h>

template <typename T>
static void test_corrupt(T &implementation, uint32_t seed,
                         simdutf::tests::helpers::random_utf8 gen_utf8) {
  std::mt19937 gen(seed);
  for (size_t i = 0; i < 10; i++) {
    auto UTF8 = gen_utf8.generate(1000);
    ASSERT_TRUE(
        implementation.validate_utf8((const char *)UTF8.data(), UTF8.size()));
    std::uniform_int_distribution<size_t> distIdx{0, UTF8.size() - 1};
    for (size_t j = 0; j < 1000; ++j) {
      const size_t corrupt = distIdx(gen);
      uint8_t restore = UTF8[corrupt];
      UTF8[corrupt] = uint8_t(gen());
      simdutf::result res_ref = implementation.validate_utf8_with_errors(
          (const char *)UTF8.data(), UTF8.size());
      size_t expected_utf16_length = implementation.utf16_length_from_utf8(
          (const char *)UTF8.data(), res_ref.count);
      simdutf::utf8_result res = implementation.validate_utf8_with_counts(
          (const char *)UTF8.data(), UTF8.size());
      size_t utf16_length =
          res.input_count - res.continuation_count + res.four_byte_count;
      ASSERT_EQUAL(res.input_count, res_ref.count);
      ASSERT_EQUAL(utf16_length, expected_utf16_length);
      ASSERT_EQUAL(res.error, res_ref.error);
      UTF8[corrupt] = restore;
    }
  }
}

TEST(corrupt_1byte) {
  uint32_t seed{1234};
  test_corrupt(implementation, seed,
               simdutf::tests::helpers::random_utf8(seed, 1, 0, 0, 0));
}

TEST(corrupt_2byte) {
  uint32_t seed{1234};
  test_corrupt(implementation, seed,
               simdutf::tests::helpers::random_utf8(seed, 0, 1, 0, 0));
  test_corrupt(implementation, seed,
               simdutf::tests::helpers::random_utf8(seed, 1, 1, 0, 0));
}

TEST(corrupt_3byte) {
  uint32_t seed{1234};
  test_corrupt(implementation, seed,
               simdutf::tests::helpers::random_utf8(seed, 0, 0, 1, 0));
  test_corrupt(implementation, seed,
               simdutf::tests::helpers::random_utf8(seed, 0, 1, 1, 0));
  test_corrupt(implementation, seed,
               simdutf::tests::helpers::random_utf8(seed, 1, 0, 1, 0));
  test_corrupt(implementation, seed,
               simdutf::tests::helpers::random_utf8(seed, 1, 1, 1, 0));
}

TEST(brute_force) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf8 gen_1_2_3_4(seed, 1, 1, 1, 1);
  size_t total = 1000;
  for (size_t i = 0; i < total; i++) {

    auto UTF8 = gen_1_2_3_4.generate(rand() % 256);
    for (size_t flip = 0; flip < 1000; ++flip) {
      // we are going to hack the string as long as it is UTF-8
      const int bitflip{1 << (rand() % 8)};
      UTF8[rand() % UTF8.size()] = uint8_t(bitflip); // we flip exactly one bit
      simdutf::result res_ref = implementation.validate_utf8_with_errors(
          (const char *)UTF8.data(), UTF8.size());
      size_t expected_utf16_length = implementation.utf16_length_from_utf8(
          (const char *)UTF8.data(), res_ref.count);
      simdutf::utf8_result res = implementation.validate_utf8_with_counts(
          (const char *)UTF8.data(), UTF8.size());
      size_t utf16_length =
          res.input_count - res.continuation_count + res.four_byte_count;
      ASSERT_EQUAL(res.input_count, res_ref.count);
      ASSERT_EQUAL(utf16_length, expected_utf16_length);
      ASSERT_EQUAL(res.error, res_ref.error);
    }
  }
}

TEST_MAIN
