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
// is dense. An empty prefix is an adjacent error and does not count: one
// broken multibyte character is several maximal subparts.
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

template <endianness endian>
simdutf_really_inline result convert_with_errors_prefix(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  if constexpr (endian == endianness::LITTLE) {
    return convert_utf8_to_utf16le_with_errors(input, length, utf16_output);
  } else {
    return convert_utf8_to_utf16be_with_errors(input, length, utf16_output);
  }
}

// Haswell stores 16 bytes and then commits as few as 4 char16_t, leaving up
// to 8 bytes past the valid prefix. The caller may size the buffer to exactly
// the replacement length. Holding back 32 input bytes leaves at least 16
// char16_t of that buffer (4-byte UTF-8 becomes 2 char16_t), which covers the
// spill. The cut is on a character boundary when the input is valid.
constexpr size_t kernel_overflow_tail = 32;

simdutf_really_inline size_t utf8_boundary_before(const char *input,
                                                  size_t limit) noexcept {
  if ((uint8_t(input[limit]) & 0xC0) != 0x80) {
    return limit;
  }
  size_t i = limit;
  while (i > 0 && (uint8_t(input[i]) & 0xC0) == 0x80) {
    i--;
  }
  return i;
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
    // Same validating kernel as convert_utf8_to_utf16, including after an
    // error. The last 32 bytes stay out of this call: a kernel that stores
    // past a failing block must not pass the end of an exact-sized buffer.
    // On error, count is an input position, so the output length is counted
    // again over the valid prefix.
    if constexpr (write) {
      const size_t remaining = length - pos;
      if (remaining > kernel_overflow_tail + 64) {
        const size_t window =
            utf8_boundary_before(input + pos, remaining - kernel_overflow_tail);
        if (window >= 64) {
          const result bulk = convert_with_errors_prefix<endian>(
              input + pos, window, utf16_output + written);
          if (bulk.error == error_code::SUCCESS) {
            written += bulk.count;
            pos += window;
            short_prefixes = 0;
            continue;
          }
          if (is_utf8_validation_error(bulk.error) && bulk.count < window) {
            const size_t valid_bytes = bulk.count;
            if (valid_bytes != 0) {
              written += utf16_length_from_utf8(input + pos, valid_bytes);
            }
            utf16_output[written] = scalar::utf16::replacement<endian>();
            written += 1;
            if (first == error_code::SUCCESS) {
              first = bulk.error;
            }
            const size_t skip = scalar::utf8_to_utf16::maximal_subpart(
                input + pos + valid_bytes, length - pos - valid_bytes);
            pos += valid_bytes + skip;
            // A broken character is several adjacent errors, each with an
            // empty valid prefix. Those must not send the rest of the input
            // down the scalar path. Only a short run of valid bytes counts.
            if (valid_bytes >= dense_prefix_limit) {
              short_prefixes = 0;
            } else if (valid_bytes != 0) {
              short_prefixes += 1;
            }
            continue;
          }
          return finish_with_scalar<endian, write>(input, length, pos, written,
                                                   first, utf16_output);
        }
      }
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
    if (validation.input_count >= dense_prefix_limit) {
      short_prefixes = 0;
    } else if (validation.input_count != 0) {
      short_prefixes += 1;
    }
    pos += skip;
  }
  return result(first, written);
}
} // namespace

