#include "simdutf.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#ifdef __linux__
  #include <sys/mman.h>
  #include <unistd.h>
#endif

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

// Helper: place a buffer at a specific alignment modulo 64.
static std::pair<std::vector<char>, char *> make_aligned_buf(size_t size,
                                                             size_t align_mod) {
  std::vector<char> backing(size + 128, '\0');
  uintptr_t base = reinterpret_cast<uintptr_t>(backing.data());
  size_t current_mod = base % 64;
  size_t offset = (align_mod >= current_mod) ? (align_mod - current_mod)
                                             : (64 - current_mod + align_mod);
  char *ptr = backing.data() + offset;
  return {std::move(backing), ptr};
}

TEST(find_char_null_needle_all_alignments) {
  const char needle = '\0';
  const uint8_t payload[] = {0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26,
                             0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x01, 0x00};
  const size_t len = sizeof(payload);

  const char *expected =
      std::find(reinterpret_cast<const char *>(payload),
                reinterpret_cast<const char *>(payload) + len, needle);
  size_t expected_offset =
      static_cast<size_t>(expected - reinterpret_cast<const char *>(payload));

  for (size_t align = 0; align < 64; ++align) {
    auto aligned_buf = make_aligned_buf(len, align);
    std::vector<char> backing = std::move(aligned_buf.first);
    char *buf = aligned_buf.second;
    std::memcpy(buf, payload, len);

    const char *result = implementation.find(buf, buf + len, needle);
    size_t got = static_cast<size_t>(result - buf);
    ASSERT_EQUAL(got, expected_offset);
  }
}

TEST(find_char_null_needle_various_sizes) {
  const char needle = '\0';
  for (size_t len = 1; len <= 256; ++len) {
    for (size_t align = 0; align < 64; align += 7) {
      auto aligned_buf = make_aligned_buf(len, align);
      std::vector<char> backing = std::move(aligned_buf.first);
      char *buf = aligned_buf.second;
      std::memset(buf, 'A', len);
      buf[len - 1] = '\0';

      const char *result = implementation.find(buf, buf + len, needle);
      size_t got = static_cast<size_t>(result - buf);
      ASSERT_EQUAL(got, len - 1);
    }
  }
}

#ifdef __linux__
static char *alloc_at_page_end(size_t size) {
  const size_t page = static_cast<size_t>(sysconf(_SC_PAGESIZE));
  const size_t pages_needed = (size + page - 1) / page + 1;
  const size_t total = pages_needed * page;
  char *base = static_cast<char *>(mmap(nullptr, total, PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  if (base == MAP_FAILED) {
    return nullptr;
  }
  mmap(base + total - page, page, PROT_NONE,
       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  return base + total - page - size;
}

static void free_at_page_end(char *buf, size_t size) {
  const size_t page = static_cast<size_t>(sysconf(_SC_PAGESIZE));
  const size_t pages_needed = (size + page - 1) / page + 1;
  const size_t total = pages_needed * page;
  char *base = buf + size + page - total;
  munmap(base, total);
}

TEST(find_char_guard_page_various_sizes) {
  for (size_t len = 1; len <= 256; ++len) {
    char *buf = alloc_at_page_end(len);
    ASSERT_TRUE(buf != nullptr);
    std::memset(buf, 'A', len);
    buf[len - 1] = 'Z';

    const char *expected = buf + len - 1;
    const char *result = implementation.find(buf, buf + len, 'Z');
    ASSERT_TRUE(result == expected);

    free_at_page_end(buf, len);
  }
}

TEST(find_char_guard_page_needle_absent) {
  for (size_t len = 1; len <= 256; ++len) {
    char *buf = alloc_at_page_end(len);
    ASSERT_TRUE(buf != nullptr);
    std::memset(buf, 'X', len);

    const char *result = implementation.find(buf, buf + len, 'Y');
    ASSERT_TRUE(result == buf + len);

    free_at_page_end(buf, len);
  }
}
#endif

TEST(find_char_all_same_character) {
  for (size_t len = 1; len <= 128; ++len) {
    const std::vector<char> input(len, '&');
    const char *start = input.data();
    const char *end = start + input.size();

    const char *result = implementation.find(start, end, '&');
    ASSERT_TRUE(result == start);
  }
}

TEST(find_char_needle_at_end) {
  for (size_t len = 1; len <= 128; ++len) {
    std::vector<char> input(len, 'A');
    input.back() = 'Z';
    const char *start = input.data();
    const char *end = start + input.size();

    const char *expected = end - 1;
    const char *result = implementation.find(start, end, 'Z');
    ASSERT_TRUE(result == expected);
  }
}

TEST(find_char_needle_absent) {
  for (size_t len = 0; len <= 128; ++len) {
    const std::vector<char> input(len, 'X');
    const char *start = input.data();
    const char *end = start + input.size();

    const char *result = implementation.find(start, end, 'Y');
    ASSERT_TRUE(result == end);
  }
}

TEST_MAIN
