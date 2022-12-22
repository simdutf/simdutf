#include "simdutf.h"
#include <cstddef>
#include <cstdint>
#include <random>
#include <iostream>
#include <iomanip>

#include <tests/helpers/random_utf8.h>
#include <tests/reference/validate_utf8.h>
#include <tests/helpers/test.h>

TEST(brute_force) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf8 gen_1_2_3_4(seed, 1, 1, 1, 1);
  size_t total = 1000;
  for (size_t i = 0; i < total; i++) {

    auto UTF8 = gen_1_2_3_4.generate(rand() % 256);
    if (!implementation.validate_utf8((const char *)UTF8.data(), UTF8.size())) {
      std::cerr << "bug" << std::endl;
      ASSERT_TRUE(false);
    }
    for (size_t flip = 0; flip < 1000; ++flip) {
      // we are going to hack the string as long as it is UTF-8
      const int bitflip{1 << (rand() % 8)};
      UTF8[rand() % UTF8.size()] = uint8_t(bitflip); // we flip exactly one bit
      bool is_ok =
          implementation.validate_utf8((const char *)UTF8.data(), UTF8.size());
      bool is_ok_basic =
          simdutf::tests::reference::validate_utf8((const char *)UTF8.data(), UTF8.size());
      if (is_ok != is_ok_basic) {
        std::cerr << "bug" << std::endl;
        ASSERT_TRUE(false);
      }
    }
  }
  puts("OK");
}


int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}