#ifndef SIMDUTF_UTF8_H
#define SIMDUTF_UTF8_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf8 {

// credit: based on code from Google Fuchsia (Apache Licensed)
template <class BytePtr>
simdutf_constexpr23 simdutf_warn_unused bool validate(BytePtr data,
                                                      size_t len) noexcept {
  static_assert(
      std::is_same<typename std::decay<decltype(*data)>::type, uint8_t>::value,
      "dereferencing the data pointer must result in a uint8_t");
  uint64_t pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    uint64_t next_pos;
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    { // check if the next 16 bytes are ascii.
      next_pos = pos + 16;
      if (next_pos <= len) { // if it is safe to read 16 more bytes, check
                             // that they are ascii
        uint64_t v1{};
        std::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2{};
        std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2};
        if ((v & 0x8080808080808080) == 0) {
          pos = next_pos;
          continue;
        }
      }
    }

    unsigned char byte = data[pos];

    while (byte < 0b10000000) {
      if (++pos == len) {
        return true;
      }
      byte = data[pos];
    }

    if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) {
        return false;
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return false;
      }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if ((code_point < 0x80) || (0x7ff < code_point)) {
        return false;
      }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) {
        return false;
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return false;
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return false;
      }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if ((code_point < 0x800) || (0xffff < code_point) ||
          (0xd7ff < code_point && code_point < 0xe000)) {
        return false;
      }
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) {
        return false;
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return false;
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return false;
      }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) {
        return false;
      }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff || 0x10ffff < code_point) {
        return false;
      }
    } else {
      // we may have a continuation
      return false;
    }
    pos = next_pos;
  }
  return true;
}

simdutf_really_inline simdutf_warn_unused bool validate(const char *buf,
                                                        size_t len) noexcept {
  return validate(reinterpret_cast<const uint8_t *>(buf), len);
}

template <class BytePtr>
simdutf_constexpr23 simdutf_warn_unused result
validate_with_errors(BytePtr data, size_t len) noexcept {
  static_assert(
      std::is_same<typename std::decay<decltype(*data)>::type, uint8_t>::value,
      "dereferencing the data pointer must result in a uint8_t");
  size_t pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    // check of the next 16 bytes are ascii.
    size_t next_pos = pos + 16;
    if (next_pos <=
        len) { // if it is safe to read 16 more bytes, check that they are ascii
      uint64_t v1;
      std::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) == 0) {
        pos = next_pos;
        continue;
      }
    }
    unsigned char byte = data[pos];

    while (byte < 0b10000000) {
      if (++pos == len) {
        return result(error_code::SUCCESS, len);
      }
      byte = data[pos];
    }

    if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if ((code_point < 0x80) || (0x7ff < code_point)) {
        return result(error_code::OVERLONG, pos);
      }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if ((code_point < 0x800) || (0xffff < code_point)) {
        return result(error_code::OVERLONG, pos);
      }
      if (0xd7ff < code_point && code_point < 0xe000) {
        return result(error_code::SURROGATE, pos);
      }
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff) {
        return result(error_code::OVERLONG, pos);
      }
      if (0x10ffff < code_point) {
        return result(error_code::TOO_LARGE, pos);
      }
    } else {
      // we either have too many continuation bytes or an invalid leading byte
      if ((byte & 0b11000000) == 0b10000000) {
        return result(error_code::TOO_LONG, pos);
      } else {
        return result(error_code::HEADER_BITS, pos);
      }
    }
    pos = next_pos;
  }
  return result(error_code::SUCCESS, len);
}

simdutf_really_inline simdutf_warn_unused result
validate_with_errors(const char *buf, size_t len) noexcept {
  return validate_with_errors(reinterpret_cast<const uint8_t *>(buf), len);
}

// Finds the previous leading byte starting backward from buf and validates with
// errors from there Used to pinpoint the location of an error when an invalid
// chunk is detected We assume that the stream starts with a leading byte, and
// to check that it is the case, we ask that you pass a pointer to the start of
// the stream (start).
inline simdutf_warn_unused result rewind_and_validate_with_errors(
    const char *start, const char *buf, size_t len) noexcept {
  // First check that we start with a leading byte
  if ((*start & 0b11000000) == 0b10000000) {
    return result(error_code::TOO_LONG, 0);
  }
  size_t extra_len{0};
  // A leading byte cannot be further than 4 bytes away
  for (int i = 0; i < 5; i++) {
    unsigned char byte = *buf;
    if ((byte & 0b11000000) != 0b10000000) {
      break;
    } else {
      buf--;
      extra_len++;
    }
  }

  result res = validate_with_errors(buf, len + extra_len);
  res.count -= extra_len;
  return res;
}

