#include "simdutf.h"

#include <array>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/test.h>

#if SIMDUTF_CPLUSPLUS23

TEST(construct_from_user_defined_literals) {
  using namespace simdutf::tests::helpers;

  constexpr auto utf32 = U"köttbulle"_utf32;
  static_assert(utf32 == CTString(U"köttbulle"));
  static_assert(utf32.size() == 9);
  static_assert(utf32[0] == char32_t{'k'});
  static_assert(std::is_same_v<decltype(utf32[0]), char32_t>);

  constexpr auto utf16 = u"köttbulle"_utf16;
  static_assert(utf16 == CTString(u"köttbulle"));
  static_assert(utf16.size() == 9);
  static_assert(utf16[0] == char16_t{'k'});
  static_assert(std::is_same_v<decltype(utf16[0]), char16_t>);

  static_assert(sizeof(utf32) == 2 * sizeof(utf16));

  constexpr auto utf8 = u8"köttbulle"_utf8;
  static_assert(utf8 == CTString(u8"köttbulle"));
  static_assert(utf8.size() == 9 + 1);
  static_assert(utf8[0] == char8_t{'k'});
  static_assert(std::is_same_v<decltype(utf8[0]), char8_t>);

  constexpr auto latin1 = "kottbulle"_latin1;
  static_assert(latin1 == CTString("kottbulle"));
  // intentionally fails at compile time due to not being latin1:
  // constexpr auto latin1_fail = "köttbulle"_latin1;
  static_assert(latin1.size() == 9);
  static_assert(sizeof(latin1) == 9);
  static_assert(latin1[0] == char8_t{'k'});
  static_assert(std::is_same_v<decltype(latin1[0]), char>);

  static_assert(sizeof(utf8) == sizeof(latin1) + 1);
  static_assert(sizeof(utf32) == 4 * sizeof(latin1));
}

namespace {
template <typename T>
concept test_construct_from_range = requires(const std::array<T, 3> &dummy) {
  {
    // this tries to ensure that the following compiles:
    // constexpr simdutf::tests::helpers::CTString tmp{dummy};

    std::integral_constant<
        std::size_t, sizeof(simdutf::tests::helpers::CTString{dummy}) * 0>{}
  } -> std::same_as<std::integral_constant<std::size_t, 0>>;
};
} // namespace

TEST(construct_from_range) {
  using namespace simdutf::tests::helpers;
  constexpr std::array<char32_t, 5> tmp{'h', 'e', 'l', 'l', 'o'};
  constexpr CTString fromrange(tmp);
  static_assert(fromrange[0] == 'h');
  static_assert(fromrange[1] == 'e');
  static_assert(fromrange[2] == 'l');
  static_assert(fromrange[3] == 'l');
  static_assert(fromrange[4] == 'o');
  static_assert(std::is_same_v<decltype(fromrange[0]), char32_t>);

  static_assert(test_construct_from_range<char>);
  static_assert(test_construct_from_range<const char>);

  static_assert(test_construct_from_range<char8_t>);
  static_assert(test_construct_from_range<const char8_t>);

  static_assert(test_construct_from_range<char16_t>);
  static_assert(test_construct_from_range<const char16_t>);

  static_assert(test_construct_from_range<char32_t>);
  static_assert(test_construct_from_range<const char32_t>);

  static_assert(not test_construct_from_range<int>);
}

// this also tests the concepts
TEST(construct_from_literals) {
  using namespace simdutf::tests::helpers;

  static_assert(CTString("abc").size() == 3);

  static_assert(not any_ctstring<int>);
  static_assert(not latin1_ctstring<int>);
  static_assert(not any_ctstring<const int>);
  static_assert(not latin1_ctstring<const int>);

  constexpr CTString a("this is a latin1 string");
  using A = std::remove_cvref_t<decltype(a)>;
  static_assert(latin1_ctstring<A>);
  static_assert(latin1_ctstring<const A>);
  static_assert(latin1_ctstring<A &>);
  static_assert(latin1_ctstring<A &&>);
  static_assert(latin1_ctstring<const A &>);
  static_assert(not utf8_ctstring<A>);
  static_assert(not utf16_ctstring<A>);
  static_assert(not utf32_ctstring<A>);

  constexpr CTString b(u8"this is a utf-8 string");
  static_assert(not latin1_ctstring<decltype(b)>);
  static_assert(utf8_ctstring<decltype(b)>);
  static_assert(not utf16_ctstring<decltype(b)>);
  static_assert(not utf32_ctstring<decltype(b)>);

  constexpr CTString c(u"this is a utf-16 string");
  static_assert(not latin1_ctstring<decltype(c)>);
  static_assert(not utf8_ctstring<decltype(c)>);
  static_assert(utf16_ctstring<decltype(c)>);
  static_assert(not utf32_ctstring<decltype(c)>);

  constexpr CTString d(U"this is a utf-32 string");
  static_assert(not latin1_ctstring<decltype(d)>);
  static_assert(not utf8_ctstring<decltype(d)>);
  static_assert(not utf16_ctstring<decltype(d)>);
  static_assert(utf32_ctstring<decltype(d)>);
}

TEST(test_endianness) {
  using namespace simdutf::tests::helpers;
  // make them equally long so we can compare the type (the length is encoded in
  // the type)
  constexpr auto native = u"NATIVE"_utf16;
  constexpr auto little = u"BIGBIG"_utf16le;
  constexpr auto bigbig = u"LITTLE"_utf16be;
  using Native = decltype(native);
  using Big = decltype(bigbig);
  using Little = decltype(little);
  static_assert(utf16_ctstring<Native>);
  static_assert(utf16_ctstring<Big>);
  static_assert(utf16_ctstring<Little>);
  constexpr bool is_little = (std::endian::native == std::endian::little);
  static_assert(utf16le_ctstring<Native> == is_little);
  static_assert(utf16be_ctstring<Native> == !is_little);

  static_assert(!std::is_same_v<Big, Little>);
  static_assert(std::is_same_v<Big, Native> || std::is_same_v<Little, Native>);

  static_assert(little[0] != bigbig[0]);
}

#else
TEST(nothing_happens_when_not_in_cxx23_mode) {}
#endif

TEST_MAIN
