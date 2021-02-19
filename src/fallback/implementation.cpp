#include "simdutf/fallback/begin.h"
#include "generic/utf16_validation/utf16_scalar_validator.h"

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

// credit: based on code from Google Fuchsia (Apache Licensed)
simdutf_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
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
    if (byte < 0b10000000) {
      pos++;
      continue;
    } else if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80 || 0x7ff < code_point) { return false; }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if (code_point < 0x800 || 0xffff < code_point ||
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

/**
* Start of functions that need to be improved and maybe moved.
*/
namespace {

int utf8_char_size(const char *c) {
  const uint8_t m0x = 0x80, c0x = 0x00, m10x = 0xC0, c10x = 0x80, m110x = 0xE0,
                c110x = 0xC0, m1110x = 0xF0, c1110x = 0xE0, m11110x = 0xF8,
                c11110x = 0xF0;

  if ((c[0] & m0x) == c0x)
    return 1;

  if ((c[0] & m110x) == c110x)
    if ((c[1] & m10x) == c10x)
      return 2;

  if ((c[0] & m1110x) == c1110x)
    if ((c[1] & m10x) == c10x)
      if ((c[2] & m10x) == c10x)
        return 3;

  if ((c[0] & m11110x) == c11110x)
    if ((c[1] & m10x) == c10x)
      if ((c[2] & m10x) == c10x)
        if ((c[3] & m10x) == c10x)
          return 4;

  if ((c[0] & m10x) == c10x) // not a first UTF-8 byte
    return 0;

  return -1; // if c[0] is a first byte but the other bytes don't match
}

int codepoint_utf16_size(uint32_t c) {
  if (c < 0x10000)
    return 1;
  if (c < 0x110000)
    return 2;

  return 0;
}

uint32_t utf8_to_unicode32(const char *c, size_t *index) {
  uint32_t v;
  int size;
  const uint8_t m6 = 63, m5 = 31, m4 = 15, m3 = 7;

  if (c == NULL)
    return 0;

  size = utf8_char_size(c);

  if (size > 0 && index)
    *index += size - 1;

  switch (size) {
  case 1:
    v = c[0];
    break;
  case 2:
    v = c[0] & m5;
    v = v << 6 | (c[1] & m6);
    break;
  case 3:
    v = c[0] & m4;
    v = v << 6 | (c[1] & m6);
    v = v << 6 | (c[2] & m6);
    break;
  case 4:
    v = c[0] & m3;
    v = v << 6 | (c[1] & m6);
    v = v << 6 | (c[2] & m6);
    v = v << 6 | (c[3] & m6);
    break;
  case 0:  // not a first UTF-8 byte
  case -1: // corrupt UTF-8 letter
  default:
    v = -1;
    break;
  }

  return v;
}

char16_t *sprint_utf16(char16_t *str, uint32_t c)	{
	int c_size;

	if (str==NULL)
		return NULL;

	c_size = codepoint_utf16_size(c);
	switch (c_size) {
		case 1:
			str[0] = uint16_t(c);
			if (c > 0)
				str[1] = '\0';
			break;

		case 2:
			c -= 0x10000;
			str[0] = uint16_t(0xD800 + (c >> 10));
			str[1] = uint16_t(0xDC00 + (c & 0x3FF));
			str[2] = '\0';
			break;

		default:
			str[0] = '\0';
	}
	return str;
}

size_t utf8_to_utf16_with_length(const char *utf8, size_t len, char16_t *utf16) {
	int j;
	uint32_t c;
  size_t i;
	for (i=0, j=0, c=1; i < len; i++) {
		c = utf8_to_unicode32(&utf8[i], &i);
		sprint_utf16(&utf16[j], c);
		j += codepoint_utf16_size(c);
	}
  return j;
}

} // anonymous namespace

/**
 * end of ugly functions to be replaced.
 */

simdutf_warn_unused bool implementation::validate_utf16(const char16_t *buf, size_t len) const noexcept {
    return fallback::utf16_validation::scalar_validate_utf16(buf, len);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16(const char* /*buf*/, size_t /*len*/, char16_t* /*utf16_output*/) const noexcept {
  return 0; // stub
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  return utf8_to_utf16_with_length(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf16_to_utf8(const char16_t* /*buf*/, size_t /*len*/, char* /*utf8_output*/) const noexcept {
  return 0; // stub
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf8(const char16_t* /*buf*/, size_t /*len*/, char* /*utf8_output*/) const noexcept {
  return 0; // stub
}

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/fallback/end.h"
