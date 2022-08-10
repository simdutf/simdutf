#ifndef SIMDUTF_UTF8_H
#define SIMDUTF_UTF8_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf8 {
// credit: based on code from Google Fuchsia (Apache Licensed)
inline simdutf_warn_unused bool validate(const char *buf, size_t len) noexcept {
  const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  uint64_t pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    // check of the next 8 bytes are ascii.
    uint64_t next_pos = pos + 16;
    if (next_pos <= len) { // if it is safe to read 8 more bytes, check that they are ascii
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
      if (++pos == len) { return true; }
      byte = data[pos];
    }

    if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if ((code_point < 0x80) || (0x7ff < code_point)) { return false; }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
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
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff || 0x10ffff < code_point) { return false; }
    } else {
      // we may have a continuation
      return false;
    }
    pos = next_pos;
  }
  return true;
}

inline simdutf_warn_unused result validate_with_errors(const char *buf, size_t len) noexcept {
  const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  uint64_t pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    // check of the next 8 bytes are ascii.
    uint64_t next_pos = pos + 16;
    if (next_pos <= len) { // if it is safe to read 8 more bytes, check that they are ascii
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
      if (++pos == len) { return result(error_code::SUCCESS, len); }
      byte = data[pos];
    }

    if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) { return result(error_code::TOO_SHORT, pos); }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if ((code_point < 0x80) || (0x7ff < code_point)) { return result(error_code::OVERLONG, pos); }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) { return result(error_code::TOO_SHORT, pos); }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if ((code_point < 0x800) || (0xffff < code_point)) { return result(error_code::OVERLONG, pos);}
      if (0xd7ff < code_point && code_point < 0xe000) { return result(error_code::SURROGATE, pos); }
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) { return result(error_code::TOO_SHORT, pos); }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff) { return result(error_code::OVERLONG, pos); }
      if (0x10ffff < code_point) { return result(error_code::TOO_LARGE, pos); }
    } else {
      // we either have too many continuation bytes or an invalid leading byte
      if ((byte & 0b11000000) == 0b10000000) { return result(error_code::TOO_LONG, pos); }
      else { return result(error_code::HEADER_BITS, pos); }
    }
    pos = next_pos;
  }
  return result(error_code::SUCCESS, len);
}

// Finds the previous leading byte and validates with errors from there
// Used to pinpoint the location of an error when an invalid chunk is detected
inline simdutf_warn_unused result rewind_and_validate_with_errors(const char *buf, size_t len) noexcept {
  size_t extra_len{0};
  // A leading byte cannot be further than 4 bytes away
  for(int i = 0; i < 5; i++) {
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

inline size_t count_code_points(const char* buf, size_t len) {
    const int8_t * p = reinterpret_cast<const int8_t *>(buf);
    size_t counter{0};
    for(size_t i = 0; i < len; i++) {
        // -65 is 0b10111111, anything larger in two-complement's should start a new code point.
        if(p[i] > -65) { counter++; }
    }
    return counter;
}

inline size_t utf16_length_from_utf8(const char* buf, size_t len) {
    const int8_t * p = reinterpret_cast<const int8_t *>(buf);
    size_t counter{0};
    for(size_t i = 0; i < len; i++) {
        if(p[i] > -65) { counter++; }
        if(uint8_t(p[i]) >= 240) { counter++; }
    }
    return counter;
}

inline size_t utf32_length_from_utf8(const char* buf, size_t len) {
    const int8_t * p = reinterpret_cast<const int8_t *>(buf);
    size_t counter{0};
    for(size_t i = 0; i < len; i++) {
        // -65 is 0b10111111, anything larger in two-complement's should start a new code point.
        if(p[i] > -65) { counter++; }
    }
    return counter;
}

} // utf8 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif