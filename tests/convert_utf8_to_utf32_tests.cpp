#include "simdutf.h"

#include <array>
#include <iostream>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>


namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_utf8_to_utf32_test_base;

  constexpr size_t trials = 10000;
}

TEST(convert_pure_ASCII) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
      return counter++ & 0x7f;
    };

    auto procedure = [&implementation](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return implementation.convert_utf8_to_utf32(utf8, size, utf32);
    };
    //auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
    //  return implementation.utf16_length_from_utf8(utf8, size);
    //};

    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test(generator, size);
      ASSERT_TRUE(test(procedure));    
      //ASSERT_TRUE(test.check_size(size_procedure));    
    }
  }
}

TEST(convert_1_or_2_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    uint32_t seed{1234+uint32_t(trial)};
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    simdutf::tests::helpers::RandomInt random(0x0000, 0x07ff, seed); // range for 1 or 2 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return implementation.convert_utf8_to_utf32(utf8, size, utf32);
    };
    //auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
    //  return implementation.utf16_length_from_utf8(utf8, size);
    //};
    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      //ASSERT_TRUE(test.check_size(size_procedure));    
    }
  }
}

TEST(convert_1_or_2_or_3_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    uint32_t seed{1234+uint32_t(trial)};
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 1, 2 or 3 UTF-8 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd7ff},
                                                     {0xe000, 0xffff}}, seed);

    auto procedure = [&implementation](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return implementation.convert_utf8_to_utf32(utf8, size, utf32);
    };
    //auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
    //  return implementation.utf16_length_from_utf8(utf8, size);
    //};
    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    //  ASSERT_TRUE(test.check_size(size_procedure));    
    }
  }
}

TEST(convert_3_or_4_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    uint32_t seed{1234+uint32_t(trial)};
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    simdutf::tests::helpers::RandomIntRanges random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed); // range for 3 or 4 UTF-8 bytes

    auto procedure = [&implementation](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return implementation.convert_utf8_to_utf32(utf8, size, utf32);
    };
    //auto size_procedure = [&implementation](const char* utf8, size_t size) -> size_t {
    //  return implementation.utf16_length_from_utf8(utf8, size);
    //};
    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      //ASSERT_TRUE(test.check_size(size_procedure));    
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
