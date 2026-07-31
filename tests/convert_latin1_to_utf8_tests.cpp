#include "simdutf.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <tests/helpers/compiletime_conversions.h>
#include <tests/helpers/fixed_string.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

namespace {
using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;

} // namespace

TEST_LOOP(convert_all_latin1) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint8_t { return counter++ & 0xFF; };

  auto procedure = [&implementation](const char *latin1, size_t size,
                                     char *utf8) -> size_t {
    return implementation.convert_latin1_to_utf8(latin1, size, utf8);
  };
  auto size_procedure = [&implementation](const char *latin1,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_latin1(latin1, size);
  };

  simdutf::tests::helpers::transcode_latin1_to_utf8_test_base test(generator,
                                                                   256);
  ASSERT_TRUE(test(procedure));
  ASSERT_TRUE(test.check_size(size_procedure));
}

TEST(convert_all_latin1_safe) {
  std::vector<char> latin1(1024);
  for (size_t i = 0; i < latin1.size(); i++) {
    latin1[i] = i & 0xff;
  }
  size_t utf8_length =
      implementation.utf8_length_from_latin1(latin1.data(), latin1.size());
  std::vector<char> utf8(utf8_length);
  const auto result = implementation.convert_latin1_to_utf8(
      latin1.data(), latin1.size(), utf8.data());
  ASSERT_EQUAL(result, utf8_length);
  for (size_t output_size = 0; output_size < utf8.size(); output_size++) {
    std::vector<char> utf8_buffer(output_size);
    size_t used_size = simdutf::convert_latin1_to_utf8_safe(
        latin1.data(), latin1.size(), utf8_buffer.data(), output_size);
    for (size_t i = 0; i < used_size; i++) {
      ASSERT_EQUAL(utf8_buffer[i], utf8[i]);
    }
    if (used_size < output_size) {
      ASSERT_EQUAL(used_size, output_size - 1);
      ASSERT_TRUE(uint8_t(utf8[used_size]) >= 0x80);
    }
  }
}

#if SIMDUTF_STRING_RESIZE_AND_OVERWRITE

namespace {
template <typename T> class limited_allocator {
public:
  using value_type = T;

  explicit limited_allocator(std::size_t maximum_size,
                             std::size_t *allocation_count = nullptr) noexcept
      : maximum_size_(maximum_size), allocation_count_(allocation_count) {}

  template <typename U>
  limited_allocator(const limited_allocator<U> &other) noexcept
      : maximum_size_(other.maximum_size_),
        allocation_count_(other.allocation_count_) {}

  T *allocate(std::size_t count) {
    if (count > maximum_size_) {
      throw std::bad_alloc();
    }
    if (allocation_count_ != nullptr) {
      ++*allocation_count_;
    }
    return std::allocator<T>{}.allocate(count);
  }

  void deallocate(T *pointer, std::size_t count) noexcept {
    std::allocator<T>{}.deallocate(pointer, count);
  }

  std::size_t max_size() const noexcept { return maximum_size_; }

  template <typename U>
  bool operator==(const limited_allocator<U> &other) const noexcept {
    return maximum_size_ == other.maximum_size_;
  }

  template <typename U>
  bool operator!=(const limited_allocator<U> &other) const noexcept {
    return !(*this == other);
  }

  template <typename> friend class limited_allocator;

private:
  std::size_t maximum_size_;
  std::size_t *allocation_count_;
};
} // namespace

TEST(convert_latin1_to_utf8_string) {
  const char latin1[] = {'A', static_cast<char>(0x80),
                         static_cast<char>(0xe9), static_cast<char>(0xff), 'z'};
  const char expected[] = {'A', static_cast<char>(0xc2),
                           static_cast<char>(0x80), static_cast<char>(0xc3),
                           static_cast<char>(0xa9), static_cast<char>(0xc3),
                           static_cast<char>(0xbf), 'z'};

  std::string output("previous output");
  const auto written =
      simdutf::convert_latin1_to_utf8(latin1, sizeof(latin1), output);
  ASSERT_EQUAL(written, sizeof(expected));
  ASSERT_EQUAL(output.size(), sizeof(expected));
  ASSERT_TRUE(std::equal(output.begin(), output.end(), std::begin(expected)));

  const auto written_again =
      simdutf::convert_latin1_to_utf8(latin1, sizeof(latin1), output);
  ASSERT_EQUAL(written_again, sizeof(expected));
  ASSERT_EQUAL(output.size(), sizeof(expected));
  ASSERT_TRUE(std::equal(output.begin(), output.end(), std::begin(expected)));
}

