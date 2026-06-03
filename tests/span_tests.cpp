#include "simdutf.h"

#include <array>
#include <utility>
#include <vector>

#include <tests/helpers/test.h>

/// a span-like class which returns int as size (not std::size_t)
struct CustomSpan1 {
  int size() const noexcept { return 3; }
  const char *data() const noexcept { return "hej"; }
};

#if SIMDUTF_SPAN
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

  std::u8string u8stringdata{1, 2, 3, 4, 5};
  auto r10a = simdutf::autodetect_encoding(u8stringdata);
  auto r10b = simdutf::autodetect_encoding(std::span{u8stringdata});
  auto r10c =
      simdutf::autodetect_encoding(std::span{std::as_const(u8stringdata)});
  auto r10d = simdutf::autodetect_encoding(std::as_const(u8stringdata));

  std::u8string_view u8stringview_data{u8stringdata};
  auto r11a = simdutf::autodetect_encoding(u8stringview_data);
  auto r11b = simdutf::autodetect_encoding(std::as_const(u8stringview_data));
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

/// this is used to show failure to compile
template <typename T>
concept is_validate_utf32_invokable =
    requires(T &input) { simdutf::validate_utf32(input); };

TEST(validate_utf32_handles_various_sources) {
  std::vector<char32_t> data{1, 2, 3, 4, 5};
  auto r1a = simdutf::validate_utf32(data);
  auto r1b = simdutf::validate_utf32(std::span{data});
  auto r1c = simdutf::validate_utf32(std::span{std::as_const(data)});
  auto r1d = simdutf::validate_utf32(std::as_const(data));
  auto r1e = simdutf::validate_utf32(std::move(data));

  static_assert(is_validate_utf32_invokable<std::vector<char32_t>>);
  static_assert(!is_validate_utf32_invokable<std::vector<char>>);
  static_assert(!is_validate_utf32_invokable<std::vector<std::uint32_t>>);
}

/// this is used to show failure to compile
template <typename T, typename U>
concept is_convert_latin1_to_utf8_safe_invokable =
    requires(T &input, U &output) {
      simdutf::convert_latin1_to_utf8_safe(input, output);
    };

// this is used to prove that no copying takes place in the output parameter
struct non_movable_or_copyable_span {
  non_movable_or_copyable_span &
  operator=(non_movable_or_copyable_span &&) = delete;
  auto data() noexcept { return m_data; }
  auto data() const noexcept { return m_data; }
  auto size() const noexcept { return m_size; }
  char *m_data{};
  std::size_t m_size{};
};
static_assert(!std::movable<non_movable_or_copyable_span>);
static_assert(!std::copyable<non_movable_or_copyable_span>);

TEST(convert_latin1_to_utf8_safe) {
  const std::vector<char> input{1, 2, 3, 4, 5};
  std::vector<char> output(input.size());
  auto r1a = simdutf::convert_latin1_to_utf8_safe(input, output);
  auto r1b = simdutf::convert_latin1_to_utf8_safe(input, std::span{output});
  auto r1c =
      simdutf::convert_latin1_to_utf8_safe(std::move(input), std::span{output});

  // we handle const, mutable and rvalues for the output parameter
  const std::span s1{output};
  auto r1d = simdutf::convert_latin1_to_utf8_safe(input, s1);
  std::span s2{output};
  auto r1e = simdutf::convert_latin1_to_utf8_safe(input, s2);
  auto r1f = simdutf::convert_latin1_to_utf8_safe(input, std::move(s2));

  // make sure no copying or move happens in the output parameter
  const non_movable_or_copyable_span nmcms{.m_data = output.data(),
                                           .m_size = output.size()};
  auto r2a = simdutf::convert_latin1_to_utf8_safe(input, nmcms);
  auto r2b = simdutf::convert_latin1_to_utf8_safe(
      input, non_movable_or_copyable_span{.m_data = output.data(),
                                          .m_size = output.size()});

  // the output can be anything byte like
  std::vector<unsigned char> output3(input.size());
  auto r3a = simdutf::convert_latin1_to_utf8_safe(input, output3);
  //... but not compile if it is anything else
  static_assert(
      !is_convert_latin1_to_utf8_safe_invokable<std::vector<char>,
                                                std::vector<char16_t>>);

  // writing to a const object should not be possible
  static_assert(
      !is_convert_latin1_to_utf8_safe_invokable<std::vector<char>,
                                                const std::vector<char>>);

  // writing to a std::string through .data() is ok since C++17
  static_assert(
      is_convert_latin1_to_utf8_safe_invokable<std::vector<char>, std::string>);
  static_assert(
      is_convert_latin1_to_utf8_safe_invokable<std::string, std::vector<char>>);
  static_assert(is_convert_latin1_to_utf8_safe_invokable<std::string_view,
                                                         std::vector<char>>);
}

