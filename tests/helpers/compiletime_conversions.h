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
template <typename CharType, std::size_t N>
constexpr auto to_latin1(const CTString<CharType, N> &input) {
  CTString<char, N> tmp;
  std::size_t ret;
  if constexpr (std::is_same_v<CharType, char32_t>) {
    ret = simdutf::convert_valid_utf32_to_latin1(input, tmp);
  } else {
    throw "unknown type";
  }
  if (!input.empty() && ret == 0) {
    throw "failed conversion";
  }
  return tmp;
}
} // namespace helpers
} // namespace tests
} // namespace simdutf

#endif // SIMDUTF_CPLUSPLUS23

#endif // SIMDUTF_COMPILETIME_CONVERSIONS_H