// Recorded errors are absolute byte indexes. The gaps between them are valid
// UTF-8, so conversion can use the unchecked kernel there. A tail past the
// stored errors, when more_errors is set, still has to be discovered.
template <endianness endian>
simdutf_really_inline size_t convert_using_locations(
    const char *input, size_t length, char16_t *utf16_output,
    const utf8_to_utf16_result &locations) noexcept {
  size_t recorded = locations.error_count;
  bool more = locations.more_errors;
  if (recorded > utf8_to_utf16_result::max_errors) {
    recorded = utf8_to_utf16_result::max_errors;
    more = true;
  }
  size_t pos = 0;
  size_t written = 0;
  for (size_t i = 0; i < recorded; i++) {
    const size_t err = locations.error_offset[i];
    if (err < pos || err >= length) {
      return written + transcode_utf8_to_utf16_with_replacement<endian, true>(
                           input + pos, length - pos, utf16_output + written)
                           .count;
    }
    if (err > pos) {
      written += convert_valid_prefix<endian>(input + pos, err - pos,
                                              utf16_output + written);
    }
    utf16_output[written] = scalar::utf16::replacement<endian>();
    written += 1;
    const size_t skip =
        scalar::utf8_to_utf16::maximal_subpart(input + err, length - err);
    pos = err + skip;
  }
  if (pos > length) {
    return written;
  }
  if (!more) {
    if (pos < length) {
      written += convert_valid_prefix<endian>(input + pos, length - pos,
                                              utf16_output + written);
    }
    return written;
  }
  return written + transcode_utf8_to_utf16_with_replacement<endian, true>(
                       input + pos, length - pos, utf16_output + written)
                       .count;
}

simdutf_warn_unused utf8_to_utf16_result
utf16_length_from_utf8_with_replacement(const char *input,
                                        size_t length) noexcept {
  utf8_to_utf16_result out;
  size_t pos = 0;
  while (pos < length) {
    const utf8_result validation =
        validate_utf8_with_counts(input + pos, length - pos);
    if (validation.error == error_code::SUCCESS) {
      out.count += validation.utf16_length();
      return out;
    }
    if (!is_utf8_validation_error(validation.error) ||
        validation.input_count >= length - pos) {
      out.count += scalar::utf8_to_utf16::count_with_replacement(input + pos,
                                                                 length - pos);
      out.more_errors = true;
      if (out.error == error_code::SUCCESS) {
        out.error = validation.error;
      }
      return out;
    }
    out.count += validation.utf16_length() + 1;
    if (out.error == error_code::SUCCESS) {
      out.error = validation.error;
    }
    const size_t err = pos + validation.input_count;
    const size_t skip =
        scalar::utf8_to_utf16::maximal_subpart(input + err, length - err);
    if (out.error_count < utf8_to_utf16_result::max_errors) {
      out.error_offset[out.error_count] = err;
      out.error_count += 1;
    } else {
      out.more_errors = true;
    }
    pos = err + skip;
  }
  return out;
}

simdutf_warn_unused size_t convert_utf8_to_utf16le_with_replacement(
    const char *input, size_t length, char16_t *utf16_output,
    const utf8_to_utf16_result &locations) noexcept {
  return convert_using_locations<endianness::LITTLE>(input, length,
                                                     utf16_output, locations);
}

simdutf_warn_unused size_t convert_utf8_to_utf16le_with_replacement(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return convert_utf8_to_utf16le_with_replacement(
      input, length, utf16_output,
      utf16_length_from_utf8_with_replacement(input, length));
}

simdutf_warn_unused size_t convert_utf8_to_utf16be_with_replacement(
    const char *input, size_t length, char16_t *utf16_output,
    const utf8_to_utf16_result &locations) noexcept {
  return convert_using_locations<endianness::BIG>(input, length, utf16_output,
                                                  locations);
}

simdutf_warn_unused size_t convert_utf8_to_utf16be_with_replacement(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return convert_utf8_to_utf16be_with_replacement(
      input, length, utf16_output,
      utf16_length_from_utf8_with_replacement(input, length));
}

simdutf_warn_unused size_t convert_utf8_to_utf16_with_replacement(
    const char *input, size_t length, char16_t *utf16_output,
    const utf8_to_utf16_result &locations) noexcept {
#if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf8_to_utf16be_with_replacement(input, length, utf16_output,
                                                  locations);
#else
  return convert_utf8_to_utf16le_with_replacement(input, length, utf16_output,
                                                  locations);
#endif
}

simdutf_warn_unused size_t convert_utf8_to_utf16_with_replacement(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return convert_utf8_to_utf16_with_replacement(
      input, length, utf16_output,
      utf16_length_from_utf8_with_replacement(input, length));
}

#endif // SIMDUTF_UTF8_TO_UTF16_WITH_REPLACEMENT_INL_H