TEST(validate_utf32_with_errors) {
  std::array<char32_t, 3> data{1, 2, 3};
  auto r1a = simdutf::validate_utf32_with_errors(data);
  auto r1b = simdutf::validate_utf32_with_errors(std::as_const(data));
}

TEST(convert_latin1_to_utf16le) {
  std::string input{"hej!"};
  std::array<char16_t, 4> output{};
  auto r1a = simdutf::convert_latin1_to_utf16le(input, output);
  auto r1b = simdutf::convert_latin1_to_utf16le(std::as_const(input), output);
}

TEST(convert_utf8_to_latin1) {
  std::string input{"hej!"};

  std::array<signed char, 4> output{};
  auto r1a = simdutf::convert_utf8_to_latin1(input, output);
  auto r1b = simdutf::convert_utf8_to_latin1(std::as_const(input), output);

  std::array<unsigned char, 4> output2{};
  auto r2a = simdutf::convert_utf8_to_latin1(input, output2);
  auto r2b = simdutf::convert_utf8_to_latin1(std::as_const(input), output2);
}

TEST(convert_utf8_to_utf16) {
  std::string input{"hej!"};

  std::array<char16_t, 4> output{};
  auto r1a = simdutf::convert_utf8_to_utf16(input, output);
  auto r1b = simdutf::convert_utf8_to_utf16(std::as_const(input), output);
}

TEST(convert_utf16_to_utf8) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16_to_utf8(input, output);
  auto r1b = simdutf::convert_utf16_to_utf8(std::as_const(input), output);
}

TEST(convert_utf16_to_latin1) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16_to_latin1(input, output);
  auto r1b = simdutf::convert_utf16_to_latin1(std::as_const(input), output);
}

TEST(convert_utf16le_to_latin1) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16le_to_latin1(input, output);
  auto r1b = simdutf::convert_utf16le_to_latin1(std::as_const(input), output);
}

TEST(convert_utf16be_to_latin1) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16be_to_latin1(input, output);
  auto r1b = simdutf::convert_utf16be_to_latin1(std::as_const(input), output);
}

TEST(convert_utf16le_to_utf8) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16le_to_utf8(input, output);
  auto r1b = simdutf::convert_utf16le_to_utf8(std::as_const(input), output);
}

TEST(convert_utf16be_to_utf8) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16be_to_utf8(input, output);
  auto r1b = simdutf::convert_utf16be_to_utf8(std::as_const(input), output);
}

TEST(convert_utf16_to_latin1_with_errors) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16_to_latin1_with_errors(input, output);
  auto r1b = simdutf::convert_utf16_to_latin1_with_errors(std::as_const(input),
                                                          output);
}

TEST(convert_utf16le_to_latin1_with_errors) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16le_to_latin1_with_errors(input, output);
  auto r1b = simdutf::convert_utf16le_to_latin1_with_errors(
      std::as_const(input), output);
}

TEST(convert_utf16be_to_latin1_with_errors) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16be_to_latin1_with_errors(input, output);
  auto r1b = simdutf::convert_utf16be_to_latin1_with_errors(
      std::as_const(input), output);
}

TEST(convert_utf16_to_utf8_with_errors) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16_to_utf8_with_errors(input, output);
  auto r1b =
      simdutf::convert_utf16_to_utf8_with_errors(std::as_const(input), output);
}

TEST(convert_utf16le_to_utf8_with_errors) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16le_to_utf8_with_errors(input, output);
  auto r1b = simdutf::convert_utf16le_to_utf8_with_errors(std::as_const(input),
                                                          output);
}

TEST(convert_utf16be_to_utf8_with_errors) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf16be_to_utf8_with_errors(input, output);
  auto r1b = simdutf::convert_utf16be_to_utf8_with_errors(std::as_const(input),
                                                          output);
}

TEST(convert_valid_utf16_to_utf8) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_valid_utf16_to_utf8(input, output);
  auto r1b = simdutf::convert_valid_utf16_to_utf8(std::as_const(input), output);
}

