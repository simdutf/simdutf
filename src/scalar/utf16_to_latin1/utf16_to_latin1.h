#ifndef SIMDUTF_UTF16_TO_LATIN1_H
#define SIMDUTF_UTF16_TO_LATIN1_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_latin1 {

template <endianness big_endian>
inline size_t convert(const char16_t* buf, size_t len, char* latin_output) {
  const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  size_t pos = 0;
  char* start{latin_output};
  uint16_t word = 0;

  while (pos < len) {
    word = !match_system(big_endian) ? utf16::swap_bytes(data[pos]) : data[pos];

    if((word & 0xFF00 ) == 0) { 
        *latin_output++ = char(word);
        pos++;
    } else { return 0;}
  }
  return latin_output - start;
}

template <endianness big_endian>
inline result convert_with_errors(const char16_t* buf, size_t len, char* latin_output) {
 const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  size_t pos = 0;
  char* start{latin_output};
  uint16_t word = 0; 
  

  while (pos < len) {
    word = !match_system(big_endian) ? utf16::swap_bytes(data[pos]) : data[pos];

    if((word & 0xFF00 ) == 0) { 
        *latin_output++ = char(word);
        pos++;
    } else { return result(error_code::TOO_LARGE, pos);}
  }
  return result(error_code::SUCCESS,latin_output - start);
}


} // utf16_to_latin1 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif