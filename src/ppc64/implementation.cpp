#include "simdutf/ppc64/begin.h"

#include "ppc64_utf16_to_utf8_tables.h"

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
#ifndef SIMDUTF_PPC64_H
  #error "ppc64.h must be included"
#endif
using namespace simd;

simdutf_really_inline bool is_ascii(const simd8x64<uint8_t> &input) {
  // careful: 0x80 is not ascii.
  return input.reduce_or().saturating_sub(0b01111111u).bits_not_set_anywhere();
}

simdutf_really_inline simd8<bool>
must_be_2_3_continuation(const simd8<uint8_t> prev2,
                         const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte =
      prev2.saturating_sub(0xe0u - 0x80); // Only 111_____ will be >= 0x80
  simd8<uint8_t> is_fourth_byte =
      prev3.saturating_sub(0xf0u - 0x80); // Only 1111____ will be >= 0x80
  // Caller requires a bool (all 1's). All values resulting from the subtraction
  // will be <= 64, so signed comparison is fine.
  return simd8<bool>(is_third_byte | is_fourth_byte);
}

#include "ppc64_validate_utf16.cpp"
#include "ppc64_convert_latin1_to_utf8.cpp"
#include "ppc64_convert_latin1_to_utf16.cpp"
#include "ppc64_convert_latin1_to_utf32.cpp"
#include "ppc64_convert_utf8_to_latin1.cpp"
#include "ppc64_convert_utf8_to_utf16.cpp"
#include "ppc64_convert_utf8_to_utf32.cpp"
#include "ppc64_convert_utf16_to_latin1.cpp"
#include "ppc64_convert_utf16_to_utf8.cpp"
#include "ppc64_convert_utf16_to_utf32.cpp"
#include "ppc64_convert_utf32_to_latin1.cpp"
#include "ppc64_convert_utf32_to_utf16.cpp"
#include "ppc64_convert_utf32_to_utf8.cpp"
#include "ppc64_utf8_length_from_latin1.cpp"

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "generic/buf_block_reader.h"
#include "generic/utf8_validation/utf8_lookup4_algorithm.h"
#include "generic/utf8_validation/utf8_validator.h"
// transcoding from UTF-8 to UTF-16
#include "generic/utf8_to_utf16/utf8_to_utf16.h"
#include "generic/utf8_to_utf16/valid_utf8_to_utf16.h"
// transcoding from UTF-8 to UTF-32
#include "generic/utf8_to_utf32/utf8_to_utf32.h"
#include "generic/utf8_to_utf32/valid_utf8_to_utf32.h"
// other functions
#include "generic/utf16.h"
#include "generic/validate_utf16.h"
#include "generic/utf8.h"
#include "generic/validate_utf32.h"
#include "generic/ascii_validation.h"
#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  #include "generic/utf8_to_latin1/utf8_to_latin1.h"
  #include "generic/utf8_to_latin1/valid_utf8_to_latin1.h"
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

//
// Implementation-specific overrides
//
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

#if SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused int
implementation::detect_encodings(const char *input,
                                 size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = simdutf::BOM::check_bom(input, length);
  if (bom_encoding != encoding_type::unspecified) {
    return bom_encoding;
  }
  int out = 0;
  // todo: reimplement as a one-pass algorithm.
  if (validate_utf8(input, length)) {
    out |= encoding_type::UTF8;
  }
  if ((length % 2) == 0) {
    if (validate_utf16le(reinterpret_cast<const char16_t *>(input),
                         length / 2)) {
      out |= encoding_type::UTF16_LE;
    }
  }
  if ((length % 4) == 0) {
    if (validate_utf32(reinterpret_cast<const char32_t *>(input), length / 4)) {
      out |= encoding_type::UTF32_LE;
    }
  }
  return out;
}
#endif // SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused bool
implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return ppc64::utf8_validation::generic_validate_utf8(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8
simdutf_warn_unused result implementation::validate_utf8_with_errors(
    const char *buf, size_t len) const noexcept {
  return ppc64::utf8_validation::generic_validate_utf8_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_ASCII
simdutf_warn_unused bool
implementation::validate_ascii(const char *buf, size_t len) const noexcept {
  return ppc64::ascii_validation::generic_validate_ascii(buf, len);
}

simdutf_warn_unused result implementation::validate_ascii_with_errors(
    const char *buf, size_t len) const noexcept {
  return ppc64::ascii_validation::generic_validate_ascii_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_ASCII

#if SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused bool
implementation::validate_utf16le(const char16_t *buf,
                                 size_t len) const noexcept {
  const auto res =
      ppc64::utf16::validate_utf16_with_errors<endianness::LITTLE>(buf, len);
  if (res.is_err()) {
    return false;
  }

  if (res.count != len) {
    return scalar::utf16::validate<endianness::LITTLE>(buf + res.count,
                                                       len - res.count);
  }

  return true;
}
#endif // SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF16
simdutf_warn_unused bool
implementation::validate_utf16be(const char16_t *buf,
                                 size_t len) const noexcept {
  return validate_utf16be_with_errors(buf, len).is_ok();
}

simdutf_warn_unused result implementation::validate_utf16le_with_errors(
    const char16_t *buf, size_t len) const noexcept {
  const auto res =
      ppc64::utf16::validate_utf16_with_errors<endianness::LITTLE>(buf, len);
  if (res.is_ok() and res.count != len) {
    auto scalar = scalar::utf16::validate_with_errors<endianness::LITTLE>(
        buf + res.count, len - res.count);
    scalar.count += res.count;
    return scalar;
  }

  return res;
}

simdutf_warn_unused result implementation::validate_utf16be_with_errors(
    const char16_t *buf, size_t len) const noexcept {
  const auto res =
      ppc64::utf16::validate_utf16_with_errors<endianness::BIG>(buf, len);
  if (res.is_ok() and res.count != len) {
    auto scalar = scalar::utf16::validate_with_errors<endianness::BIG>(
        buf + res.count, len - res.count);
    scalar.count += res.count;
    return scalar;
  }

  return res;
}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused bool
implementation::validate_utf32(const char32_t *buf, size_t len) const noexcept {
  const char32_t *ptr = ppc64::utf32::validate_utf32(buf, len);
  if (ptr == nullptr) {
    return false;
  }

  return scalar::utf32::validate(ptr, len - (ptr - buf));
}
#endif // SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF32
simdutf_warn_unused result implementation::validate_utf32_with_errors(
    const char32_t *buf, size_t len) const noexcept {
  const auto res = ppc64::utf32::validate_utf32_with_errors(buf, len);
  if (res.is_err() or res.count != len) {
    auto scalar =
        scalar::utf32::validate_with_errors(buf + res.count, len - res.count);
    scalar.count += res.count;
    return scalar;
  }

  return res;
}
#endif // SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_latin1_to_utf8(
    const char *buf, size_t len, char *utf8_output) const noexcept {
  const auto ret = ppc64_convert_latin1_to_utf8(buf, len, utf8_output);
  size_t converted_chars = ret.second - utf8_output;

  if (ret.first != buf + len) {
    const size_t scalar_converted_chars = scalar::latin1_to_utf8::convert(
        ret.first, len - (ret.first - buf), ret.second);
    converted_chars += scalar_converted_chars;
  }

  return converted_chars;
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_latin1_to_utf16le(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  size_t n =
      ppc64_convert_latin1_to_utf16<endianness::LITTLE>(buf, len, utf16_output);
  if (n < len) {
    n += scalar::latin1_to_utf16::convert<endianness::LITTLE>(buf + n, len - n,
                                                              utf16_output + n);
  }

  return n;
}

simdutf_warn_unused size_t implementation::convert_latin1_to_utf16be(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  size_t n =
      ppc64_convert_latin1_to_utf16<endianness::BIG>(buf, len, utf16_output);
  if (n < len) {
    n += scalar::latin1_to_utf16::convert<endianness::BIG>(buf + n, len - n,
                                                           utf16_output + n);
  }

  return n;
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_latin1_to_utf32(
    const char *buf, size_t len, char32_t *utf32_output) const noexcept {
  const auto ret = ppc64_convert_latin1_to_utf32(buf, len, utf32_output);
  if (ret.first != buf + len) {
    const size_t processed = ret.first - buf;
    scalar::latin1_to_utf32::convert(ret.first, len - processed, ret.second);
  }

  return len;
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_utf8_to_latin1(
    const char *buf, size_t len, char *latin1_output) const noexcept {
  utf8_to_latin1::validating_transcoder converter;
  return converter.convert(buf, len, latin1_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_latin1_with_errors(
    const char *buf, size_t len, char *latin1_output) const noexcept {
  utf8_to_latin1::validating_transcoder converter;
  return converter.convert_with_errors(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_latin1(
    const char *buf, size_t len, char *latin1_output) const noexcept {
  return ppc64::utf8_to_latin1::convert_valid(buf, len, latin1_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t implementation::convert_utf8_to_utf16le(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert<endianness::LITTLE>(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16be(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert<endianness::BIG>(buf, len, utf16_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_utf16le_with_errors(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert_with_errors<endianness::LITTLE>(buf, len,
                                                           utf16_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_utf16be_with_errors(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert_with_errors<endianness::BIG>(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16le(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return utf8_to_utf16::convert_valid<endianness::LITTLE>(buf, len,
                                                          utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16be(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return utf8_to_utf16::convert_valid<endianness::BIG>(buf, len, utf16_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::convert_utf8_to_utf32(
    const char *buf, size_t len, char32_t *utf32_output) const noexcept {
  utf8_to_utf32::validating_transcoder converter;
  return converter.convert(buf, len, utf32_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_utf32_with_errors(
    const char *buf, size_t len, char32_t *utf32_output) const noexcept {
  utf8_to_utf32::validating_transcoder converter;
  return converter.convert_with_errors(buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf32(
    const char *input, size_t size, char32_t *utf32_output) const noexcept {
  return utf8_to_utf32::convert_valid(input, size, utf32_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_utf16le_to_latin1(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  std::pair<const char16_t *, char *> ret =
      ppc64_convert_utf16_to_latin1<endianness::LITTLE>(buf, len,
                                                        latin1_output);
  if (ret.first == nullptr) {
    return 0;
  }
  size_t saved_bytes = ret.second - latin1_output;

  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes =
        scalar::utf16_to_latin1::convert<endianness::LITTLE>(
            ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) {
      return 0;
    }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_latin1(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  std::pair<const char16_t *, char *> ret =
      ppc64_convert_utf16_to_latin1<endianness::BIG>(buf, len, latin1_output);
  if (ret.first == nullptr) {
    return 0;
  }
  size_t saved_bytes = ret.second - latin1_output;

  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes =
        scalar::utf16_to_latin1::convert<endianness::BIG>(
            ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) {
      return 0;
    }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused result
implementation::convert_utf16le_to_latin1_with_errors(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  std::pair<result, char *> ret =
      ppc64_convert_utf16_to_latin1_with_errors<endianness::LITTLE>(
          buf, len, latin1_output);
  if (ret.first.error) {
    return ret.first;
  } // Can return directly since scalar fallback already found correct
    // ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res =
        scalar::utf16_to_latin1::convert_with_errors<endianness::LITTLE>(
            buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count =
      ret.second -
      latin1_output; // Set count to the number of 8-bit code units written
  return ret.first;
}

simdutf_warn_unused result
implementation::convert_utf16be_to_latin1_with_errors(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  std::pair<result, char *> ret =
      ppc64_convert_utf16_to_latin1_with_errors<endianness::BIG>(buf, len,
                                                                 latin1_output);
  if (ret.first.error) {
    return ret.first;
  } // Can return directly since scalar fallback already found correct
    // ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res =
        scalar::utf16_to_latin1::convert_with_errors<endianness::BIG>(
            buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count =
      ret.second -
      latin1_output; // Set count to the number of 8-bit code units written
  return ret.first;
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_latin1(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  // optimization opportunity: we could provide an optimized function.
  return convert_utf16be_to_latin1(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_latin1(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  // optimization opportunity: we could provide an optimized function.
  return convert_utf16le_to_latin1(buf, len, latin1_output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t implementation::convert_utf16le_to_utf8(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  std::pair<const char16_t *, char *> ret =
      ppc64_convert_utf16_to_utf8<endianness::LITTLE>(buf, len, utf8_output);
  if (ret.first == nullptr) {
    return 0;
  }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes =
        scalar::utf16_to_utf8::convert<endianness::LITTLE>(
            ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) {
      return 0;
    }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_utf8(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  std::pair<const char16_t *, char *> ret =
      ppc64_convert_utf16_to_utf8<endianness::BIG>(buf, len, utf8_output);
  if (ret.first == nullptr) {
    return 0;
  }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes =
        scalar::utf16_to_utf8::convert<endianness::BIG>(
            ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) {
      return 0;
    }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused result implementation::convert_utf16le_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of
  // code units written even if finished
  std::pair<result, char *> ret =
      ppc64_convert_utf16_to_utf8_with_errors<endianness::LITTLE>(buf, len,
                                                                  utf8_output);
  if (ret.first.error) {
    return ret.first;
  } // Can return directly since scalar fallback already found correct
    // ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res =
        scalar::utf16_to_utf8::convert_with_errors<endianness::LITTLE>(
            buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count =
      ret.second -
      utf8_output; // Set count to the number of 8-bit code units written
  return ret.first;
}

simdutf_warn_unused result implementation::convert_utf16be_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of
  // code units written even if finished
  std::pair<result, char *> ret =
      ppc64_convert_utf16_to_utf8_with_errors<endianness::BIG>(buf, len,
                                                               utf8_output);
  if (ret.first.error) {
    return ret.first;
  } // Can return directly since scalar fallback already found correct
    // ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res =
        scalar::utf16_to_utf8::convert_with_errors<endianness::BIG>(
            buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count =
      ret.second -
      utf8_output; // Set count to the number of 8-bit code units written
  return ret.first;
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_utf8(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  return convert_utf16le_to_utf8(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_utf8(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  return convert_utf16be_to_utf8(buf, len, utf8_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

template <ErrorChecking ec>
size_t convert_utf32_to_latin1_impl(const char32_t *buf, size_t len,
                                    char *latin1_output) {
  const auto ret = ppc64_convert_utf32_to_latin1<ec>(buf, len, latin1_output);

  const size_t consumed = ret.first - buf;
  if (consumed == len) {
    return consumed; // Note: we have 1:1 mapping
  }

  const size_t scalar_consumed =
      scalar::utf32_to_latin1::convert(ret.first, len - consumed, ret.second);
  if (scalar_consumed == 0) {
    return 0;
  }

  return scalar_consumed + consumed;
}

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_utf32_to_latin1(
    const char32_t *buf, size_t len, char *latin1_output) const noexcept {
  return convert_utf32_to_latin1_impl<ErrorChecking::enabled>(buf, len,
                                                              latin1_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_latin1_with_errors(
    const char32_t *buf, size_t len, char *latin1_output) const noexcept {
  const auto ret = ppc64_convert_utf32_to_latin1<ErrorChecking::enabled>(
      buf, len, latin1_output);

  const size_t consumed = ret.first - buf;
  if (consumed == len) {
    return result(error_code::SUCCESS, consumed);
  }

  auto scalar_ret = scalar::utf32_to_latin1::convert_with_errors(
      ret.first, len - consumed, ret.second);
  scalar_ret.count += consumed;

  return scalar_ret;
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_latin1(
    const char32_t *buf, size_t len, char *latin1_output) const noexcept {
  return convert_utf32_to_latin1_impl<ErrorChecking::disabled>(buf, len,
                                                               latin1_output);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
template <typename VectorizedConvert, typename ScalarConvert, typename Source,
          typename Destination>
size_t convert_impl(VectorizedConvert vectorized_convert,
                    ScalarConvert scalar_convert, const Source *buf, size_t len,
                    Destination *output) {
  const auto vr = vectorized_convert(buf, len, output);
  const size_t consumed = vr.input - buf;
  const size_t written = vr.output - output;
  if (vr.err != error_code::SUCCESS) {
    return 0;
  }

  if (consumed == len) {
    return written;
  }

  const auto ret = scalar_convert(vr.input, len - consumed, vr.output);
  if (ret == 0) {
    return 0;
  }

  return written + ret;
}

template <typename VectorizedConvert, typename ScalarConvert, typename Source,
          typename Destination>
result convert_with_errors_impl(VectorizedConvert vectorized_convert,
                                ScalarConvert scalar_convert, const Source *buf,
                                size_t len, Destination *output) {
  const auto vr = vectorized_convert(buf, len, output);
  const size_t consumed = vr.input - buf;
  const size_t written = vr.output - output;
  if (vr.err != error_code::SUCCESS) {
    return result(vr.err, consumed);
  }

  if (consumed == len) {
    return result(error_code::SUCCESS, written);
  }

  result sr = scalar_convert(vr.input, len - consumed, vr.output);
  if (sr.is_ok()) {
    sr.count += written;
  } else {
    sr.count += consumed;
  }

  return sr;
}

simdutf_warn_unused size_t implementation::convert_utf32_to_utf8(
    const char32_t *buf, size_t len, char *utf8_output) const noexcept {
  return convert_impl(ppc64_convert_utf32_to_utf8<ErrorReporting::at_the_end>,
                      scalar::utf32_to_utf8::convert, buf, len, utf8_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_utf8_with_errors(
    const char32_t *buf, size_t len, char *utf8_output) const noexcept {
  return convert_with_errors_impl(
      ppc64_convert_utf32_to_utf8<ErrorReporting::precise>,
      scalar::utf32_to_utf8::convert_with_errors, buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf8(
    const char32_t *buf, size_t len, char *utf8_output) const noexcept {
  return convert_impl(ppc64_convert_utf32_to_utf8<ErrorReporting::none>,
                      scalar::utf32_to_utf8::convert, buf, len, utf8_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
template <endianness endian, ErrorReporting er>
size_t convert_utf32_to_utf16_impl(const char32_t *buf, size_t len,
                                   char16_t *utf16_output) {
  const auto ret =
      ppc64_convert_utf32_to_utf16<endian, er>(buf, len, utf16_output);

  const size_t consumed = ret.input - buf;
  const size_t written = ret.output - utf16_output;
  if (er == ErrorReporting::at_the_end and ret.err != error_code::SUCCESS) {
    return 0;
  }

  if (consumed == len) {
    return written; // no error detected
  }

  const auto scalar_written = scalar::utf32_to_utf16::convert<endian>(
      ret.input, len - consumed, ret.output);
  if (scalar_written == 0) {
    return 0;
  }

  return scalar_written + written;
}

template <endianness endian>
result convert_utf32_to_utf16_with_errors_impl(const char32_t *buf, size_t len,
                                               char16_t *utf16_output) {
  const auto ret =
      ppc64_convert_utf32_to_utf16<endian, ErrorReporting::precise>(
          buf, len, utf16_output);

  const size_t consumed = ret.input - buf;
  const size_t written = ret.output - utf16_output;
  if (consumed == len) {
    return result(error_code::SUCCESS, consumed);
  }

  auto scalar_ret = scalar::utf32_to_utf16::convert_with_errors<endian>(
      ret.input, len - consumed, ret.output);
  if (ret.is_ok()) {
    scalar_ret.count += written;
  } else {
    scalar_ret.count += consumed;
  }

  return scalar_ret;
}

simdutf_warn_unused size_t implementation::convert_utf32_to_utf16le(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return convert_utf32_to_utf16_impl<endianness::LITTLE,
                                     ErrorReporting::at_the_end>(buf, len,
                                                                 utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf32_to_utf16be(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return convert_utf32_to_utf16_impl<endianness::BIG,
                                     ErrorReporting::at_the_end>(buf, len,
                                                                 utf16_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_utf16le_with_errors(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::LITTLE>(
      buf, len, utf16_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_utf16be_with_errors(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::BIG>(
      buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf16le(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return convert_utf32_to_utf16_impl<endianness::LITTLE, ErrorReporting::none>(
      buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf16be(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return convert_utf32_to_utf16_impl<endianness::BIG, ErrorReporting::none>(
      buf, len, utf16_output);
  return scalar::utf32_to_utf16::convert_valid<endianness::BIG>(buf, len,
                                                                utf16_output);
}

template <endianness endian>
size_t convert_utf16_to_utf32(const char16_t *buf, size_t len,
                              char32_t *utf32_output) {
  const auto ret = ppc64_convert_utf16_to_utf32<endian>(buf, len, utf32_output);
  if (ret.err != error_code::SUCCESS) {
    return 0;
  }

  const size_t saved_chars = ret.output - utf32_output;
  if (buf + len == ret.input) {
    return saved_chars;
  }

  const size_t saved_chars_scalar = scalar::utf16_to_utf32::convert<endian>(
      ret.input, len - (ret.input - buf), ret.output);
  if (saved_chars_scalar == 0) {
    return 0;
  }

  return saved_chars + saved_chars_scalar;
}

simdutf_warn_unused size_t implementation::convert_utf16le_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return convert_utf16_to_utf32<endianness::LITTLE>(buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return convert_utf16_to_utf32<endianness::BIG>(buf, len, utf32_output);
}

template <endianness endian>
result convert_utf16_to_utf32_with_errors(const char16_t *buf, size_t len,
                                          char32_t *utf32_output) {
  const auto ret = ppc64_convert_utf16_to_utf32<endian>(buf, len, utf32_output);

  const size_t chars_consumed = ret.input - buf;
  if (ret.err != error_code::SUCCESS) {
    return result(ret.err, chars_consumed);
  }

  const size_t saved_chars = ret.output - utf32_output;
  if (buf + len == ret.input) {
    return result(error_code::SUCCESS, saved_chars);
  }

  auto scalar_ret = scalar::utf16_to_utf32::convert_with_errors<endian>(
      ret.input, len - chars_consumed, ret.output);

  if (scalar_ret.is_err()) {
    scalar_ret.count += chars_consumed;
  } else {
    scalar_ret.count += saved_chars;
  }

  return scalar_ret;
}

simdutf_warn_unused result implementation::convert_utf16le_to_utf32_with_errors(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return convert_utf16_to_utf32_with_errors<endianness::LITTLE>(buf, len,
                                                                utf32_output);
}

simdutf_warn_unused result implementation::convert_utf16be_to_utf32_with_errors(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return convert_utf16_to_utf32_with_errors<endianness::BIG>(buf, len,
                                                             utf32_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return convert_utf16le_to_utf32(buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return convert_utf16be_to_utf32(buf, len, utf32_output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16
void implementation::change_endianness_utf16(const char16_t *input,
                                             size_t length,
                                             char16_t *output) const noexcept {
  utf16::change_endianness_utf16(input, length, output);
}

simdutf_warn_unused size_t implementation::count_utf16le(
    const char16_t *input, size_t length) const noexcept {
  return utf16::count_code_points<endianness::LITTLE>(input, length);
}

simdutf_warn_unused size_t implementation::count_utf16be(
    const char16_t *input, size_t length) const noexcept {
  return utf16::count_code_points<endianness::BIG>(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8
simdutf_warn_unused size_t
implementation::count_utf8(const char *input, size_t length) const noexcept {
  return utf8::count_code_points(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::latin1_length_from_utf8(
    const char *buf, size_t len) const noexcept {
  return count_utf8(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::utf8_length_from_latin1(
    const char *input, size_t length) const noexcept {
  const auto ret = ppc64_utf8_length_from_latin1(input, length);
  const size_t consumed = ret.first - input;

  if (consumed == length) {
    return ret.second;
  }

  const auto scalar =
      scalar::latin1::utf8_length_from_latin1(ret.first, length - consumed);
  return scalar + ret.second;
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t implementation::utf8_length_from_utf16le(
    const char16_t *input, size_t length) const noexcept {
  return utf16::utf8_length_from_utf16<endianness::LITTLE>(input, length);
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf16be(
    const char16_t *input, size_t length) const noexcept {
  return utf16::utf8_length_from_utf16<endianness::BIG>(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::utf32_length_from_utf16le(
    const char16_t *input, size_t length) const noexcept {
  return utf16::utf32_length_from_utf16<endianness::LITTLE>(input, length);
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf16be(
    const char16_t *input, size_t length) const noexcept {
  return utf16::utf32_length_from_utf16<endianness::BIG>(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t implementation::utf16_length_from_utf8(
    const char *input, size_t length) const noexcept {
  return utf8::utf16_length_from_utf8(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::utf8_length_from_utf32(
    const char32_t *input, size_t length) const noexcept {
  return scalar::utf32::utf8_length_from_utf32(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::utf16_length_from_utf32(
    const char32_t *input, size_t length) const noexcept {
  return scalar::utf32::utf16_length_from_utf32(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::utf32_length_from_utf8(
    const char *input, size_t length) const noexcept {
  return utf8::count_code_points(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_BASE64
simdutf_warn_unused size_t implementation::maximal_binary_length_from_base64(
    const char *input, size_t length) const noexcept {
  return scalar::base64::maximal_binary_length_from_base64(input, length);
}

simdutf_warn_unused result implementation::base64_to_binary(
    const char *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage);
  while (length > 0 &&
         scalar::base64::is_ascii_white_space(input[length - 1])) {
    length--;
  }
  size_t equallocation =
      length; // location of the first padding character if any
  size_t equalsigns = 0;
  if (length > 0 && input[length - 1] == '=') {
    equallocation = length - 1;
    length -= 1;
    equalsigns++;
    while (length > 0 &&
           scalar::base64::is_ascii_white_space(input[length - 1])) {
      length--;
    }
    if (length > 0 && input[length - 1] == '=') {
      equallocation = length - 1;
      equalsigns++;
      length -= 1;
    }
  }
  if (length == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      if (last_chunk_options == last_chunk_handling_options::strict) {
        return {BASE64_INPUT_REMAINDER, 0};
      } else if (last_chunk_options ==
                 last_chunk_handling_options::stop_before_partial) {
        return {SUCCESS, 0};
      }
      return {INVALID_BASE64_CHARACTER, equallocation};
    }
    return {SUCCESS, 0};
  }
  result r = scalar::base64::base64_tail_decode(
      output, input, length, equalsigns, options, last_chunk_options);
  if (last_chunk_options != stop_before_partial &&
      r.error == error_code::SUCCESS && equalsigns > 0 && !ignore_garbage) {
    // additional checks
    if ((r.count % 3 == 0) || ((r.count % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation};
    }
  }
  return r;
}

simdutf_warn_unused full_result implementation::base64_to_binary_details(
    const char *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage);
  while (length > 0 &&
         scalar::base64::is_ascii_white_space(input[length - 1])) {
    length--;
  }
  size_t equallocation =
      length; // location of the first padding character if any
  size_t equalsigns = 0;
  if (length > 0 && input[length - 1] == '=') {
    equallocation = length - 1;
    length -= 1;
    equalsigns++;
    while (length > 0 &&
           scalar::base64::is_ascii_white_space(input[length - 1])) {
      length--;
    }
    if (length > 0 && input[length - 1] == '=') {
      equallocation = length - 1;
      equalsigns++;
      length -= 1;
    }
  }
  if (length == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      if (last_chunk_options == last_chunk_handling_options::strict) {
        return {BASE64_INPUT_REMAINDER, 0, 0};
      } else if (last_chunk_options ==
                 last_chunk_handling_options::stop_before_partial) {
        return {SUCCESS, 0, 0};
      }
      return {INVALID_BASE64_CHARACTER, equallocation, 0};
    }
    return {SUCCESS, 0, 0};
  }
  full_result r = scalar::base64::base64_tail_decode(
      output, input, length, equalsigns, options, last_chunk_options);
  if (last_chunk_options != stop_before_partial &&
      r.error == error_code::SUCCESS && equalsigns > 0 && !ignore_garbage) {
    // additional checks
    if ((r.output_count % 3 == 0) ||
        ((r.output_count % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation, r.output_count};
    }
  }
  return r;
}

simdutf_warn_unused size_t implementation::maximal_binary_length_from_base64(
    const char16_t *input, size_t length) const noexcept {
  return scalar::base64::maximal_binary_length_from_base64(input, length);
}

simdutf_warn_unused result implementation::base64_to_binary(
    const char16_t *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage);
  while (length > 0 &&
         scalar::base64::is_ascii_white_space(input[length - 1])) {
    length--;
  }
  size_t equallocation =
      length; // location of the first padding character if any
  size_t equalsigns = 0;
  if (length > 0 && input[length - 1] == '=') {
    equallocation = length - 1;
    length -= 1;
    equalsigns++;
    while (length > 0 &&
           scalar::base64::is_ascii_white_space(input[length - 1])) {
      length--;
    }
    if (length > 0 && input[length - 1] == '=') {
      equallocation = length - 1;
      equalsigns++;
      length -= 1;
    }
  }
  if (length == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      if (last_chunk_options == last_chunk_handling_options::strict) {
        return {BASE64_INPUT_REMAINDER, 0};
      } else if (last_chunk_options ==
                 last_chunk_handling_options::stop_before_partial) {
        return {SUCCESS, 0};
      }
      return {INVALID_BASE64_CHARACTER, equallocation};
    }
    return {SUCCESS, 0};
  }
  result r = scalar::base64::base64_tail_decode(
      output, input, length, equalsigns, options, last_chunk_options);
  if (last_chunk_options != stop_before_partial &&
      r.error == error_code::SUCCESS && equalsigns > 0 && !ignore_garbage) {
    // additional checks
    if ((r.count % 3 == 0) || ((r.count % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation};
    }
  }
  return r;
}

simdutf_warn_unused full_result implementation::base64_to_binary_details(
    const char16_t *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage);
  while (length > 0 &&
         scalar::base64::is_ascii_white_space(input[length - 1])) {
    length--;
  }
  size_t equallocation =
      length; // location of the first padding character if any
  size_t equalsigns = 0;
  if (length > 0 && input[length - 1] == '=') {
    equallocation = length - 1;
    length -= 1;
    equalsigns++;
    while (length > 0 &&
           scalar::base64::is_ascii_white_space(input[length - 1])) {
      length--;
    }
    if (length > 0 && input[length - 1] == '=') {
      equallocation = length - 1;
      equalsigns++;
      length -= 1;
    }
  }
  if (length == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      if (last_chunk_options == last_chunk_handling_options::strict) {
        return {BASE64_INPUT_REMAINDER, 0, 0};
      } else if (last_chunk_options ==
                 last_chunk_handling_options::stop_before_partial) {
        return {SUCCESS, 0, 0};
      }
      return {INVALID_BASE64_CHARACTER, equallocation, 0};
    }
    return {SUCCESS, 0, 0};
  }
  full_result r = scalar::base64::base64_tail_decode(
      output, input, length, equalsigns, options, last_chunk_options);
  if (last_chunk_options != stop_before_partial &&
      r.error == error_code::SUCCESS && equalsigns > 0 && !ignore_garbage) {
    // additional checks
    if ((r.output_count % 3 == 0) ||
        ((r.output_count % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation, r.output_count};
    }
  }
  return r;
}

simdutf_warn_unused size_t implementation::base64_length_from_binary(
    size_t length, base64_options options) const noexcept {
  return scalar::base64::base64_length_from_binary(length, options);
}

size_t implementation::binary_to_base64(const char *input, size_t length,
                                        char *output,
                                        base64_options options) const noexcept {
  return scalar::base64::tail_encode_base64(output, input, length, options);
}
#endif // SIMDUTF_FEATURE_BASE64

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/ppc64/end.h"