TEST(convert_valid_utf16_to_latin1) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_valid_utf16_to_latin1(input, output);
  auto r1b =
      simdutf::convert_valid_utf16_to_latin1(std::as_const(input), output);
}

TEST(convert_valid_utf16le_to_latin1) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_valid_utf16le_to_latin1(input, output);
  auto r1b =
      simdutf::convert_valid_utf16le_to_latin1(std::as_const(input), output);
}

TEST(convert_valid_utf16be_to_latin1) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_valid_utf16be_to_latin1(input, output);
  auto r1b =
      simdutf::convert_valid_utf16be_to_latin1(std::as_const(input), output);
}

TEST(convert_valid_utf16le_to_utf8) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_valid_utf16le_to_utf8(input, output);
  auto r1b =
      simdutf::convert_valid_utf16le_to_utf8(std::as_const(input), output);
}

TEST(convert_valid_utf16be_to_utf8) {
  std::array<char16_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_valid_utf16be_to_utf8(input, output);
  auto r1b =
      simdutf::convert_valid_utf16be_to_utf8(std::as_const(input), output);
}

TEST(convert_utf32_to_latin1) {
  std::array<char32_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf32_to_latin1(input, output);
  auto r1b = simdutf::convert_utf32_to_latin1(std::as_const(input), output);
}

TEST(convert_utf32_to_latin1_with_errors) {
  std::array<char32_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_utf32_to_latin1_with_errors(input, output);
  auto r1b = simdutf::convert_utf32_to_latin1_with_errors(std::as_const(input),
                                                          output);
}

TEST(convert_valid_utf32_to_latin1) {
  std::array<char32_t, 4> input{};
  std::string output;
  auto r1a = simdutf::convert_valid_utf32_to_latin1(input, output);
  auto r1b =
      simdutf::convert_valid_utf32_to_latin1(std::as_const(input), output);
}

// ---------------------------------------------------------------------------
// Tests for the short-input scalar fast path in the convert_utf8_to_utf16le/be
// span overloads (PR #986).  The overloads are simdutf_really_inline in
// implementation.h; for inputs <= 16 bytes they call the scalar path directly
// instead of going through the SIMD dispatcher.
// ---------------------------------------------------------------------------

// Helpers: compare span result against the C-style reference.
namespace {
std::vector<char16_t> ref_le(const char *u8, size_t n) {
  std::vector<char16_t> out(n + 1);
  out.resize(simdutf::convert_utf8_to_utf16le(u8, n, out.data()));
  return out;
}
std::vector<char16_t> span_le(const char *u8, size_t n) {
  std::vector<char16_t> out(n + 1);
  std::span<char16_t> sp{out.data(), out.size()};
  out.resize(simdutf::convert_utf8_to_utf16le(
      std::span<const char>{u8, n}, sp));
  return out;
}
std::vector<char16_t> ref_be(const char *u8, size_t n) {
  std::vector<char16_t> out(n + 1);
  out.resize(simdutf::convert_utf8_to_utf16be(u8, n, out.data()));
  return out;
}
std::vector<char16_t> span_be(const char *u8, size_t n) {
  std::vector<char16_t> out(n + 1);
  std::span<char16_t> sp{out.data(), out.size()};
  out.resize(simdutf::convert_utf8_to_utf16be(
      std::span<const char>{u8, n}, sp));
  return out;
}
} // namespace

// All ASCII sizes 0..32 — exercises the scalar branch (<=16) and the
// dispatch fallback (>16) to verify both paths produce identical output.
TEST(span_convert_utf8_to_utf16le_ascii_matches_cstyle) {
  const std::string s(32, 'a');
  for (size_t n = 0; n <= 32; ++n) {
    ASSERT_EQUAL(ref_le(s.data(), n), span_le(s.data(), n));
  }
}

TEST(span_convert_utf8_to_utf16be_ascii_matches_cstyle) {
  const std::string s(32, 'z');
  for (size_t n = 0; n <= 32; ++n) {
    ASSERT_EQUAL(ref_be(s.data(), n), span_be(s.data(), n));
  }
}

// 2-byte UTF-8 sequences (é U+00E9 = 0xC3 0xA9).
TEST(span_convert_utf8_to_utf16le_two_byte_matches_cstyle) {
  std::string s;
  for (int i = 0; i < 10; ++i) s += "\xc3\xa9"; // 20 bytes
  for (size_t n = 0; n <= s.size(); n += 2) {
    ASSERT_EQUAL(ref_le(s.data(), n), span_le(s.data(), n));
  }
}

