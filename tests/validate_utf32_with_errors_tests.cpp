#include "simdutf.h"

#include <array>
#include <algorithm>

#include "helpers/random_utf32.h"
#include <tests/helpers/test.h>
#include <fstream>
#include <iostream>
#include <memory>

TEST(validate_utf32_with_errors__returns_success_for_valid_input) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf32 generator{seed};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf32{generator.generate(256, seed)};

    simdutf::result res = implementation.validate_utf32_with_errors(reinterpret_cast<const char32_t*>(utf32.data()), utf32.size());

    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(res.position, utf32.size());
  }
}

TEST(validate_utf32_with_errors__returns_success_for_empty_string) {
  const char32_t* buf = (char32_t*)"";

  simdutf::result res = implementation.validate_utf32_with_errors(buf, 0);

  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(res.position, 0);
}

TEST(validate_utf32_with_errors__returns_error_when_input_in_forbidden_range) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf32 generator{seed};
  for(size_t trial = 0; trial < 10; trial++) {
    auto utf32{generator.generate(128)};
    const char32_t*  buf = reinterpret_cast<const char32_t*>(utf32.data());
    const size_t len = utf32.size();

    for (char32_t wrong_value = 0xd800; wrong_value <= 0xdfff; wrong_value++) {
      for (size_t i=0; i < utf32.size(); i++) {
        const char32_t old = utf32[i];
        utf32[i] = wrong_value;

        simdutf::result res = implementation.validate_utf32_with_errors(buf, len);

        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.position, i);

        utf32[i] = old;
      }
    }
  }
}

TEST(validate_utf32_with_errors__returns_error_when_input_too_large) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf32 generator{seed};

  std::uniform_int_distribution<uint32_t> bad_range{0x110000, 0xffffffff};
  std::mt19937 gen{seed};

  for(size_t trial = 0; trial < 10; trial++) {
    auto utf32{generator.generate(128)};
    const char32_t*  buf = reinterpret_cast<const char32_t*>(utf32.data());
    const size_t len = utf32.size();

    for (size_t r = 0; r < 1000; r++) {
      uint32_t wrong_value = bad_range(gen);
      for (size_t i = 0; i < utf32.size(); i++) {
        const char32_t old = utf32[i];
        utf32[i] = wrong_value;

        simdutf::result res = implementation.validate_utf32_with_errors(buf, len);

        ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LARGE);
        ASSERT_EQUAL(res.position, i);

        utf32[i] = old;
      }
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
