#ifndef SIMDUTF_VALID_UTF16_TO_LATIN1_H
#define SIMDUTF_VALID_UTF16_TO_LATIN1_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_latin1 {

inline size_t convert_valid(const char16_t* buf, size_t len, char* latin_output) {
 const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  size_t pos = 0;
  char* start{latin_output};
  
  while (pos < len) {
        if((word &0xF800 ) != 0xD800) { //check if the first top five bits indicate a surrogate pair
        // If none, we just need to make sure that it will fit into 8 bit:
            if((word & 0xFF00) == 0) {
                *latin_output++ = char(word));
            }
        pos++;
    } 
  }
  return latin_output - start;
}


} // utf16_to_latin1 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif