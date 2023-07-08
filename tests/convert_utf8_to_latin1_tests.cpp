#include "simdutf.h"

#include <array>
#include <iostream>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <sstream>

#include "reference/validate_utf8_to_latin1.h"


namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_utf8_to_latin1_test_base;

  constexpr size_t trials = 10000;
}

// For invalid UTF-8, we expect the conversion to fail (return 0)
TEST(convert_random_inputs) {
  simdutf::tests::helpers::RandomInt r(0x00, 0xff, 1234);
  for(size_t trial = 0; trial < trials; trial ++) {
    uint32_t seed{1234+uint32_t(trial)};
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }

    for (size_t size: input_size) {
      std::vector<char> utf8(size);
      for(size_t i = 0; i < size; i++) {
        utf8[i] = r();
      }
      size_t buffer_size = implementation.latin1_length_from_utf8(utf8.data(), size);
      std::vector<char> latin1(buffer_size);
      size_t actual_size = implementation.convert_utf8_to_latin1(utf8.data(), size, latin1.data());
      if(simdutf::tests::reference::validate_utf8_to_latin1(utf8.data(), size)) {
          ASSERT_EQUAL(actual_size,buffer_size) ;
      } else {
        ASSERT_EQUAL(actual_size,0);
      }
    }
  }
}

TEST(convert_pure_ASCII) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    size_t counter = 0;
    auto generator = [&counter]() -> uint8_t {
      return counter++ & 0x7f;
    };

    auto procedure = [&implementation](const char* utf8, size_t size, char* latin1) -> size_t {
      return implementation.convert_valid_utf8_to_latin1(utf8, size, latin1);
    };
    auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
      return implementation.latin1_length_from_utf8(utf8, size);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_latin1_test_base test(generator, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
  }
} 

TEST(convert_1_or_2_valid_UTF8_bytes_to_latin1) {
  for(size_t trial = 0; trial < trials; trial ++) {
    uint32_t seed{1234+uint32_t(trial)};
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    simdutf::tests::helpers::RandomInt random(0x0000, 0x0ff, seed); // range for 1 or 2 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char* latin1) -> size_t {
      return implementation.convert_valid_utf8_to_latin1(utf8, size, latin1);
    };
    auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
      return implementation.latin1_length_from_utf8(utf8, size);
    };
    for (size_t size: input_size) {
      transcode_utf8_to_latin1_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
