#include "simdutf.h"

#include <array>
#include <iostream>

#include <tests/reference/validate_utf32.h>
#include <tests/reference/decode_utf32.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>


namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_latin1_to_utf16_test_base;

  constexpr int trials = 1000;
}

TEST(convert_all_latin) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 2 UTF-16 bytes
    simdutf::tests::helpers::RandomIntRanges random({
                                                     {0x00, 0xff}}, 0);

    auto procedure = [&implementation](const char* latin1, size_t size, char16_t* utf16le) -> size_t {
      std::vector<char16_t> utf16be(size);  
      size_t len = implementation.convert_latin1_to_utf16be(latin1, size, utf16be.data());
      implementation.change_endianness_utf16(utf16be.data(), size, utf16le);
      return len;
    };
    auto size_procedure = [&implementation](const char* latin1, size_t size) -> size_t {
      return implementation.utf16_length_from_latin1(latin1, size);
    };
    for (size_t size: input_size) {
      transcode_latin1_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
