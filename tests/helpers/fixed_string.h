#ifndef SIMDUTF_FIXED_STRING_H
#define SIMDUTF_FIXED_STRING_H

#include <simdutf/compiler_check.h>

#if SIMDUTF_CPLUSPLUS23

  #include <string>
  #include <cstddef>
  #include <span>
  #include <type_traits>
  #include <algorithm>
  #include <ranges>

namespace simdutf {
namespace tests {
namespace helpers {

namespace detail {
/// a concept that is true for the char types that are valid for representing
/// ascii/latin1, utf-8, utf-16 and utf-32
template <typename CharType>
concept valid_chartype = std::disjunction_v<std::is_same<CharType, char>,     //
                                            std::is_same<CharType, char8_t>,  //
                                            std::is_same<CharType, char16_t>, //
                                            std::is_same<CharType, char32_t>>;

/**
 * an internal helper type for use with user defined literals.
 *
 * implementation note: the reason for this is that the literal operator
 * template deduces the size including the null character. that makes the
 * template parameter N be the size+1, which is annoying.
 */
template <detail::valid_chartype CharType, std::size_t N>
struct TmpStringLiteral {
  CharType storage[N - 1];

  using type = CharType;

  static constexpr std::size_t size() noexcept { return N - 1; }

  /**
   * constructs from a string literal
   *
   * StringLiteral a("this is an ascii string");
   * StringLiteral b(u8"this is a utf-8 string");
   * StringLiteral c(u"this is a utf-16 string");
   * StringLiteral d(U"this is a utf-32 string");
   */
  constexpr /*explicit*/ TmpStringLiteral(CharType const (&str)[N]) {
    static_assert(N > 1, "weird size");
    std::copy(str, str + size(), storage);
  }
};
} // namespace detail

/**
 * A compile time string with fixed size, aimed to simplify testing since
 * std::string is hard to use at compile time due to it internally
 * allocating memory.
 *
 * clang-format off
 *
 * CharType implicitly determines the content:
 * char     - ascii/latin1
 * char8_t  - utf8
 * char16_t - utf16
 * char32_t - utf32
 *
 * clang-format on
 */
template <detail::valid_chartype CharType, std::size_t N> struct CTString {
  /// the string, not null terminated
  CharType storage[N];

  using type = CharType;
  constexpr CTString() = default;

  /// construction from a string literal (includes a null character)
  constexpr explicit CTString(CharType const (&str)[N + 1]) {
    std::copy(str, str + size(), storage);
  }

  /// construct from a range with the appropriate type
  constexpr explicit CTString(std::ranges::input_range auto &&range)
    requires detail::valid_chartype<
                 std::ranges::range_value_t<decltype(range)>> &&
             std::ranges::sized_range<decltype(range)>
  {
  #if !defined(_MSVC_LANG) && !defined(__clang__)
    constexpr auto Ninput = std::ranges::size(range);
    static_assert(Ninput == size());
  #endif
    std::ranges::copy(range, storage);
  }

  constexpr bool operator==(const CTString &other) const noexcept = default;

  /// size in number of code units
  static constexpr std::size_t size() noexcept { return N; }
  static constexpr bool empty() noexcept { return size() == 0; }
  constexpr CharType *data() noexcept { return storage; }
  constexpr const CharType *data() const noexcept { return storage; }
  constexpr auto span() noexcept { return std::span(storage); }
  constexpr auto span() const noexcept { return std::span(storage); }

  constexpr auto begin() noexcept { return data(); }
  constexpr auto end() noexcept { return data() + size(); }
  constexpr auto begin() const noexcept { return data(); }
  constexpr auto end() const noexcept { return data() + size(); }
  constexpr CharType operator[](auto index) const noexcept {
    return storage[index];
  }
  template <typename DestCharType> constexpr auto as_array() const noexcept {
    std::array<DestCharType, size()> ret;
    for (std::size_t i = 0; i < size(); ++i) {
      ret[i] = static_cast<DestCharType>(storage[i]);
    }
    return ret;
  }
};

/// Class template argument deduction (CTAD) for a string literal (null
/// terminated)
template <detail::valid_chartype CharType, std::size_t N>
CTString(const CharType (&str)[N]) -> CTString<CharType, N - 1>;

/// Class template argument deduction (CTAD) for a range (must have size known
/// at compile time)
template <std::ranges::input_range R>
CTString(R &&)
    -> CTString<std::ranges::range_value_t<R>, std::ranges::size(R{})>;

/// creates a fixed size string of UTF-32
template <detail::TmpStringLiteral a> constexpr auto operator""_utf32() {
  using C = typename decltype(a)::type;
  static_assert(
      std::is_same_v<C, char32_t>,
      "_utf32 user defined literal must be on the form U\"text...\"_utf32");
  return CTString<C, a.size()>(a.storage);
}

/// creates a fixed size string of UTF-16
template <detail::TmpStringLiteral a> constexpr auto operator""_utf16() {
  using C = typename decltype(a)::type;
  static_assert(
      std::is_same_v<C, char16_t>,
      "_utf16 user defined literal must be on the form u\"text...\"_utf16");
  return CTString<C, a.size()>(a.storage);
}

/// creates a fixed size string of UTF-8
template <detail::TmpStringLiteral a> constexpr auto operator""_utf8() {
  using C = typename decltype(a)::type;
  static_assert(
      std::is_same_v<C, char8_t>,
      "_utf8 user defined literal must be on the form u8\"text...\"_utf8");
  return CTString<C, a.size()>(a.storage);
}

/// creates a fixed size string of ascii/latin1
template <detail::TmpStringLiteral a> constexpr auto operator""_latin1() {
  using C = typename decltype(a)::type;
  static_assert(
      std::is_same_v<C, char>,
      "_latin1 user defined literal must be on the form \"text...\"_latin1");
  return CTString<C, a.size()>(a.storage);
}

namespace detail {
template <class T> struct is_CTString : std::false_type {};

template <typename CharType, std::size_t N>
struct is_CTString<CTString<CharType, N>> : std::true_type {};

} // namespace detail

/// concept which gives true for any CTString (regardless of if it is latin1,
/// utf-8, utf-16 or utf-32)
template <class T>
concept any_ctstring = detail::is_CTString<std::decay_t<T>>::value;

template <class T>
concept latin1_ctstring =
    (detail::is_CTString<std::decay_t<T>>::value &&
     std::is_same_v<std::decay_t<typename std::decay_t<T>::type>, char>);

template <class T>
concept utf8_ctstring =
    (detail::is_CTString<std::decay_t<T>>::value &&
     std::is_same_v<typename std::decay_t<T>::type, char8_t>);

template <class T>
concept utf16_ctstring =
    (detail::is_CTString<std::decay_t<T>>::value &&
     std::is_same_v<typename std::decay_t<T>::type, char16_t>);

template <class T>
concept utf32_ctstring =
    (detail::is_CTString<std::decay_t<T>>::value &&
     std::is_same_v<typename std::decay_t<T>::type, char32_t>);

} // namespace helpers
} // namespace tests
} // namespace simdutf
#endif // SIMDUTF_CPLUSPLUS23
#endif // SIMDUTF_FIXED_STRING_H
