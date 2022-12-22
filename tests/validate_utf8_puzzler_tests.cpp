#include "simdutf.h"
#include <cstddef>
#include <cstdint>
#include <random>
#include <iostream>
#include <iomanip>
#include <tests/helpers/test.h>

// This is an attempt at reproducing an issue with the utf8 fuzzer
TEST(puzzler) {
  const char* bad64 = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
  size_t length = 64;
  ASSERT_FALSE(implementation.validate_utf8(bad64, length));
}


int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}