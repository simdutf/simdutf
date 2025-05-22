#include "simdutf.h"

#include <algorithm>
#include <random>
#include <vector>
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

TEST_MAIN
