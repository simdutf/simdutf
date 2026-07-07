#include "simdutf.h"
#include "simdutf/error.h"

#include <tests/helpers/random_utf8.h>
#include <tests/reference/validate_utf8.h>
#include <tests/helpers/test.h>

void printByteInBinary(const unsigned char &byte) {
  for (int i = 7; i >= 0; --i) {
    putchar(((byte >> i) & 1) ? '1' : '0');
  }
}

void printDataInBinary(const unsigned char *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printByteInBinary(buf[i]);
    putchar(' ');
  }
}

void debugPrintRes(const simdutf::utf8_result &res) {
  printf("utf8 result:\n");
  printf("    length: %zu\n", res.input_count);
  printf("    continuations: %zu\n", res.continuation_count);
  printf("    non_ascii: %zu\n", res.non_ascii_count);
  printf("    four_byte: %zu\n", res.four_byte_count);
  printf("    error: %u\n", res.error);
}

void debugCompareResults(const simdutf::utf8_result &res,
                         const simdutf::utf8_result &res_scalar,
                         const unsigned char *buf, size_t len) {
  if (res.input_count != res_scalar.input_count ||
      res.non_ascii_count != res_scalar.non_ascii_count ||
      res.continuation_count != res_scalar.continuation_count ||
      res.four_byte_count != res_scalar.four_byte_count ||
      res.error != res_scalar.error) {
    printf("produced ");
    debugPrintRes(res);
    printf("\nscalar ");
    debugPrintRes(res_scalar);
    printf("\nDifference on input:\n");
    printDataInBinary(buf, res_scalar.input_count);
    printf("\nNext few bytes:\n");
    size_t forward = 5;
    size_t remaining_size = len - res_scalar.input_count;
    if (forward > remaining_size) {
      forward = remaining_size;
    }
    printDataInBinary(buf + res_scalar.input_count, forward);
    printf("\n");
  }
  ASSERT_EQUAL(res.input_count, res_scalar.input_count);
  ASSERT_EQUAL(res.non_ascii_count, res_scalar.non_ascii_count);
  ASSERT_EQUAL(res.continuation_count, res_scalar.continuation_count);
  ASSERT_EQUAL(res.four_byte_count, res_scalar.four_byte_count);
  ASSERT_EQUAL(res.error, res_scalar.error);
}

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

      simdutf::utf8_result res = implementation.validate_utf8_with_counts(
          (const char *)UTF8.data(), UTF8.size());

      /*
       Plain comparison with scalar implementation for debugging:
       */
      simdutf::utf8_result res_scalar =
          simdutf::scalar::utf8::validate_utf8_with_counts(
              (const char *)UTF8.data(), UTF8.size());
      debugCompareResults(res, res_scalar, UTF8.data(), UTF8.size());

      simdutf::result res_ref = implementation.validate_utf8_with_errors(
          (const char *)UTF8.data(), UTF8.size());
      size_t expected_utf16_length = implementation.utf16_length_from_utf8(
          (const char *)UTF8.data(), res_ref.count);
      ASSERT_EQUAL(res.input_count, res_ref.count);
      ASSERT_EQUAL(res.error, res_ref.error);
      ASSERT_EQUAL(res.utf16_length(), expected_utf16_length);
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
      ASSERT_EQUAL(res.input_count, res_ref.count);
      ASSERT_EQUAL(res.utf16_length(), expected_utf16_length);
      ASSERT_EQUAL(res.error, res_ref.error);
    }
  }
}

TEST_MAIN
