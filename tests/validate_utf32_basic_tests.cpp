#include "simdutf.h"

#include <array>
#include <algorithm>

#include "helpers/random_utf32.h"
#include <tests/helpers/test.h>
#include <fstream>
#include <iostream>
#include <memory>

TEST(validate_utf32__returns_true_for_valid_input) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf32 generator{seed};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf32{generator.generate(1024, seed)};

    ASSERT_TRUE(implementation.validate_utf32(
              reinterpret_cast<const char32_t*>(utf32.data()), utf32.size()));
  }
}

TEST(validate_utf32__returns_true_for_empty_string) {
  const char32_t* buf = (char32_t*)"";

  ASSERT_TRUE(implementation.validate_utf32(buf, 0));
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
