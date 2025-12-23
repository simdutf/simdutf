#include "simdutf.h"

#include <span>

#include <tests/helpers/test.h>

#if __cpp_pp_embed

TEST(test_embed) {
  // clang-format off
  constexpr unsigned char binary[]{
  #embed <embed/valid_utf8.txt>
  };
  // clang-format on
  static_assert(simdutf::validate_utf8(std::span(binary)));
}

#else
TEST(embed_is_not_available_in_older_cpp_versions) {}
#endif

TEST_MAIN
