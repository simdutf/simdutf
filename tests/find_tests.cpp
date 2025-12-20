#include "simdutf.h"

#include <algorithm>
#include <random>
#include <vector>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/test.h>

const uint64_t seed = 0x123456789ABCDEF0;

template <typename char_type, typename impl>
void random_char_search(impl &implementation) {
  // Random number generator
  std::random_device rd;
  std::mt19937 gen(rd());

  // Generate random size between 0 and 1024
  std::uniform_int_distribution<size_t> size_dist(0, 1024);
  size_t size = size_dist(gen);

  // Create vector of random characters
  std::vector<char_type> arr(size);
  std::uniform_int_distribution<int> char_dist(32, 126);

  for (size_t i = 0; i < size; ++i) {
    arr[i] = static_cast<char_type>(char_dist(gen));
  }

  // Pick a random character to search for
  char_type search_char = static_cast<char_type>(char_dist(gen));

  // Use std::find to search for the character
  auto result = std::find(arr.data(), arr.data() + size, search_char);

  // Nest use simdutf::find to search for the character
  auto simd_result =
      implementation.find(arr.data(), arr.data() + size, search_char);
  // Check if the results are the same
  ASSERT_TRUE(simd_result == result);
  simd_result = simdutf::find(arr.data(), arr.data() + size, search_char);
  // Check if the results are the same
  ASSERT_TRUE(simd_result == result);
}

TEST(random_char_search_char) {
  for (size_t i = 0; i < 1000; ++i) {
    random_char_search<char>(implementation);
  }
}
TEST(random_char_search_char16_t) {
  for (size_t i = 0; i < 1000; ++i) {
    random_char_search<char16_t>(implementation);
  }
}

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_find_char) {
  using namespace simdutf::tests::helpers;
  constexpr auto s = "ensure find() is constexpr"_latin1;
  constexpr auto loc = std::distance(
      s.data(), simdutf::find(s.data(), s.data() + s.size(), 'c'));
  static_assert(loc == 17);
}

TEST(compile_time_find_utf16) {
  using namespace simdutf::tests::helpers;
  constexpr auto s = u"ensure find() is constexpr"_utf16;
  constexpr auto loc = std::distance(
      s.data(), simdutf::find(s.data(), s.data() + s.size(), 'c'));
  static_assert(loc == 17);
}

#endif

TEST_MAIN
