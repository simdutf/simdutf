#include "simdutf.h"

#define SIMDUTF_IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

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
  // auto utf16{generator.generate(128)};
  std::vector<char16_t> utf16{
      34086, 23924, 65167, 9449,  32427, 65404, 14192, 60752, 57590, 63026,
      21580, 61350, 10983, 62185, 57347, 60300, 4467,  62929, 8682,  23452,
      36980, 28869, 3039,  58105, 58452, 61481, 47340, 62550, 60371, 5138,
      18914, 64294, 59075, 18991, 59766, 51746, 62468, 10585, 47438, 23289,
      26964, 60025, 45402, 46457, 14455, 43444, 4468,  61865, 62211, 20469,
      64275, 64580, 59200, 65492, 58097, 62185, 62338, 21634, 46275, 63289,
      65112, 60074, 8557,  64274, 60533, 65032, 4111,  36288, 26464, 40249,
      61369, 45399, 32932, 64028, 64190, 23838, 1945,  58776, 18272, 64343,
      63253, 18963, 62372, 32657, 61650, 58466, 8742,  21137, 37523, 54819,
      43802, 61335, 58945, 25550, 19772, 36049, 59839, 63373, 32361, 40867,
      31315, 62197, 32946, 59498, 60676, 59519, 28254, 62015, 58067, 62280,
      52750, 21820, 63529, 38616, 13940, 60929, 59059, 60084, 31374, 64431,
      59037, 27232, 64273, 11103, 65006, 46645, 65432, 5559};
  for (auto u : utf16) {
    std::printf("%d, ", u);
  }
  std::printf("\n");

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

      temporary::result res = temporary::validate_utf16le_with_errors(
          reinterpret_cast<const char16_t *>(buf), len);

      ASSERT_EQUAL(res.error, temporary::error_code::SURROGATE);
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
