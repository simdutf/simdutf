#ifndef SIMDUTF_UTF16_TO_LATIN1_H
#define SIMDUTF_UTF16_TO_LATIN1_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_latin1 {

inline size_t convert(const char16_t* buf, size_t len, char* latin_output) {
 const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  size_t pos = 0;
  char* start{latin_output};
  uint16_t word = 0;

  while (pos < len) {
        word = data[pos];
        if((word &0xF800 ) != 0xD800) { //check if the first top five bits indicate a surrogate pair
        // If none, we just need to make sure that it will fit into 8 bit:
            if((word & 0xFF00) == 0) {
                *latin_output++ = char(word);
            }
            else {
                return 0;
            }
        pos++;
    } else { return 0;}
  }
  return latin_output - start;
}

inline result convert_with_errors(const char16_t* buf, size_t len, char* latin_output) {
 const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  size_t pos = 0;
  char* start{latin_output};
  uint16_t word = 0; 
  
  while (pos < len) {
        word = data[pos];
        if((word &0xF800 ) != 0xD800) { //check if the first top five bits indicate a surrogate pair
        // If none, we just need to make sure that it will fit into 8 bit:
            if((word & 0xFF00) == 0) {
                *latin_output++ = char(word);
            }
            else {
                return result(error_code::TOO_LARGE, pos); // Valid UTF16 but not a Latin1 character
            }
        pos++;
    } else { return result(error_code::SURROGATE, pos);}
  }
  return result(error_code::SUCCESS,latin_output - start);
}


} // utf16_to_latin1 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif