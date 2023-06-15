#include "simdutf.h"

#include <array>
#include <iostream>

#include <tests/reference/validate_utf16.h>
#include <tests/reference/decode_utf16.h>
#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>

namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_utf16_to_latin1_test_base;

  constexpr int trials = 1000;
}

TEST(convert_2_UTF16_bytes) {
  int seed = {1234};
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 1, 2 or 3 UTF-8 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0x00ff},
                                                     }, seed);

    auto procedure = [&implementation](const char16_t* utf16le, size_t size, char* latin1) -> size_t {
      std::vector<char16_t> utf16be(size);
      implementation.change_endianness_utf16(utf16le, size, utf16be.data());
      return implementation.convert_utf16be_to_latin1(utf16be.data(), size, latin1);
    };
    auto size_procedure = [&implementation](const char16_t* utf16, size_t size) -> size_t {
      return implementation.latin1_length_from_utf16(size);
    };
    for (size_t size: input_size) {
      transcode_utf16_to_latin1_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
  }
}

TEST(convert_fails_if_input_too_large) {
  uint32_t seed{1234};
  simdutf::tests::helpers::RandomInt generator(0xff, 0xffff, seed); 
  //starts with Little endian: LSB goes first e.g. 0xabcd. By default, C++ represents all hex values as LE

  auto procedure = [&implementation](const char16_t* utf16le, size_t size, char* latin1) -> size_t {
      std::vector<char16_t> utf16be(size);
      // std::cout << "Before change_endianness_utf16: " << std::hex << *utf16le << std::endl;

      implementation.change_endianness_utf16(utf16le, size, utf16be.data());
      
      //we switch here to Big endian: MSB goes first e.g.cdab
      // std::cout << "After change_endianness_utf16: " << std::hex << utf16be[0] << std::endl;
      // std::cout << "Before convert_utf16be_to_latin1: " << std::hex << utf16be[0] << std::endl;
      auto result = implementation.convert_utf16be_to_latin1(utf16be.data(), size, latin1);
      // std::cout << "After convert_utf16be_to_latin1: " << std::hex << (int)latin1[0] << std::endl;
      return result;
  };
  const size_t size = 64;
  transcode_utf16_to_latin1_test_base test([](){ return '*'; }, size+32);

  for (size_t j = 0; j < 1000; j++) {
    uint16_t wrong_value = generator();
    // std::cout << "wrong_value: " << wrong_value << std::endl;
    for (size_t i=0; i < size; i++) {
      auto old = test.input_utf16[i];
      // std::cout << "Before: " << std::hex << test.input_utf16[i] << std::endl;
      test.input_utf16[i] = wrong_value;
      ASSERT_TRUE(test(procedure));
      // std::cout << "After: " << std::hex << test.input_utf16[i] << std::endl;
      test.input_utf16[i] = old;
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
