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

  using simdutf::tests::helpers::transcode_utf32_to_utf16_test_base;

  constexpr int trials = 1000;
}

TEST(convert_into_2_UTF16_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 2 UTF-16 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd7ff},
                                                     {0xe000, 0xffff}}, 0);

    auto procedure = [&implementation](const char32_t* utf32, size_t size, char16_t* utf16le) -> size_t {
      std::vector<char16_t> utf16be(size);
      size_t len = implementation.convert_utf32_to_utf16be(utf32, size, utf16be.data());
      implementation.change_endianness_utf16(utf16be.data(), len, utf16le);
      return len;
    };
    for (size_t size: input_size) {
      transcode_utf32_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_into_4_UTF16_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 4 UTF-16 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x10000, 0x10ffff}}, 0);

    auto procedure = [&implementation](const char32_t* utf32, size_t size, char16_t* utf16le) -> size_t {
      std::vector<char16_t> utf16be(2*size);
      size_t len = implementation.convert_utf32_to_utf16be(utf32, size, utf16be.data());
      implementation.change_endianness_utf16(utf16be.data(), len, utf16le);
      return len;
    };
    for (size_t size: input_size) {
      transcode_utf32_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_into_2_or_4_UTF16_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 2 or 4 UTF-16 bytes (all codepoints)
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd7ff},
                                                     {0xe000, 0xffff},
                                                     {0x10000, 0x10ffff}}, 0);

    auto procedure = [&implementation](const char32_t* utf32, size_t size, char16_t* utf16le) -> size_t {
      std::vector<char16_t> utf16be(2*size);
      size_t len = implementation.convert_utf32_to_utf16be(utf32, size, utf16be.data());
      implementation.change_endianness_utf16(utf16be.data(), len, utf16le);
      return len;
    };
    for (size_t size: input_size) {
      transcode_utf32_to_utf16_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
