namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_to_utf16 {

using namespace simd;

// The finisher_functions namespace contains
// functions that can be used to "finish" the processing.
// They are not expected to fast.
// Author: Michel Rouzic (https://github.com/Photosounder)
// Original source:
// https://github.com/Photosounder/rouziclib/blob/master/rouziclib/text/unicode.c
// Project license: Apache
namespace finisher_functions {

int utf8_char_size(const uint8_t *c) {
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
static inline int codepoint_utf16_size(uint32_t c) {
  if (c < 0x10000)
    return 1;
  if (c < 0x110000)
    return 2;

  return 0;
}

uint32_t utf8_to_unicode32(const uint8_t *c, size_t *index) {
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

size_t strlen_utf8_to_utf16_with_length(const uint8_t *str, size_t len) {
  size_t i, count;
  uint32_t c;

  for (i = 0, count = 0; i < len; i++) {
    c = utf8_to_unicode32(&str[i], &i);
    count += codepoint_utf16_size(c);
  }
  return count;
}

} // namespace finisher_functions

} // namespace utf8_to_utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
