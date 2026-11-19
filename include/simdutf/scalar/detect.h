#ifndef SIMDUTF_SCALAR_DETECT_H
#define SIMDUTF_SCALAR_DETECT_H

#include "simdutf/encoding_types.h"
#include "simdutf/scalar/utf8.h"
#include "simdutf/scalar/utf16.h"
#include "simdutf/scalar/utf32.h"

namespace simdutf {
namespace scalar {

namespace detail {

/**
 * Adapter that reinterprets a byte pointer as UTF-16 code units.
 * Used for constexpr encoding detection where reinterpret_cast is not allowed.
 * Reads two consecutive bytes and combines them into a char16_t.
 */
template <typename BytePtr> struct u16_ptr_shim {
  BytePtr ptr;
  simdutf_constexpr23 char16_t operator[](size_t i) const {
    size_t idx = i * 2;
#ifdef SIMDUTF_IS_BIG_ENDIAN
    return static_cast<char16_t>((ptr[idx] << 8) | ptr[idx + 1]);
#else
    return static_cast<char16_t>(ptr[idx] | (ptr[idx + 1] << 8));
#endif
  }
};

/**
 * Adapter that reinterprets a byte pointer as UTF-32 code units.
 * Used for constexpr encoding detection where reinterpret_cast is not allowed.
 * Reads four consecutive bytes and combines them into a uint32_t.
 */
template <typename BytePtr> struct u32_ptr_shim {
  BytePtr ptr;
  simdutf_constexpr23 uint32_t operator[](size_t i) const {
    size_t idx = i * 4;
#ifdef SIMDUTF_IS_BIG_ENDIAN
    return static_cast<uint32_t>(
        (uint32_t(ptr[idx]) << 24) | (uint32_t(ptr[idx + 1]) << 16) |
        (uint32_t(ptr[idx + 2]) << 8) | uint32_t(ptr[idx + 3]));
#else
    return static_cast<uint32_t>(
        uint32_t(ptr[idx]) | (uint32_t(ptr[idx + 1]) << 8) |
        (uint32_t(ptr[idx + 2]) << 16) | (uint32_t(ptr[idx + 3]) << 24));
#endif
  }
};

} // namespace detail

template <typename InputPtr>
simdutf_warn_unused simdutf_really_inline simdutf_constexpr23 encoding_type
autodetect_encoding(InputPtr input, size_t length) noexcept {
  auto bom_encoding = simdutf::BOM::check_bom(input, length);
  if (bom_encoding != encoding_type::unspecified) {
    return bom_encoding;
  }
  if (scalar::utf8::validate(input, length)) {
    return encoding_type::UTF8;
  }
  if ((length % 2) == 0) {
    if (scalar::utf16::validate<endianness::LITTLE>(
            detail::u16_ptr_shim<InputPtr>{input}, length / 2)) {
      return encoding_type::UTF16_LE;
    }
  }
  if ((length % 4) == 0) {
    if (scalar::utf32::validate(detail::u32_ptr_shim<InputPtr>{input},
                                length / 4)) {
      return encoding_type::UTF32_LE;
    }
  }
  return encoding_type::unspecified;
}

template <typename InputPtr>
simdutf_warn_unused simdutf_really_inline simdutf_constexpr23 int
detect_encodings(InputPtr input, size_t length) noexcept {
  auto bom_encoding = simdutf::BOM::check_bom(input, length);
  if (bom_encoding != encoding_type::unspecified) {
    return bom_encoding;
  }
  int out = 0;
  if (scalar::utf8::validate(input, length)) {
    out |= encoding_type::UTF8;
  }
  if ((length % 2) == 0) {
    if (scalar::utf16::validate<endianness::LITTLE>(
            detail::u16_ptr_shim<InputPtr>{input}, length / 2)) {
      out |= encoding_type::UTF16_LE;
    }
  }
  if ((length % 4) == 0) {
    if (scalar::utf32::validate(detail::u32_ptr_shim<InputPtr>{input},
                                length / 4)) {
      out |= encoding_type::UTF32_LE;
    }
  }
  return out;
}

} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_SCALAR_DETECT_H
