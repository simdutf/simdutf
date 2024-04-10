#include "simdutf.h"

#include <array>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <memory>

namespace {
  using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;

  constexpr size_t trials = 10000;
}

TEST_LOOP(trials, convert_all_latin1) {
      size_t counter = 0;
      auto generator = [&counter]() -> uint8_t {
        return counter++ & 0xFF;
      };

      auto procedure = [&implementation](const char* latin1, size_t size, char* utf8) -> size_t {
        return implementation.convert_latin1_to_utf8(latin1, size, utf8);
      };
      auto size_procedure = [&implementation](const char* latin1, size_t size) -> size_t {
        return implementation.utf8_length_from_latin1(latin1, size); 
      };
        
      simdutf::tests::helpers::transcode_latin1_to_utf8_test_base test(generator, 256);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
}

TEST_MAIN
