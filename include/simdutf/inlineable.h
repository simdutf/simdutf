#ifndef SIMDUTF_INLINEABLE_H
#define SIMDUTF_INLINEABLE_H

/**
 * @file inlineable.h
 * @brief Header-only, fully inlineable scalar variants of the public simdutf
 * API.
 *
 * The functions in the `simdutf::inlineable` namespace mimic the signatures of
 * the corresponding `simdutf::` public API functions, but they are implemented
 * directly against the header-only scalar routines in simdutf/scalar/. This
 * means a call to `simdutf::inlineable::convert_utf8_to_utf16(...)` can be
 * inlined (and constant-folded when the input is known at compile time) into
 * the caller, because no out-of-line function call into the compiled simdutf
 * library is made.
 *
 * These scalar routines are typically slower than the dispatched SIMD
 * implementations in the simdutf library for large inputs, but they avoid the
 * fixed cost of an indirect function call through the runtime dispatcher. For
 * very small inputs (on the order of a few dozen bytes) they can therefore be
 * faster in practice.
 *
 * Recommended usage pattern:
 * @code
 *   size_t convert(const char *input, size_t len, char16_t *out) {
 *     constexpr size_t small = 64; // tune to your workload
 *     if (len < small) {
 *       return simdutf::inlineable::convert_utf8_to_utf16(input, len, out);
 *     }
 *     return simdutf::convert_utf8_to_utf16(input, len, out);
 *   }
 * @endcode
 *
 * @note This header must be included after `<simdutf.h>` (or
 *       `simdutf/implementation.h`), which pulls in the scalar headers that
 *       these wrappers depend on.
 */

#include "simdutf/implementation.h"

namespace simdutf {
namespace inlineable {

// -----------------------------------------------------------------------------
// UTF-8 validation
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused bool
validate_utf8(const char *buf, size_t len) noexcept {
  #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8::validate(
        detail::constexpr_cast_ptr<uint8_t>(input.data()), input.size());
  } else
  #endif
  {
    return scalar::utf8::validate(buf, len);
  }
}
#endif // SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
validate_utf8_with_errors(const char *buf, size_t len) noexcept {
  #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8::validate(
        detail::constexpr_cast_ptr<uint8_t>(input.data()), input.size());
  } else
  #endif
  {
    return scalar::utf8::validate_with_errors(buf, len);
  }
}
#endif // SIMDUTF_FEATURE_UTF8