TEST(convert_latin1_to_utf8_string_reuses_capacity) {
  using limited_string =
      std::basic_string<char, std::char_traits<char>, limited_allocator<char>>;

  std::size_t allocation_count = 0;
  limited_allocator<char> allocator(4096, &allocation_count);
  limited_string output(allocator);
  const std::string latin1(512, static_cast<char>(0xe9));

  ASSERT_EQUAL(simdutf::convert_latin1_to_utf8(latin1.data(), latin1.size(),
                                                output),
               latin1.size() * 2);
  const auto allocations_after_first_conversion = allocation_count;
  ASSERT_TRUE(allocations_after_first_conversion > 0);

  ASSERT_EQUAL(simdutf::convert_latin1_to_utf8(latin1.data(), latin1.size(),
                                                output),
               latin1.size() * 2);
  ASSERT_EQUAL(allocation_count, allocations_after_first_conversion);
}

TEST(convert_latin1_to_utf8_string_empty) {
  std::string output("previous output");
  const auto written = simdutf::convert_latin1_to_utf8("", 0, output);
  ASSERT_EQUAL(written, 0);
  ASSERT_TRUE(output.empty());
}

TEST(convert_latin1_to_utf8_string_respects_max_size) {
  using limited_string =
      std::basic_string<char, std::char_traits<char>, limited_allocator<char>>;

  limited_allocator<char> allocator(128);
  limited_string output("unchanged", allocator);
  const std::string previous(output.data(), output.size());
  const std::size_t input_length = output.max_size() / 2 + 1;
  const std::string latin1(input_length, 'x');

  ASSERT_TRUE(input_length > output.max_size() / 2);
  const auto written = simdutf::convert_latin1_to_utf8(
      latin1.data(), latin1.size(), output);
  ASSERT_EQUAL(written, 0);
  ASSERT_EQUAL(output.size(), previous.size());
  ASSERT_TRUE(std::equal(output.begin(), output.end(), previous.begin()));
}

#endif // SIMDUTF_STRING_RESIZE_AND_OVERWRITE

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_utf8_length_from_latin1) {
  using namespace simdutf::tests::helpers;
  static_assert(simdutf::utf8_length_from_latin1("x"_latin1) == 1);
  // swedish character "ö":
  static_assert(simdutf::utf8_length_from_latin1("\xF6"_latin1) == 2);
}

TEST(compile_time_convert_latin1_to_utf8) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = "I am a nice and wellbehaved string"_latin1;
  constexpr auto expected = u8"I am a nice and wellbehaved string"_utf8;
  static_assert(simdutf::utf8_length_from_latin1(input) == expected.size());
  constexpr auto converted = to_utf8<input>();
  static_assert(converted == expected);
}

TEST(compile_time_convert_latin1_to_utf8_harder) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = "k\xF6ttbulle"_latin1;
  constexpr auto expected = u8"köttbulle"_utf8;
  static_assert(simdutf::utf8_length_from_latin1(input) == expected.size());
  constexpr auto converted = to_utf8<input>();
  static_assert(converted == expected);
}

namespace {
template <auto input, std::size_t N> constexpr auto convert_safe() {
  simdutf::tests::helpers::CTString<char8_t, N> ret{};
  auto written = simdutf::convert_latin1_to_utf8_safe(input, ret);
  return std::tuple(written, ret);
}
} // namespace

TEST(compile_time_convert_latin1_to_utf8_safe) {
  using namespace simdutf::tests::helpers;

  constexpr auto input = "k\xF6ttbulle"_latin1;
  constexpr auto expected = u8"köttbulle"_utf8;

  // convert using a too small buffer
  {
    constexpr auto small = convert_safe<input, 2>();
    constexpr auto written = std::get<0>(small);
    static_assert(written == 1);
  }

  // use a large enough buffer
  {
    constexpr auto large = convert_safe<input, 100>();
    constexpr auto written = std::get<0>(large);
    static_assert(written == expected.size());
    constexpr auto output = std::get<1>(large).shrink<written>();
    static_assert(output == expected);
  }
}
#endif

TEST_MAIN
