#include "simdutf.h"
#include <cstddef>
#include <cstdint>
#include <random>
#include <iostream>
#include <iomanip>

#include "helpers/random_utf8.h"
#include "reference/validate_utf8.h"

int main() {
  printf("running brute-force UTF-8 tests... ");
  fflush(NULL);
  std::random_device rd{};
  simdutf::tests::helpers::RandomUTF8 gen_1_2_3_4(rd, 1, 1, 1, 1);
  size_t total = 1000;
  for (size_t i = 0; i < total; i++) {

    auto UTF8 = gen_1_2_3_4.generate(rand() % 256);
    if (!simdutf::validate_utf8((const char *)UTF8.data(), UTF8.size())) {
      std::cerr << "bug" << std::endl;
      abort();
    }
    for (size_t flip = 0; flip < 1000; ++flip) {
      // we are going to hack the string as long as it is UTF-8
      const int bitflip{1 << (rand() % 8)};
      UTF8[rand() % UTF8.size()] = uint8_t(bitflip); // we flip exactly one bit
      bool is_ok =
          simdutf::validate_utf8((const char *)UTF8.data(), UTF8.size());
      bool is_ok_basic =
          simdutf::tests::reference::validate_utf8((const char *)UTF8.data(), UTF8.size());
      if (is_ok != is_ok_basic) {
        std::cerr << "bug" << std::endl;
        abort();
      }
    }
  }
  puts("OK");

  return EXIT_SUCCESS;
}
