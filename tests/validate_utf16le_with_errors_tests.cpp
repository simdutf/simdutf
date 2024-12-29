#include "simdutf.h"

#include <cstdio>

int main(int argc, char *argv[]) {
  auto utf16 = temporary::get_test_data();

  const char16_t valid_surrogate_W1 = 0xd800;
  const char16_t W2 = 0;
  for (size_t i = 0; i < utf16.size() - 1; i++) {
    const char16_t old_W1 = utf16[i + 0];
    const char16_t old_W2 = utf16[i + 1];

    utf16[i + 0] = valid_surrogate_W1;
    utf16[i + 1] = W2;

    auto res =
        temporary::validate_utf16le_with_errors(utf16.data(), utf16.size());

    if (res != i) {
      // for (auto u : utf16) {
      //   std::printf("%d, ", u);
      // }
      // std::printf("\n");

      std::printf("FAILED!!\n");
    }

    utf16[i + 0] = old_W1;
    utf16[i + 1] = old_W2;
  }
}
