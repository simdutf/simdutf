#include "simdutf.h"

#include <memory>
#include <utility>
#include <vector>

#include <tests/helpers/test.h>

/// a span-like class which returns int as size (not std::size_t)
struct CustomSpan1 {
  int size() const noexcept { return 3; }
  const char *data() const noexcept { return "hej"; }
};

#if SIMDUTF_CPLUSPLUS20
  #include <span>
TEST(autodect_can_use_containers_and_views) {
  std::vector<char> data{1, 2, 3, 4, 5};
  auto r1a = simdutf::autodetect_encoding(data);
  auto r1b = simdutf::autodetect_encoding(std::span{data});
  auto r1c = simdutf::autodetect_encoding(std::span{std::as_const(data)});
  auto r1d = simdutf::autodetect_encoding(std::as_const(data));
  auto r1e = simdutf::autodetect_encoding(std::move(data));

  std::vector<unsigned char> udata{1, 2, 3, 4, 5};
  auto r2a = simdutf::autodetect_encoding(udata);
  auto r2b = simdutf::autodetect_encoding(std::span{udata});
  auto r2c = simdutf::autodetect_encoding(std::span{std::as_const(udata)});
  auto r2d = simdutf::autodetect_encoding(std::as_const(udata));

  std::vector<signed char> sdata{1, 2, 3, 4, 5};
  auto r3a = simdutf::autodetect_encoding(udata);
  auto r3b = simdutf::autodetect_encoding(std::span{udata});
  auto r3c = simdutf::autodetect_encoding(std::span{std::as_const(udata)});
  auto r3d = simdutf::autodetect_encoding(std::as_const(udata));

  std::string stringdata{1, 2, 3, 4, 5};
  auto r4a = simdutf::autodetect_encoding(stringdata);
  auto r4b = simdutf::autodetect_encoding(std::span{stringdata});
  auto r4c = simdutf::autodetect_encoding(std::span{std::as_const(stringdata)});
  auto r4d = simdutf::autodetect_encoding(std::as_const(stringdata));

  std::string_view stringview_data{stringdata};
  auto r5a = simdutf::autodetect_encoding(stringview_data);
  auto r5b = simdutf::autodetect_encoding(std::as_const(stringview_data));

  std::vector<std::uint8_t> u8data{1, 2, 3, 4, 5};
  auto r6a = simdutf::autodetect_encoding(u8data);
  auto r6b = simdutf::autodetect_encoding(std::span{u8data});
  auto r6c = simdutf::autodetect_encoding(std::span{std::as_const(u8data)});
  auto r6d = simdutf::autodetect_encoding(std::as_const(u8data));

  std::vector<std::uint8_t> i8data{1, 2, 3, 4, 5};
  auto r7a = simdutf::autodetect_encoding(i8data);
  auto r7b = simdutf::autodetect_encoding(std::span{i8data});
  auto r7c = simdutf::autodetect_encoding(std::span{std::as_const(i8data)});
  auto r7d = simdutf::autodetect_encoding(std::as_const(i8data));

  std::vector<std::byte> bdata;
  auto r8a = simdutf::autodetect_encoding(bdata);
  auto r8b = simdutf::autodetect_encoding(std::span{bdata});
  auto r8c = simdutf::autodetect_encoding(std::span{std::as_const(bdata)});
  auto r8d = simdutf::autodetect_encoding(std::as_const(bdata));

  CustomSpan1 c;
  auto r9a = simdutf::autodetect_encoding(c);
  auto r9b = simdutf::autodetect_encoding(std::as_const(c));
  auto r9c = simdutf::autodetect_encoding(std::move(c));
}

/// this is used to show failure to compile
template <typename T>
concept is_autodetect_invokable =
    requires(T &input) { simdutf::autodetect_encoding(input); };

TEST(autodetect_should_not_be_invokable_on_non_byte_like_spans) {
  static_assert(!is_autodetect_invokable<std::vector<int>>);
  static_assert(!is_autodetect_invokable<const std::vector<int>>);

  static_assert(!is_autodetect_invokable<std::span<int>>);
  static_assert(!is_autodetect_invokable<std::span<const int>>);
}

/// a span-like class which returns an iterator as data (not a pointer)
struct CustomSpan2 {
  std::size_t size() const noexcept { return 3; }
  auto data() const noexcept { return m_data.begin(); }
  std::list<char> m_data;
};

TEST(autodetect_should_not_be_invokable_on_non_pointer_like_spans) {
  // simdutf::autodetect_encoding(CustomSpan2{});
  static_assert(!is_autodetect_invokable<CustomSpan2>);
}

/// this is used to show failure to compile
template <typename T>
concept is_validate_utf16_invokable =
    requires(T &input) { simdutf::validate_utf16(input); };

TEST(validate_utf16_handles_various_sources) {
  std::vector<char16_t> data{1, 2, 3, 4, 5};
  auto r1a = simdutf::validate_utf16(data);
  auto r1b = simdutf::validate_utf16(std::span{data});
  auto r1c = simdutf::validate_utf16(std::span{std::as_const(data)});
  auto r1d = simdutf::validate_utf16(std::as_const(data));
  auto r1e = simdutf::validate_utf16(std::move(data));

  static_assert(is_validate_utf16_invokable<std::vector<char16_t>>);
  static_assert(!is_validate_utf16_invokable<std::vector<char>>);
  static_assert(!is_validate_utf16_invokable<std::vector<std::uint16_t>>);
}

#endif
TEST_MAIN