// credit: based on code from Google Fuchsia (Apache Licensed)
// TODO: upon finalizing, figure out constexpr23 stuff?
// TODO: also only-ascii optimization?
template <class BytePtr>
simdutf_constexpr23 simdutf_warn_unused utf8_result
validate_utf8_with_counts(BytePtr data, size_t len) noexcept {
  static_assert(
      std::is_same<typename std::decay<decltype(*data)>::type, uint8_t>::value,
      "dereferencing the data pointer must result in a uint8_t");
  uint64_t pos = 0;
  uint32_t code_point = 0;
  uint64_t continuations = 0;
  uint64_t four_byte = 0;
  while (pos < len) {
    uint64_t next_pos;
    unsigned char byte = data[pos];

    while (byte < 0b10000000) {
      if (++pos == len) {
        return utf8_result(error_code::SUCCESS, pos, continuations, four_byte);
      }
      byte = data[pos];
    }

    if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80) {
        return utf8_result(error_code::OVERLONG, pos, continuations, four_byte);
      }
      continuations += 1;
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if (code_point < 0x800) {
        return utf8_result(error_code::OVERLONG, pos, continuations, four_byte);
      } else if (0xd7ff < code_point && code_point < 0xe000) {
        return utf8_result(error_code::SURROGATE, pos, continuations,
                           four_byte);
      }
      continuations += 2;
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) {
        return utf8_result(error_code::TOO_SHORT, pos, continuations,
                           four_byte);
      }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff) {
        return utf8_result(error_code::OVERLONG, pos, continuations, four_byte);
      } else if (0x10ffff < code_point) {
        return utf8_result(error_code::TOO_LARGE, pos, continuations,
                           four_byte);
      }
      continuations += 3;
      four_byte += 1;
    } else {
      // Continuation byte or invalid header byte
      if ((byte & 0b11000000) == 0b10000000) {
        return utf8_result(error_code::TOO_LONG, pos, continuations, four_byte);
      } else {
        return utf8_result(error_code::HEADER_BITS, pos, continuations,
                           four_byte);
      }
    }
    pos = next_pos;
  }
  return utf8_result(error_code::SUCCESS, pos, continuations, four_byte);
}

simdutf_really_inline simdutf_warn_unused utf8_result
validate_utf8_with_counts(const char *buf, size_t len) noexcept {
  return validate_utf8_with_counts(reinterpret_cast<const uint8_t *>(buf), len);
}

// Finds the previous leading byte starting backward from buf and validates from
// there on out while counting continuation bytes and 4-byte leads. Used to
// pinpoint the location of an error when an invalid chunk is detected. We check
// that the stream starts with a leading byte with the passed pointer to the
// start of the stream (start).
inline simdutf_warn_unused utf8_result rewind_and_validate_with_counts(
    const char *start, const char *buf, size_t len) noexcept {
  // First check that we start with a leading byte
  if ((*start & 0b11000000) == 0b10000000) {
    return utf8_result(error_code::TOO_LONG, 0, 0, 0);
  }
  // If this is the start of the buffer, we don't need to rewind to check the
  // previous chunk.
  if (start == buf) {
    return validate_utf8_with_counts(buf, len);
  }
  // As this isn't the start of the buffer, the error might be in the previous
  // chunk. So we backtrack and also need to avoid double counting.
  size_t extra_len{0};
  size_t extra_continuations{0};
  size_t extra_four_bytes{0};
  // A leading byte cannot be further than 4 bytes away
  // TODO: Wouldn't `i < 4` suffice? Maybe actually no difference behavior-wise
  // (Changed to 4 in a previous commit to try)
  for (int i = 0; i < 5; i++) {
    // We rewind at least one byte to find the actual previous leading byte.
    // We do so to backtrack into the last chunk in case the utf8 error happened
    // there already.
    buf--;
    unsigned char byte = *buf;
    extra_len++;
    if (byte >= 0b11110000) {
      extra_four_bytes += 1;
    } else if ((int8_t)byte < -65 + 1) {
      extra_continuations += 1;
    }
    if ((byte & 0b11000000) != 0b10000000) {
      break;
    }
  }

  utf8_result res = validate_utf8_with_counts(buf, len + extra_len);
  res.input_count -= extra_len;
  res.continuation_count -= extra_continuations;
  res.four_byte_count -= extra_four_bytes;
  return res;
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t count_code_points(InputPtr data, size_t len) {
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    // -65 is 0b10111111, anything larger in two-complement's should start a new
    // code point.
    if (int8_t(data[i]) > -65) {
      counter++;
    }
  }
  return counter;
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t utf16_length_from_utf8(InputPtr data, size_t len) {
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    if (int8_t(data[i]) > -65) {
      counter++;
    }
    if (uint8_t(data[i]) >= 240) {
      counter++;
    }
  }
  return counter;
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_warn_unused simdutf_constexpr23 size_t
trim_partial_utf8(InputPtr input, size_t length) {
  if (length < 3) {
    switch (length) {
    case 2:
      if (uint8_t(input[length - 1]) >= 0xc0) {
        return length - 1;
      } // 2-, 3- and 4-byte characters with only 1 byte left
      if (uint8_t(input[length - 2]) >= 0xe0) {
        return length - 2;
      } // 3- and 4-byte characters with only 2 bytes left
      return length;
    case 1:
      if (uint8_t(input[length - 1]) >= 0xc0) {
        return length - 1;
      } // 2-, 3- and 4-byte characters with only 1 byte left
      return length;
    case 0:
      return length;
    }
  }
  if (uint8_t(input[length - 1]) >= 0xc0) {
    return length - 1;
  } // 2-, 3- and 4-byte characters with only 1 byte left
  if (uint8_t(input[length - 2]) >= 0xe0) {
    return length - 2;
  } // 3- and 4-byte characters with only 1 byte left
  if (uint8_t(input[length - 3]) >= 0xf0) {
    return length - 3;
  } // 4-byte characters with only 3 bytes left
  return length;
}

} // namespace utf8
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
