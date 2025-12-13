#include "simdutf.h"

#include <array>

#include <tests/helpers/random_utf8.h>
#include <tests/helpers/test.h>

TEST_LOOP(no_error_ASCII) {
  simdutf::tests::helpers::random_utf8 generator{seed, 1, 0, 0, 0};
  const auto ascii{generator.generate(512)};

  simdutf::result res = implementation.validate_ascii_with_errors(
      reinterpret_cast<const char *>(ascii.data()), ascii.size());

  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(res.count, ascii.size());
}

TEST_LOOP(error_ASCII) {
  simdutf::tests::helpers::random_utf8 generator{seed, 1, 0, 0, 0};

  auto ascii{generator.generate(512)};

  for (unsigned int i = 0; i < ascii.size(); i++) {
    ascii[i] += 0b10000000;

    simdutf::result res = implementation.validate_ascii_with_errors(
        reinterpret_cast<const char *>(ascii.data()), ascii.size());

    ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LARGE);
    ASSERT_EQUAL(res.count, i);

    ascii[i] -= 0b10000000;
  }
}

TEST_MAIN