// -----------------------------------------------------------------------------
// ASCII validation
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_ASCII
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused bool
validate_ascii(const char *buf, size_t len) noexcept {
  return scalar::ascii::validate(buf, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
validate_ascii_with_errors(const char *buf, size_t len) noexcept {
  return scalar::ascii::validate_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_ASCII

// -----------------------------------------------------------------------------
// UTF-16 validation
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused bool
validate_utf16(const char16_t *buf, size_t len) noexcept {
  return scalar::utf16::validate<endianness::NATIVE>(buf, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused bool
validate_utf16le(const char16_t *buf, size_t len) noexcept {
  return scalar::utf16::validate<endianness::LITTLE>(buf, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused bool
validate_utf16be(const char16_t *buf, size_t len) noexcept {
  return scalar::utf16::validate<endianness::BIG>(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF16
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
validate_utf16_with_errors(const char16_t *buf, size_t len) noexcept {
  return scalar::utf16::validate_with_errors<endianness::NATIVE>(buf, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
validate_utf16le_with_errors(const char16_t *buf, size_t len) noexcept {
  return scalar::utf16::validate_with_errors<endianness::LITTLE>(buf, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
validate_utf16be_with_errors(const char16_t *buf, size_t len) noexcept {
  return scalar::utf16::validate_with_errors<endianness::BIG>(buf, len);
}

simdutf_really_inline simdutf_constexpr23 void
to_well_formed_utf16(const char16_t *input, size_t len,
                     char16_t *output) noexcept {
  scalar::utf16::to_well_formed_utf16<endianness::NATIVE>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 void
to_well_formed_utf16le(const char16_t *input, size_t len,
                       char16_t *output) noexcept {
  scalar::utf16::to_well_formed_utf16<endianness::LITTLE>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 void
to_well_formed_utf16be(const char16_t *input, size_t len,
                       char16_t *output) noexcept {
  scalar::utf16::to_well_formed_utf16<endianness::BIG>(input, len, output);
}
#endif // SIMDUTF_FEATURE_UTF16

// -----------------------------------------------------------------------------
// UTF-32 validation
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused bool
validate_utf32(const char32_t *buf, size_t len) noexcept {
  #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf32::validate(
        detail::constexpr_cast_ptr<uint32_t>(input.data()), input.size());
  } else
  #endif
  {
    return scalar::utf32::validate(buf, len);
  }
}
#endif // SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF32
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
validate_utf32_with_errors(const char32_t *buf, size_t len) noexcept {
  #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf32::validate_with_errors(
        detail::constexpr_cast_ptr<uint32_t>(input.data()), input.size());
  } else
  #endif
  {
    return scalar::utf32::validate_with_errors(buf, len);
  }
}
#endif // SIMDUTF_FEATURE_UTF32

// -----------------------------------------------------------------------------
// Latin1 -> UTF-8 / UTF-16 / UTF-32
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_LATIN1 && SIMDUTF_FEATURE_UTF8
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_latin1_to_utf8(const char *input, size_t len, char *output) noexcept {
  return scalar::latin1_to_utf8::convert(input, len, output);
}
#endif // SIMDUTF_FEATURE_LATIN1 && SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_LATIN1 && SIMDUTF_FEATURE_UTF16
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_latin1_to_utf16(const char *input, size_t len,
                        char16_t *output) noexcept {
  return scalar::latin1_to_utf16::convert<endianness::NATIVE>(input, len,
                                                              output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_latin1_to_utf16le(const char *input, size_t len,
                          char16_t *output) noexcept {
  return scalar::latin1_to_utf16::convert<endianness::LITTLE>(input, len,
                                                              output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_latin1_to_utf16be(const char *input, size_t len,
                          char16_t *output) noexcept {
  return scalar::latin1_to_utf16::convert<endianness::BIG>(input, len, output);
}
#endif // SIMDUTF_FEATURE_LATIN1 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_LATIN1 && SIMDUTF_FEATURE_UTF32
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_latin1_to_utf32(const char *input, size_t len,
                        char32_t *output) noexcept {
  return scalar::latin1_to_utf32::convert(input, len, output);
}
#endif // SIMDUTF_FEATURE_LATIN1 && SIMDUTF_FEATURE_UTF32

// -----------------------------------------------------------------------------
// UTF-8 -> Latin1 / UTF-16 / UTF-32
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf8_to_latin1(const char *input, size_t len, char *output) noexcept {
  return scalar::utf8_to_latin1::convert(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf8_to_latin1_with_errors(const char *input, size_t len,
                                   char *output) noexcept {
  return scalar::utf8_to_latin1::convert_with_errors(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf8_to_latin1(const char *input, size_t len,
                             char *output) noexcept {
  return scalar::utf8_to_latin1::convert_valid(input, len, output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf8_to_utf16(const char *input, size_t len,
                      char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert<endianness::NATIVE>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf8_to_utf16le(const char *input, size_t len,
                        char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert<endianness::LITTLE>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf8_to_utf16be(const char *input, size_t len,
                        char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert<endianness::BIG>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf8_to_utf16_with_errors(const char *input, size_t len,
                                  char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert_with_errors<endianness::NATIVE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf8_to_utf16le_with_errors(const char *input, size_t len,
                                    char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert_with_errors<endianness::LITTLE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf8_to_utf16be_with_errors(const char *input, size_t len,
                                    char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert_with_errors<endianness::BIG>(input, len,
                                                                     output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf8_to_utf16(const char *input, size_t len,
                            char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert_valid<endianness::NATIVE>(input, len,
                                                                  output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf8_to_utf16le(const char *input, size_t len,
                              char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert_valid<endianness::LITTLE>(input, len,
                                                                  output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf8_to_utf16be(const char *input, size_t len,
                              char16_t *output) noexcept {
  return scalar::utf8_to_utf16::convert_valid<endianness::BIG>(input, len,
                                                               output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf8_to_utf32(const char *input, size_t len,
                      char32_t *output) noexcept {
  return scalar::utf8_to_utf32::convert(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf8_to_utf32_with_errors(const char *input, size_t len,
                                  char32_t *output) noexcept {
  return scalar::utf8_to_utf32::convert_with_errors(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf8_to_utf32(const char *input, size_t len,
                            char32_t *output) noexcept {
  return scalar::utf8_to_utf32::convert_valid(input, len, output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

// -----------------------------------------------------------------------------
// UTF-16 -> UTF-8 / Latin1 / UTF-32
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF8
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16_to_utf8(const char16_t *input, size_t len,
                      char *output) noexcept {
  return scalar::utf16_to_utf8::convert<endianness::NATIVE>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16le_to_utf8(const char16_t *input, size_t len,
                        char *output) noexcept {
  return scalar::utf16_to_utf8::convert<endianness::LITTLE>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16be_to_utf8(const char16_t *input, size_t len,
                        char *output) noexcept {
  return scalar::utf16_to_utf8::convert<endianness::BIG>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16_to_utf8_with_errors(const char16_t *input, size_t len,
                                  char *output) noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<endianness::NATIVE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16le_to_utf8_with_errors(const char16_t *input, size_t len,
                                    char *output) noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<endianness::LITTLE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16be_to_utf8_with_errors(const char16_t *input, size_t len,
                                    char *output) noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<endianness::BIG>(input, len,
                                                                     output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16_to_utf8(const char16_t *input, size_t len,
                            char *output) noexcept {
  return scalar::utf16_to_utf8::convert_valid<endianness::NATIVE>(input, len,
                                                                  output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16le_to_utf8(const char16_t *input, size_t len,
                              char *output) noexcept {
  return scalar::utf16_to_utf8::convert_valid<endianness::LITTLE>(input, len,
                                                                  output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16be_to_utf8(const char16_t *input, size_t len,
                              char *output) noexcept {
  return scalar::utf16_to_utf8::convert_valid<endianness::BIG>(input, len,
                                                               output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16_to_latin1(const char16_t *input, size_t len,
                        char *output) noexcept {
  return scalar::utf16_to_latin1::convert<endianness::NATIVE>(input, len,
                                                              output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16le_to_latin1(const char16_t *input, size_t len,
                          char *output) noexcept {
  return scalar::utf16_to_latin1::convert<endianness::LITTLE>(input, len,
                                                              output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16be_to_latin1(const char16_t *input, size_t len,
                          char *output) noexcept {
  return scalar::utf16_to_latin1::convert<endianness::BIG>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16_to_latin1_with_errors(const char16_t *input, size_t len,
                                    char *output) noexcept {
  return scalar::utf16_to_latin1::convert_with_errors<endianness::NATIVE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16le_to_latin1_with_errors(const char16_t *input, size_t len,
                                      char *output) noexcept {
  return scalar::utf16_to_latin1::convert_with_errors<endianness::LITTLE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16be_to_latin1_with_errors(const char16_t *input, size_t len,
                                      char *output) noexcept {
  return scalar::utf16_to_latin1::convert_with_errors<endianness::BIG>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16_to_latin1(const char16_t *input, size_t len,
                              char *output) noexcept {
  return scalar::utf16_to_latin1::convert_valid<endianness::NATIVE>(input, len,
                                                                    output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16le_to_latin1(const char16_t *input, size_t len,
                                char *output) noexcept {
  return scalar::utf16_to_latin1::convert_valid<endianness::LITTLE>(input, len,
                                                                    output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16be_to_latin1(const char16_t *input, size_t len,
                                char *output) noexcept {
  return scalar::utf16_to_latin1::convert_valid<endianness::BIG>(input, len,
                                                                 output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16_to_utf32(const char16_t *input, size_t len,
                       char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert<endianness::NATIVE>(input, len,
                                                             output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16le_to_utf32(const char16_t *input, size_t len,
                         char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert<endianness::LITTLE>(input, len,
                                                             output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf16be_to_utf32(const char16_t *input, size_t len,
                         char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert<endianness::BIG>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16_to_utf32_with_errors(const char16_t *input, size_t len,
                                   char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<endianness::NATIVE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16le_to_utf32_with_errors(const char16_t *input, size_t len,
                                     char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<endianness::LITTLE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf16be_to_utf32_with_errors(const char16_t *input, size_t len,
                                     char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<endianness::BIG>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16_to_utf32(const char16_t *input, size_t len,
                             char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert_valid<endianness::NATIVE>(input, len,
                                                                   output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16le_to_utf32(const char16_t *input, size_t len,
                               char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert_valid<endianness::LITTLE>(input, len,
                                                                   output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf16be_to_utf32(const char16_t *input, size_t len,
                               char32_t *output) noexcept {
  return scalar::utf16_to_utf32::convert_valid<endianness::BIG>(input, len,
                                                                output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

// -----------------------------------------------------------------------------
// UTF-32 -> UTF-8 / UTF-16 / Latin1
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_UTF8
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf32_to_utf8(const char32_t *input, size_t len,
                      char *output) noexcept {
  return scalar::utf32_to_utf8::convert(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf32_to_utf8_with_errors(const char32_t *input, size_t len,
                                  char *output) noexcept {
  return scalar::utf32_to_utf8::convert_with_errors(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf32_to_utf8(const char32_t *input, size_t len,
                            char *output) noexcept {
  return scalar::utf32_to_utf8::convert_valid(input, len, output);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_UTF16
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf32_to_utf16(const char32_t *input, size_t len,
                       char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert<endianness::NATIVE>(input, len,
                                                             output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf32_to_utf16le(const char32_t *input, size_t len,
                         char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert<endianness::LITTLE>(input, len,
                                                             output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf32_to_utf16be(const char32_t *input, size_t len,
                         char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert<endianness::BIG>(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf32_to_utf16_with_errors(const char32_t *input, size_t len,
                                   char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::NATIVE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf32_to_utf16le_with_errors(const char32_t *input, size_t len,
                                     char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::LITTLE>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf32_to_utf16be_with_errors(const char32_t *input, size_t len,
                                     char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::BIG>(
      input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf32_to_utf16(const char32_t *input, size_t len,
                             char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert_valid<endianness::NATIVE>(input, len,
                                                                   output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf32_to_utf16le(const char32_t *input, size_t len,
                               char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert_valid<endianness::LITTLE>(input, len,
                                                                   output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf32_to_utf16be(const char32_t *input, size_t len,
                               char16_t *output) noexcept {
  return scalar::utf32_to_utf16::convert_valid<endianness::BIG>(input, len,
                                                                output);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_utf32_to_latin1(const char32_t *input, size_t len,
                        char *output) noexcept {
  return scalar::utf32_to_latin1::convert(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
convert_utf32_to_latin1_with_errors(const char32_t *input, size_t len,
                                    char *output) noexcept {
  return scalar::utf32_to_latin1::convert_with_errors(input, len, output);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
convert_valid_utf32_to_latin1(const char32_t *input, size_t len,
                              char *output) noexcept {
  return scalar::utf32_to_latin1::convert_valid(input, len, output);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

// -----------------------------------------------------------------------------
// Counting and length-from helpers
// -----------------------------------------------------------------------------
#if SIMDUTF_FEATURE_UTF8
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
count_utf8(const char *input, size_t len) noexcept {
  return scalar::utf8::count_code_points(input, len);
}
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF16
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
count_utf16(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::count_code_points<endianness::NATIVE>(input, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
count_utf16le(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::count_code_points<endianness::LITTLE>(input, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
count_utf16be(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::count_code_points<endianness::BIG>(input, len);
}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf8_length_from_latin1(const char *input, size_t len) noexcept {
  return scalar::latin1_to_utf8::utf8_length_from_latin1(input, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
latin1_length_from_utf8(const char *input, size_t len) noexcept {
  return scalar::utf8::count_code_points(input, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf16_length_from_utf8(const char *input, size_t len) noexcept {
  return scalar::utf8::utf16_length_from_utf8(input, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf32_length_from_utf8(const char *input, size_t len) noexcept {
  return scalar::utf8::count_code_points(input, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf8_length_from_utf16(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::utf8_length_from_utf16<endianness::NATIVE>(input, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf8_length_from_utf16le(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::utf8_length_from_utf16<endianness::LITTLE>(input, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf8_length_from_utf16be(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::utf8_length_from_utf16<endianness::BIG>(input, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf32_length_from_utf16(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::utf32_length_from_utf16<endianness::NATIVE>(input, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf32_length_from_utf16le(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::utf32_length_from_utf16<endianness::LITTLE>(input, len);
}

simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf32_length_from_utf16be(const char16_t *input, size_t len) noexcept {
  return scalar::utf16::utf32_length_from_utf16<endianness::BIG>(input, len);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_UTF8
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf8_length_from_utf32(const char32_t *input, size_t len) noexcept {
  return scalar::utf32::utf8_length_from_utf32(input, len);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_UTF16
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused size_t
utf16_length_from_utf32(const char32_t *input, size_t len) noexcept {
  return scalar::utf32::utf16_length_from_utf32(input, len);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_UTF16

} // namespace inlineable
} // namespace simdutf

#endif // SIMDUTF_INLINEABLE_H
