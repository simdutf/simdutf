// Included from implementation.cpp, inside namespace simdutf.
#ifndef SIMDUTF_UTF8_TO_UTF16_WITH_REPLACEMENT_INL_H
#define SIMDUTF_UTF8_TO_UTF16_WITH_REPLACEMENT_INL_H

namespace {
constexpr bool is_utf8_validation_error(error_code code) noexcept {
  switch (code) {
  case error_code::HEADER_BITS:
  case error_code::TOO_SHORT:
  case error_code::TOO_LONG:
  case error_code::OVERLONG:
  case error_code::TOO_LARGE:
  case error_code::SURROGATE:
    return true;
  default:
    return false;
  }
}

// Valid prefixes shorter than this are cheaper to finish in the scalar decoder
// than to rediscover with another validating scan. Two in a row means the tail
// is dense.
constexpr size_t dense_prefix_limit = 512;

template <endianness endian>
simdutf_really_inline size_t convert_valid_prefix(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  if constexpr (endian == endianness::LITTLE) {
    return convert_valid_utf8_to_utf16le(input, length, utf16_output);
  } else {
    return convert_valid_utf8_to_utf16be(input, length, utf16_output);
  }
}

template <endianness endian, bool write>
simdutf_really_inline result
finish_with_scalar(const char *input, size_t length, size_t pos, size_t written,
                   error_code first, char16_t *utf16_output) noexcept {
  if (first == error_code::SUCCESS && pos < length) {
    const utf8_result scalar_validation =
        scalar::utf8::validate_with_counts(input + pos, length - pos);
    if (scalar_validation.error != error_code::SUCCESS) {
      first = scalar_validation.error;
    }
  }
  if constexpr (write) {
    written += scalar::utf8_to_utf16::convert_with_replacement<endian>(
        input + pos, length - pos, utf16_output + written);
  } else {
    written += scalar::utf8_to_utf16::count_with_replacement(input + pos,
                                                             length - pos);
  }
  return result(first, written);
}

template <endianness endian, bool write>
simdutf_really_inline result transcode_utf8_to_utf16_with_replacement(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  if (length == 0) {
    return result(error_code::SUCCESS, 0);
  }
  size_t pos = 0;
  size_t written = 0;
  error_code first = error_code::SUCCESS;
  int short_prefixes = 0;
  while (pos < length) {
    if (short_prefixes >= 2) {
      return finish_with_scalar<endian, write>(input, length, pos, written,
                                               first, utf16_output);
    }
    const size_t remaining = length - pos;
    const utf8_result validation =
        validate_utf8_with_counts(input + pos, remaining);
    if (validation.error != error_code::SUCCESS &&
        (!is_utf8_validation_error(validation.error) ||
         validation.input_count >= remaining)) {
      return finish_with_scalar<endian, write>(input, length, pos, written,
                                               first, utf16_output);
    }
    size_t units = validation.utf16_length();
    if constexpr (write) {
      if (validation.input_count != 0) {
        units = convert_valid_prefix<endian>(
            input + pos, validation.input_count, utf16_output + written);
      }
    }
    written += units;
    if (validation.error == error_code::SUCCESS) {
      return result(first, written);
    }
    pos += validation.input_count;
    const size_t skip =
        scalar::utf8_to_utf16::maximal_subpart(input + pos, length - pos);
    if constexpr (write) {
      utf16_output[written] = scalar::utf16::replacement<endian>();
    }
    written += 1;
    if (first == error_code::SUCCESS) {
      first = validation.error;
    }
    if (validation.input_count < dense_prefix_limit) {
      short_prefixes += 1;
    } else {
      short_prefixes = 0;
    }
    pos += skip;
  }
  return result(first, written);
}
} // namespace

simdutf_warn_unused result utf16_length_from_utf8_with_replacement(
    const char *input, size_t length) noexcept {
  return transcode_utf8_to_utf16_with_replacement<endianness::LITTLE, false>(
      input, length, nullptr);
}

simdutf_warn_unused size_t convert_utf8_to_utf16le_with_replacement(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return transcode_utf8_to_utf16_with_replacement<endianness::LITTLE, true>(
             input, length, utf16_output)
      .count;
}

simdutf_warn_unused size_t convert_utf8_to_utf16be_with_replacement(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return transcode_utf8_to_utf16_with_replacement<endianness::BIG, true>(
             input, length, utf16_output)
      .count;
}

simdutf_warn_unused size_t convert_utf8_to_utf16_with_replacement(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
#if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf8_to_utf16be_with_replacement(input, length, utf16_output);
#else
  return convert_utf8_to_utf16le_with_replacement(input, length, utf16_output);
#endif
}

#endif // SIMDUTF_UTF8_TO_UTF16_WITH_REPLACEMENT_INL_H