// 3-byte UTF-8 sequences (漢 U+6F22 = 0xE6 0xBC 0xA2).
TEST(span_convert_utf8_to_utf16le_three_byte_matches_cstyle) {
  std::string s;
  for (int i = 0; i < 7; ++i) s += "\xe6\xbc\xa2"; // 21 bytes, straddles threshold
  for (size_t n = 0; n <= s.size(); n += 3) {
    ASSERT_EQUAL(ref_le(s.data(), n), span_le(s.data(), n));
  }
}

// 4-byte UTF-8 sequences (😀 U+1F600 = 0xF0 0x9F 0x98 0x80).
TEST(span_convert_utf8_to_utf16le_four_byte_matches_cstyle) {
  std::string s;
  for (int i = 0; i < 5; ++i) s += "\xf0\x9f\x98\x80"; // 20 bytes
  for (size_t n = 0; n <= s.size(); n += 4) {
    ASSERT_EQUAL(ref_le(s.data(), n), span_le(s.data(), n));
  }
}

// Threshold boundary: 14, 15, 16 → scalar branch; 17, 18 → dispatch.
TEST(span_convert_utf8_to_utf16le_threshold_boundary) {
  const std::string s(32, 'B');
  for (size_t n = 14; n <= 18; ++n) {
    ASSERT_EQUAL(ref_le(s.data(), n), span_le(s.data(), n));
  }
}

// Exactly 16 bytes of 2-byte sequences (8 × é): must be in scalar branch.
TEST(span_convert_utf8_to_utf16le_exactly_16_bytes_two_byte_seq) {
  std::string s;
  for (int i = 0; i < 8; ++i) s += "\xc3\xa9";
  ASSERT_EQUAL(size_t{16}, s.size());
  ASSERT_EQUAL(ref_le(s.data(), 16), span_le(s.data(), 16));
}

// 17 bytes must fall through to the SIMD dispatcher and still be correct.
TEST(span_convert_utf8_to_utf16le_17_bytes_falls_through_to_dispatch) {
  const std::string s(17, 'C');
  ASSERT_EQUAL(ref_le(s.data(), 17), span_le(s.data(), 17));
}

// _with_errors variants: invalid UTF-8 must return the same error code
// and position via span as via C-style.
TEST(span_convert_utf8_to_utf16le_with_errors_bad_continuation) {
  const char bad[] = "hi\x80ok";
  size_t len = sizeof(bad) - 1;
  std::vector<char16_t> out_c(len + 1), out_s(len + 1);
  auto rc = simdutf::convert_utf8_to_utf16le_with_errors(bad, len, out_c.data());
  auto rs = simdutf::convert_utf8_to_utf16le_with_errors(
      std::span<const char>{bad, len},
      std::span<char16_t>{out_s.data(), out_s.size()});
  ASSERT_EQUAL(rc.error, rs.error);
  ASSERT_EQUAL(rc.count, rs.count);
}

TEST(span_convert_utf8_to_utf16be_with_errors_bad_continuation) {
  const char bad[] = "AB\x80" "CD"; // split to stop hex escape at \x80
  size_t len = sizeof(bad) - 1;
  std::vector<char16_t> out_c(len + 1), out_s(len + 1);
  auto rc = simdutf::convert_utf8_to_utf16be_with_errors(bad, len, out_c.data());
  auto rs = simdutf::convert_utf8_to_utf16be_with_errors(
      std::span<const char>{bad, len},
      std::span<char16_t>{out_s.data(), out_s.size()});
  ASSERT_EQUAL(rc.error, rs.error);
  ASSERT_EQUAL(rc.count, rs.count);
}

// LE and BE must differ in byte order for non-ASCII output.
TEST(span_convert_utf8_to_utf16le_and_be_byte_order_distinct) {
  // é U+00E9: LE stores 0xE9 0x00, BE stores 0x00 0xE9.
  auto le = span_le("\xc3\xa9", 2);
  auto be = span_be("\xc3\xa9", 2);
  ASSERT_EQUAL(size_t{1}, le.size());
  ASSERT_EQUAL(size_t{1}, be.size());
  ASSERT_TRUE(le[0] != be[0]);
}
#endif
TEST_MAIN
