#ifndef SIMDUTF_UTF32_TO_UTF8_H
#define SIMDUTF_UTF32_TO_UTF8_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_utf8 {

inline size_t convert(const char32_t* buf, size_t len, char* utf8_output) {
  const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
  char* start{utf8_output};
  return utf8_output - start;
}

} // utf32_to_utf8 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif