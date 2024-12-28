#include "simdutf.h"

#include <array>
#include <cstdio>
#include <sstream>

#define ASSERT_EQUAL(a, b)                                                     \
  {                                                                            \
    const auto lhs = (a);                                                      \
    const auto rhs = (b);                                                      \
    if (lhs != rhs) {                                                          \
      std::stringstream lhs_str;                                               \
      lhs_str << lhs;                                                          \
      std::stringstream rhs_str;                                               \
      rhs_str << rhs;                                                          \
      printf("lhs: %s = %s\n", #a, lhs_str.str().c_str());                     \
      printf("rhs: %s = %s\n", #b, rhs_str.str().c_str());                     \
      printf("%s \n", #a);                                                     \
      printf("file %s:%d, function %s  \n", __FILE__, __LINE__, __func__);     \
      exit(1);                                                                 \
    }                                                                          \
  }

constexpr size_t trials = 1000;

int main(int argc, char *argv[]) {
  auto utf16 = temporary::get_test_data();

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
