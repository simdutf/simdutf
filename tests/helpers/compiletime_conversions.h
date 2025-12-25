#ifndef SIMDUTF_COMPILETIME_CONVERSIONS_H
#define SIMDUTF_COMPILETIME_CONVERSIONS_H

#include "simdutf/compiler_check.h"

#if SIMDUTF_CPLUSPLUS23

  #include <simdutf.h>

  #include <tests/helpers/fixed_string.h>

namespace simdutf {
namespace tests {
namespace helpers {

/**
 * creates a copy of input with the same endianness but with
 * illformed data replaced.
 */
template <typename CharType, std::size_t N, std::endian endianness>
constexpr auto to_wellformed(const CTString<CharType, N, endianness> &input) {
  CTString<CharType, N, endianness> output;
  if constexpr (endianness == std::endian::little) {
    simdutf::to_well_formed_utf16le(input, output);
  } else {
    simdutf::to_well_formed_utf16be(input, output);
  }
  return output;
}

/// converts valid input to latin1
template <typename CharType, std::size_t N, std::endian endianness>
constexpr auto to_latin1(const CTString<CharType, N, endianness> &input) {
  CTString<char, N> tmp;
  std::size_t ret;
  if constexpr (std::is_same_v<CharType, char32_t>) {
    ret = simdutf::convert_valid_utf32_to_latin1(input, tmp);
  } else if constexpr (std::is_same_v<CharType, char16_t> &&
                       endianness == std::endian::little) {
    ret = simdutf::convert_valid_utf16le_to_latin1(input, tmp);
  } else if constexpr (std::is_same_v<CharType, char16_t> &&
                       endianness == std::endian::big) {
    ret = simdutf::convert_valid_utf16be_to_latin1(input, tmp);
  } else {
    throw "unknown type";
  }
  if (!input.empty() && ret == 0) {
    throw "failed conversion";
  }
  return tmp;
}

/// converts latin1 input to utf8
template <auto input>
  requires latin1_ctstring<decltype(input)>
constexpr auto to_utf8() {
  constexpr auto N = utf8_length_from_latin1(input);
  CTString<char8_t, N> tmp;
  auto ret = simdutf::convert_latin1_to_utf8(input, tmp);
  if (!input.empty() && ret == 0) {
    throw "failed conversion";
  }
  return tmp;
}

namespace detail {
template <std::endian output_endianness, typename CharType, std::size_t N,
          std::endian input_endianness>
constexpr auto
to_utf16_impl(const CTString<CharType, N, input_endianness> &input) {

  if constexpr (std::is_same_v<CharType, char16_t>) {
    if constexpr (output_endianness == input_endianness) {
      // no-op
      return input;
    } else {
      // byteswap
      CTString<CharType, N, output_endianness> output;
      simdutf::change_endianness_utf16(input, output);
      return output;
    }
  }
}
} // namespace detail

/// converts valid input to utf16
template <typename CharType, std::size_t N, std::endian endianness>
constexpr auto to_utf16(const CTString<CharType, N, endianness> &input) {
  return detail::to_utf16_impl<std::endian::native>(input);
}

/// converts valid input to utf16le
template <typename CharType, std::size_t N, std::endian endianness>
constexpr auto to_utf16le(const CTString<CharType, N, endianness> &input) {
  return detail::to_utf16_impl<std::endian::little>(input);
}

/// converts valid input to utf16be
template <typename CharType, std::size_t N, std::endian endianness>
constexpr auto to_utf16be(const CTString<CharType, N, endianness> &input) {
  return detail::to_utf16_impl<std::endian::big>(input);
}

template <std::endian target_endianness>
constexpr auto latin1_to_utf16(latin1_ctstring auto &&input) {
  using I = decltype(input);
  constexpr auto N = I{}.size();
  CTString<char16_t, N, target_endianness> output;
  std::size_t converted;
  if constexpr (target_endianness == std::endian::little) {
    converted = convert_latin1_to_utf16le(input, output);
  } else {
    converted = convert_latin1_to_utf16be(input, output);
  }
  if (converted != N) {
    throw "oops";
  }
  return output;
}

template <std::endian target_endianness, bool with_errors, auto input>
constexpr auto utf8_to_utf16() {
  using namespace simdutf::tests::helpers;
  constexpr auto Nout = simdutf::utf16_length_from_utf8(input);
  CTString<char16_t, Nout, target_endianness> tmp{};
  std::size_t N;
  if constexpr (target_endianness == std::endian::little) {
    if constexpr (with_errors) {
      auto res = simdutf::convert_utf8_to_utf16le_with_errors(input, tmp);
      if (res.is_err()) {
        throw "fail";
      }
      N = res.count;
    } else {
      N = simdutf::convert_utf8_to_utf16le(input, tmp);
    }
  } else {
    if constexpr (with_errors) {
      auto res = simdutf::convert_utf8_to_utf16be_with_errors(input, tmp);
      if (res.is_err()) {
        throw "fail";
      }
      N = res.count;
    } else {
      N = simdutf::convert_utf8_to_utf16be(input, tmp);
    }
  }
  if (N != input.size()) {
    throw "oops";
  }
  return tmp;
}

template <std::endian target_endianness, auto input>
constexpr auto valid_utf8_to_utf16() {
  using namespace simdutf::tests::helpers;
  constexpr auto Nout = simdutf::utf16_length_from_utf8(input);
  CTString<char16_t, Nout, target_endianness> tmp{};
  std::size_t N;
  if constexpr (target_endianness == std::endian::little) {
    N = simdutf::convert_valid_utf8_to_utf16le(input, tmp);
  } else {
    N = simdutf::convert_valid_utf8_to_utf16be(input, tmp);
  }
  if (N != input.size()) {
    throw "oops";
  }
  return tmp;
}

} // namespace helpers
} // namespace tests
} // namespace simdutf

#endif // SIMDUTF_CPLUSPLUS23

#endif // SIMDUTF_COMPILETIME_CONVERSIONS_H
