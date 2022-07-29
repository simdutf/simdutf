#ifndef SIMDUTF_UTF16_H
#define SIMDUTF_UTF16_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16 {

inline simdutf_warn_unused uint16_t swap_bytes(const uint16_t word) {
  return uint16_t((word >> 8) | (word << 8));
}

template <endianness big_endian>
inline simdutf_warn_unused bool validate(const char16_t *buf, size_t len) noexcept {
  const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  uint64_t pos = 0;
  while (pos < len) {
    uint16_t word = big_endian ? swap_bytes(data[pos]) : data[pos];
    if((word &0xF800) == 0xD800) {
        if(pos + 1 >= len) { return false; }
        uint16_t diff = uint16_t(word - 0xD800);
        if(diff > 0x3FF) { return false; }
        uint16_t next_word = big_endian ? uint16_t((data[pos + 1] >> 8) | (data[pos + 1] << 8)) : data[pos + 1];
        uint16_t diff2 = uint16_t(next_word - 0xDC00);
        if(diff2 > 0x3FF) { return false; }
        pos += 2;
    } else {
        pos++;
    }
  }
  return true;
}

template <endianness big_endian>
inline size_t count_code_points(const char16_t* buf, size_t len) {
  // We are not BOM aware.
  const uint16_t * p = reinterpret_cast<const uint16_t *>(buf);
  size_t counter{0};
  for(size_t i = 0; i < len; i++) {
    uint16_t word = big_endian ? swap_bytes(p[i]) : p[i];
    counter += ((word & 0xFC00) != 0xDC00);
  }
  return counter;
}

template <endianness big_endian>
inline size_t utf8_length_from_utf16(const char16_t* buf, size_t len) {
  // We are not BOM aware.
  const uint16_t * p = reinterpret_cast<const uint16_t *>(buf);
  size_t counter{0};
  for(size_t i = 0; i < len; i++) {
    uint16_t word = big_endian ? swap_bytes(p[i]) : p[i];
    /** ASCII **/
    if(word <= 0x7F) { counter++; }
    /** two-byte **/
    else if (word <= 0x7FF) { counter += 2; }
    /** three-byte **/
    else if((word <= 0xD7FF) || (word >= 0xE000)) { counter += 3; }
    /** surrogates -- 4 bytes **/
    else { counter += 2; }
  }
  return counter;
}

template <endianness big_endian>
inline size_t utf32_length_from_utf16(const char16_t* buf, size_t len) {
  // We are not BOM aware.
  const uint16_t * p = reinterpret_cast<const uint16_t *>(buf);
  size_t counter{0};
  for(size_t i = 0; i < len; i++) {
    uint16_t word = big_endian ? swap_bytes(p[i]) : p[i];
    counter += ((word & 0xFC00) != 0xDC00);
  }
  return counter;
}

simdutf_really_inline void change_endianness_utf16(const char16_t* in, size_t size, char16_t* out) {
  const uint16_t * input = reinterpret_cast<const uint16_t *>(in);
  uint16_t * output = reinterpret_cast<uint16_t *>(out);
  for (size_t i = 0; i < size; i++) {
    *output++ = uint16_t(input[i] >> 8 | input[i] << 8);
  }
}

} // utf16 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif