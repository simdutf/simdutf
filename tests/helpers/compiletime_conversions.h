#ifndef SIMDUTF_COMPILETIME_CONVERSIONS_H
#define SIMDUTF_COMPILETIME_CONVERSIONS_H

#include "simdutf/compiler_check.h"

#if SIMDUTF_CPLUSPLUS23

  #include <simdutf.h>

  #include <tests/helpers/fixed_string.h>

namespace simdutf {
namespace tests {
namespace helpers {

constexpr auto to_wellformed(utf16_ctstring auto &&input) {
  using T = std::decay_t<decltype(input)>;
  T output;
  simdutf::to_well_formed_utf16le(input, output);
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

} // namespace helpers
} // namespace tests
} // namespace simdutf

#endif // SIMDUTF_CPLUSPLUS23

#endif // SIMDUTF_COMPILETIME_CONVERSIONS_H
