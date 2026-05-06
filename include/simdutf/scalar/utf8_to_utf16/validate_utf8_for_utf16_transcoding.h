#ifndef SIMDUTF_VALIDATE_UTF8_FOR_UTF16_TRANSCODING_H
#define SIMDUTF_VALIDATE_UTF8_FOR_UTF16_TRANSCODING_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_utf16 {

// credit: based on code from Google Fuchsia (Apache Licensed)
// TODO: upon finalizing, figure out constexpr23 stuff?
// TODO: also only-ascii optimization?
template <class BytePtr>
simdutf_constexpr23 simdutf_warn_unused full_result
validate_utf8_for_utf16_transcoding(BytePtr data, size_t len) noexcept {
  static_assert(
      std::is_same<typename std::decay<decltype(*data)>::type, uint8_t>::value,
      "dereferencing the data pointer must result in a uint8_t");
  uint64_t pos = 0;
  uint32_t code_point = 0;
  uint64_t utf16_units = 0;
  while (pos < len) {
    uint64_t next_pos;
    unsigned char byte = data[pos];

    while (byte < 0b10000000) {
      utf16_units += 1;
      if (++pos == len) {
        return full_result(error_code::SUCCESS, pos, utf16_units);
      }
      byte = data[pos];
    }

    if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80) {
        return full_result(error_code::OVERLONG, pos, utf16_units);
      }
      utf16_units += 1;
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if (code_point < 0x800) {
        return full_result(error_code::OVERLONG, pos, utf16_units);
      } else if (0xd7ff < code_point && code_point < 0xe000) {
        return full_result(error_code::SURROGATE, pos, utf16_units);
      }
      utf16_units += 1;
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) {
        return full_result(error_code::TOO_SHORT, pos, utf16_units);
      }
      // range check
      if (code_point < 0xffff) {
        return full_result(error_code::TOO_LONG, pos, utf16_units);
      } else if (0x10ffff < code_point) {
        return full_result(error_code::TOO_LARGE, pos, utf16_units);
      }
      utf16_units += 2;
    } else {
      // Continuation byte or invalid header byte
      return full_result(error_code::HEADER_BITS, pos, utf16_units);
    }
    pos = next_pos;
  }
  return full_result(error_code::SUCCESS, pos, utf16_units);
}

simdutf_really_inline simdutf_warn_unused full_result
validate_utf8_for_utf16_transcoding(const char *buf, size_t len) noexcept {
  return validate_utf8_for_utf16_transcoding(
      reinterpret_cast<const uint8_t *>(buf), len);
}

} // namespace utf8_to_utf16
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
