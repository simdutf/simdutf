#include "simdutf.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <tuple>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/test.h>

#if SIMDUTF_CPLUSPLUS23

TEST(compile_time_length_from_binary) {
  using namespace simdutf::tests::helpers;
  const auto binary = "Abracadabra!"_latin1;
  const auto encoded = "QWJyYWNhZGFicmEh"_latin1;
  constexpr auto encoded_length =
      simdutf::base64_length_from_binary(binary.size());
  static_assert(encoded_length == encoded.size());
}

TEST(compile_time_maximal_binary_length) {
  using namespace simdutf::tests::helpers;
  constexpr auto binary = "Abracadabra!"_latin1;

  constexpr auto encoded = "QWJyYWNhZGFicmEh"_latin1;
  // char
  static_assert(simdutf::maximal_binary_length_from_base64(encoded) ==
                binary.size());
  // unsigned char
  static_assert(simdutf::maximal_binary_length_from_base64(
                    encoded.as_array<unsigned char>()) == binary.size());
  // signed char
  static_assert(simdutf::maximal_binary_length_from_base64(
                    encoded.as_array<signed char>()) == binary.size());
}

TEST(compile_time_maximal_binary_length16) {
  using namespace simdutf::tests::helpers;
  constexpr auto binary = "Abracadabra!"_latin1;
  constexpr auto encoded = u"QWJyYWNhZGFicmEh"_utf16;
  static_assert(simdutf::maximal_binary_length_from_base64(encoded) ==
                binary.size());
}

TEST(compile_time_binary_length_from_base64) {
  using namespace std::string_view_literals;

  // empty input
  static_assert(simdutf::binary_length_from_base64(""sv) == 0);
  static_assert(simdutf::binary_length_from_base64(" "sv) == 0);

  // increasing length of "a" repeated
  static_assert(simdutf::binary_length_from_base64("YQ=="sv) == 1);
  static_assert(simdutf::binary_length_from_base64("YWE="sv) == 2);
  static_assert(simdutf::binary_length_from_base64("YWFh"sv) == 3);
  static_assert(simdutf::binary_length_from_base64("YWFhYQ=="sv) == 4);
  static_assert(simdutf::binary_length_from_base64("YWFhYWE="sv) == 5);

  // all these are base64 of 'a', mixed with whitespace in different ways
  constexpr std::array mixedwithspaces{
      " YQ=="sv,     //
      "Y Q=="sv,     //
      "YQ =="sv,     //
      "YQ= ="sv,     //
      "YQ== "sv,     //
      " Y Q = = "sv, //
      " YQ = ="sv,   //
  };
  static_assert(std::ranges::all_of(mixedwithspaces, [](auto s) {
    return simdutf::binary_length_from_base64(s) == 1;
  }));
}

TEST(compile_time_binary_length_from_base64_utf16) {
  using namespace std::string_view_literals;

  // empty input
  static_assert(simdutf::binary_length_from_base64(u""sv) == 0);
  static_assert(simdutf::binary_length_from_base64(u" "sv) == 0);

  // increasing length of "a" repeated
  static_assert(simdutf::binary_length_from_base64(u"YQ=="sv) == 1);
  static_assert(simdutf::binary_length_from_base64(u"YWE="sv) == 2);
  static_assert(simdutf::binary_length_from_base64(u"YWFh"sv) == 3);
  static_assert(simdutf::binary_length_from_base64(u"YWFhYQ=="sv) == 4);
  static_assert(simdutf::binary_length_from_base64(u"YWFhYWE="sv) == 5);

  // all these are base64 of 'a', mixed with whitespace in different ways
  constexpr std::array mixedwithspaces{
      u" YQ=="sv,     //
      u"Y Q=="sv,     //
      u"YQ =="sv,     //
      u"YQ= ="sv,     //
      u"YQ== "sv,     //
      u" Y Q = = "sv, //
      u" YQ = ="sv,   //
  };
  static_assert(std::ranges::all_of(mixedwithspaces, [](auto s) {
    return simdutf::binary_length_from_base64(s) == 1;
  }));
}

namespace {

template <auto input>
  requires simdutf::tests::helpers::any_ctstring<decltype(input)>
constexpr auto b64_to_bin_impl() {
  using namespace simdutf::tests::helpers;
  constexpr auto Nmax = simdutf::maximal_binary_length_from_base64(input);
  CTString<char, Nmax> buffer{};
  auto res = simdutf::base64_to_binary(input, buffer);
  if (res.is_err()) {
    throw "failed convert";
  }
  if (res.count > Nmax) {
    throw "weird";
  }
  return std::tuple(res.count, buffer);
}

/**
 * converts base64 to binary. ideally the input should be passed as a
 * function parameter but I could not get that to work.
 */
template <auto input>
  requires simdutf::tests::helpers::any_ctstring<decltype(input)>
constexpr auto b64_to_binary() {
  constexpr auto r = b64_to_bin_impl<input>();
  constexpr auto N = std::get<0>(r);
  constexpr auto ret = std::get<1>(r);
  if constexpr (ret.size() != N) {
    return ret.template shrink<N>();
  } else {
    return ret;
  }
}

} // namespace

TEST(compile_time_length_from_binary_with_lines) {
  static_assert(4 == simdutf::base64_length_from_binary_with_lines(1));
  static_assert(137 == simdutf::base64_length_from_binary_with_lines(100));
}

TEST(compile_time_base64_to_binary) {
  using namespace simdutf::tests::helpers;
  constexpr auto binary = "Abracadabra!"_latin1;

  // tightly packed base64 works fine
  {
    constexpr auto base64 = "QWJyYWNhZGFicmEh"_latin1;
    constexpr auto binary_again = b64_to_binary<base64>();
    static_assert(binary_again == binary);
  }

  // extra spaces is no problem
  {
    constexpr auto base64 = "   QWJyYWNhZGF icmEh   "_latin1;
    constexpr auto binary_again = b64_to_binary<base64>();
    static_assert(binary_again == binary);
  }
}

TEST(compile_time_base64_utf16_to_binary) {
  using namespace simdutf::tests::helpers;
  constexpr auto binary = "Abracadabra!"_latin1;

  // tightly packed base64 works fine
  {
    constexpr auto base64 = u"QWJyYWNhZGFicmEh"_utf16;
    constexpr auto binary_again = b64_to_binary<base64>();
    static_assert(binary_again == binary);
  }

  // extra spaces is no problem
  {
    constexpr auto base64 = u"   QWJyYWNhZGF icmEh   "_utf16;
    constexpr auto binary_again = b64_to_binary<base64>();
    static_assert(binary_again == binary);
  }
}

namespace {
template <auto input>
  requires simdutf::tests::helpers::any_ctstring<decltype(input)>
constexpr auto binary_to_b64() {
  using namespace simdutf::tests::helpers;
  constexpr auto N = simdutf::base64_length_from_binary(input.size());
  CTString<char, N> buffer{};
  const auto r1 = simdutf::binary_to_base64(input, buffer);
  if (r1 != N) {
    throw "oops, size mismatch";
  }
  return buffer;
}
} // namespace

TEST(compile_time_binary_to_base64_char) {
  using namespace simdutf::tests::helpers;
  constexpr auto binary = "Abracadabra!"_latin1;
  constexpr auto expected = "QWJyYWNhZGFicmEh"_latin1;
  constexpr auto encoded = binary_to_b64<binary>();
  static_assert(expected == encoded);
}

namespace {
// this is just to demo that it is possible to do _base64 literals.
template <std::size_t N> struct Base64LiteralHelper {
  char storage[N - 1];

  static constexpr std::size_t size() noexcept { return N - 1; }

  constexpr Base64LiteralHelper(const char (&str)[N]) {
    static_assert(N > 1, "weird size");
    std::copy(str, str + size(), storage);
  }
};

template <Base64LiteralHelper a> constexpr auto operator""_base64() {
  using namespace simdutf::tests::helpers;
  constexpr auto N = a.size();
  constexpr std::span data(a.storage);
  constexpr CTString<char, N> tmp(data);
  return b64_to_binary<tmp>().template as_array<std::uint8_t>();
}

} // namespace

TEST(compile_time_base64_literal_demo) {
  using namespace simdutf::tests::helpers;

  constexpr std::array decoded = "QWJyYWNhZGFicmEh"_base64;
  const auto readable = std::string(begin(decoded), end(decoded));
  ASSERT_EQUAL(readable, "Abracadabra!");

  static_assert(decoded.size() == 12);
  static_assert(decoded[0] == 'A');
  static_assert(decoded[1] == 'b');
  static_assert(decoded[2] == 'r');
  static_assert(decoded[3] == 'a');
  static_assert(decoded[4] == 'c');
  static_assert(decoded[5] == 'a');
  static_assert(decoded[6] == 'd');
  static_assert(decoded[7] == 'a');
  static_assert(decoded[8] == 'b');
  static_assert(decoded[9] == 'r');
  static_assert(decoded[10] == 'a');
  static_assert(decoded[11] == '!');
}

namespace {
template <auto input, std::size_t lines>
  requires simdutf::tests::helpers::any_ctstring<decltype(input)>
constexpr auto binary_to_b64_with_lines() {
  using namespace simdutf::tests::helpers;
  constexpr auto N = simdutf::base64_length_from_binary_with_lines(
      input.size(), simdutf::base64_default, lines);
  CTString<char, N> buffer{};
  const auto r1 = simdutf::binary_to_base64_with_lines(input, buffer, lines,
                                                       simdutf::base64_default);
  if (r1 != N) {
    throw "oops, size mismatch";
  }
  return buffer;
}
} // namespace

TEST(compile_time_binary_to_base64_with_lines_char) {
  using namespace simdutf::tests::helpers;

  constexpr std::size_t lines = 4;
  constexpr auto binary = "Abracadabra!"_latin1;
  constexpr auto expected = "QWJy\nYWNh\nZGFi\ncmEh"_latin1;
  constexpr auto encoded = binary_to_b64_with_lines<binary, lines>();
  static_assert(expected == encoded);
}

TEST(compile_time_base64_ignorable) {
  static_assert(simdutf::base64_ignorable(' '));
  static_assert(not simdutf::base64_ignorable('x'));

  static_assert(simdutf::base64_ignorable(char16_t{' '}));
  static_assert(not simdutf::base64_ignorable(char16_t{'x'}));
}

TEST(compile_time_base64_valid) {
  static_assert(not simdutf::base64_valid(' '));
  static_assert(simdutf::base64_valid('x'));

  static_assert(not simdutf::base64_valid(char16_t{' '}));
  static_assert(simdutf::base64_valid(char16_t{'x'}));
}

TEST(compile_time_base64_valid_or_padding) {
  static_assert(not simdutf::base64_valid_or_padding(' '));
  static_assert(simdutf::base64_valid_or_padding('x'));
  static_assert(simdutf::base64_valid_or_padding('='));

  static_assert(not simdutf::base64_valid_or_padding(char16_t{' '}));
  static_assert(simdutf::base64_valid_or_padding(char16_t{'x'}));
  static_assert(simdutf::base64_valid_or_padding(char16_t{'='}));
}

namespace {

template <auto input>
  requires simdutf::tests::helpers::any_ctstring<decltype(input)>
constexpr auto b64_to_bin_safe_impl() {
  using namespace simdutf::tests::helpers;
  constexpr auto Nmax = simdutf::maximal_binary_length_from_base64(input);
  CTString<char, Nmax> buffer{};
  auto [res, outlen] = simdutf::base64_to_binary_safe(input, buffer);
  if (res.is_err()) {
    throw "failed convert";
  }
  if (res.count > input.size()) {
    throw "res.count > input.size()";
  }
  if (outlen > Nmax) {
    throw "outlen > Nmax";
  }
  return std::tuple(res.count, buffer, outlen);
}

/**
 * converts base64 to binary. ideally the input should be passed as a
 * function parameter but I could not get that to work.
 */
template <auto input>
  requires simdutf::tests::helpers::any_ctstring<decltype(input)>
constexpr auto b64_to_binary_safe() {
  constexpr auto r = b64_to_bin_safe_impl<input>();
  constexpr auto N_read_from_input = std::get<0>(r);
  constexpr auto ret = std::get<1>(r);
  constexpr auto outputbuffer_size = ret.size();
  constexpr auto outlen = std::get<2>(r);
  static_assert(N_read_from_input <= input.size());
  static_assert(outputbuffer_size >= outlen);
  if constexpr (ret.size() != outlen) {
    return ret.template shrink<outlen>();
  } else {
    return ret;
  }
}

} // namespace

TEST(compile_time_base64_to_binary_safe) {
  using namespace simdutf::tests::helpers;
  constexpr auto binary = "Abracadabra!"_latin1;
  static_assert(binary.size() == 12);

  // tightly packed base64 works fine
  {
    constexpr auto base64 = "QWJyYWNhZGFicmEh"_latin1;
    constexpr auto binary_again = b64_to_binary_safe<base64>();
    static_assert(binary_again == binary);
  }

  // extra spaces is no problem
  {
    constexpr auto base64 = "   QWJyYWNhZGF icmEh   "_latin1;
    constexpr auto binary_again = b64_to_binary_safe<base64>();
    static_assert(binary_again == binary);
  }

  // tightly packed base64 works fine also in utf-16
  {
    constexpr auto base64 = u"QWJyYWNhZGFicmEh"_utf16;
    constexpr auto binary_again = b64_to_binary_safe<base64>();
    static_assert(binary_again == binary);
  }

  // extra spaces is no problem also in utf-16
  {
    constexpr auto base64 = u"   QWJyYWNhZGF icmEh   "_utf16;
    constexpr auto binary_again = b64_to_binary_safe<base64>();
    static_assert(binary_again == binary);
  }
}

#else
TEST(no_compile_time_tests_below_cpp23) {}
#endif

TEST_MAIN
