#include "simdutf.h"

#ifndef SIMDUTF_IS_BIG_ENDIAN
  #error "SIMDUTF_IS_BIG_ENDIAN should be defined."
#endif

#include <array>
#include <fstream>
#include <memory>

#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>

constexpr size_t trials = 1000;

int main(int argc, char *argv[]) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  auto utf16{generator.generate(128)};
  const char16_t *buf = reinterpret_cast<const char16_t *>(utf16.data());
  const size_t len = utf16.size();

  const std::array<char16_t, 5> sample_wrong_second_word{0x0000, 0x1000, 0xdbff,
                                                         0xe000, 0xffff};

  const char16_t valid_surrogate_W1 = 0xd800;
  for (char16_t W2 : sample_wrong_second_word) {
    for (size_t i = 0; i < utf16.size() - 1; i++) {
      const char16_t old_W1 = utf16[i + 0];
      const char16_t old_W2 = utf16[i + 1];

      utf16[i + 0] = valid_surrogate_W1;
      utf16[i + 1] = W2;

      simdutf::result res = temporary::validate_utf16le_with_errors(
          reinterpret_cast<const char16_t *>(buf), len);

      ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
      if (res.count != i) {
        // for (int j = 0; j < static_cast<int>(utf16.size()); ++j) {
        //   std::printf("oops! utf16[%d]=%d\n", j, +utf16.at(j));
        // }
      }
      ASSERT_EQUAL(res.count, i);

      utf16[i + 0] = old_W1;
      utf16[i + 1] = old_W2;
    }
  }
}
