#ifndef SIMDUTF_UTF32_TO_LATIN1_H
#define SIMDUTF_UTF32_TO_LATIN1_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_latin1 {


inline size_t convert(const char32_t *buf, size_t len, char *latin1_output) {
    const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
    char* start{latin1_output};


    uint32_t utf32_char;

    for (size_t i = 0; i < len; i++) {

        utf32_char = (uint32_t)data[i]; 
        
        if ((utf32_char & 0xFFFFFF00) == 0){ // Check if the character can be represented in Latin-1
            latin1_output[i] = (char)(utf32_char & 0xFF);
        } else {return 0};
    }
  return latin1_output - start;

}

inline size_t convert_with_errors(const char32_t *buf, size_t len, char *latin1_output) {
    const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
    char* start{latin1_output};

    uint32_t utf32_char;

    for (size_t i = 0; i < len; i++) {

        utf32_char = (uint32_t)data[i]; 
        
        if ((utf32_char & 0xFFFFFF00) == 0){ // Check if the character can be represented in Latin-1
            latin1_output[i] = (char)(utf32_char & 0xFF);
        } else {return error_code::OTHER};
    }
  return result(error_code::SUCCESS, latin1_output - start);

}



} // utf32_to_latin1 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif