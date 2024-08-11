#ifndef SIMDUTF_VALID_UTF16_TO_LATIN1_H
#define SIMDUTF_VALID_UTF16_TO_LATIN1_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_latin1 {

template <endianness big_endian>
inline size_t convert_valid(const char16_t* buf, size_t len, char* latin_output) {
 const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  size_t pos = 0;
  char* start{latin_output};
  uint16_t word = 0;

  while (pos < len) {
    word = !match_system(big_endian) ? utf16::swap_bytes(data[pos]) : data[pos];
    if (word <= 0xFF) {
        *latin_output++ = char(word);
        pos++;
    } else {
        // can't be represented as latin1 - return error
        return 0;
    }
  }

  return latin_output - start;
}

} // utf16_to_latin1 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
