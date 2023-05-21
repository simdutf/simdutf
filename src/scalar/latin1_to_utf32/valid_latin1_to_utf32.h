#ifndef SIMDUTF_VALID_LATIN1_TO_UTF32_H
#define SIMDUTF_VALID_LATIN1_TO_UTF32_H

namespace simdutf {
namespace scalar {
namespace {
namespace latin1_to_utf32 {

//redundant? Not sure how I can further validate Latin1 input. 
inline size_t convert_valid(const char *buf, size_t len, char32_t *utf32_output) {
  const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
  char32_t* start{utf32_output};

  for (size_t i = 0; i < len; i++) {
    utf32_output[i] = (char32_t)data[i]; 
  }

  return utf32_output - start;
}



} // latin1_to_utf32 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif