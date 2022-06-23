#include "simdutf.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include <tests/helpers/random_int.h>
#include <tests/helpers/random_utf8.h>
#include <tests/helpers/test.h>

namespace {
std::array<size_t, 7> input_size{8, 16, 12, 64, 68, 128, 256};
} // namespace

TEST(pure_utf8_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 1, 0, 0, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;    // 3
      auto actual = implementation.op_autodetect_encodings(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
