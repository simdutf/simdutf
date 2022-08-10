#ifndef SIMDUTF_UTF8_TO_UTF32_H
#define SIMDUTF_UTF8_TO_UTF32_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_utf32 {

inline size_t convert(const char* buf, size_t len, char32_t* utf32_output) {
 const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  size_t pos = 0;
  char32_t* start{utf32_output};
  while (pos < len) {
    // try to convert the next block of 16 ASCII bytes
    if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that they are ascii
      uint64_t v1;
      ::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) == 0) {
        size_t final_pos = pos + 16;
        while(pos < final_pos) {
          *utf32_output++ = char32_t(buf[pos]);
          pos++;
        }
        continue;
      }
    }
    uint8_t leading_byte = data[pos]; // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf32_output++ = char32_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8
      if(pos + 1 >= len) { return 0; } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
      // range check
      uint32_t code_point = (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80 || 0x7ff < code_point) { return 0; }
      *utf32_output++ = char32_t(code_point);
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8
      if(pos + 2 >= len) { return 0; } // minimal bound checking

      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return 0; }
      // range check
      uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if (code_point < 0x800 || 0xffff < code_point ||
          (0xd7ff < code_point && code_point < 0xe000)) {
        return 0;
      }
      *utf32_output++ = char32_t(code_point);
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if(pos + 3 >= len) { return 0; } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return 0; }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) { return 0; }

      // range check
      uint32_t code_point =
          (leading_byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff || 0x10ffff < code_point) { return 0; }
      *utf32_output++ = char32_t(code_point);
      pos += 4;
    } else {
      return 0;
    }
  }
  return utf32_output - start;
}

inline result convert_with_errors(const char* buf, size_t len, char32_t* utf32_output) {
 const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  size_t pos = 0;
  char32_t* start{utf32_output};
  while (pos < len) {
    // try to convert the next block of 16 ASCII bytes
    if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that they are ascii
      uint64_t v1;
      ::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) == 0) {
        size_t final_pos = pos + 16;
        while(pos < final_pos) {
          *utf32_output++ = char32_t(buf[pos]);
          pos++;
        }
        continue;
      }
    }
    uint8_t leading_byte = data[pos]; // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf32_output++ = char32_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8
      if(pos + 1 >= len) { return result(error_code::TOO_SHORT, pos); } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      // range check
      uint32_t code_point = (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80 || 0x7ff < code_point) { return result(error_code::OVERLONG, pos); }
      *utf32_output++ = char32_t(code_point);
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8
      if(pos + 2 >= len) { return result(error_code::TOO_SHORT, pos); } // minimal bound checking

      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      // range check
      uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if (code_point < 0x800 || 0xffff < code_point) { return result(error_code::OVERLONG, pos); }
      if (0xd7ff < code_point && code_point < 0xe000) { return result(error_code::SURROGATE, pos); }
      *utf32_output++ = char32_t(code_point);
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if(pos + 3 >= len) { return result(error_code::TOO_SHORT, pos); } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos);}
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }

      // range check
      uint32_t code_point =
          (leading_byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff) { return result(error_code::OVERLONG, pos); }
      if (0x10ffff < code_point) { return result(error_code::TOO_LARGE, pos); }
      *utf32_output++ = char32_t(code_point);
      pos += 4;
    } else {
      // we either have too many continuation bytes or an invalid leading byte
      if ((leading_byte & 0b11000000) == 0b10000000) { return result(error_code::TOO_LONG, pos); }
      else { return result(error_code::HEADER_BITS, pos); }
    }
  }
  return result(error_code::SUCCESS, utf32_output - start);
}

inline result rewind_and_convert_with_errors(const char* buf, size_t len, char32_t* utf32_output) {
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

  result res = convert_with_errors(buf, len + extra_len, utf32_output);
  if (res.error) {
    res.count -= extra_len;
  }
  return res;
}

} // utf8_to_utf32 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
