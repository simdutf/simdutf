#ifndef SIMDUTF_VALID_UTF32_TO_UTF16_H
#define SIMDUTF_VALID_UTF32_TO_UTF16_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_utf16 {

inline size_t convert_valid(const char32_t* buf, size_t len, char16_t* utf16_output) {
  const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
  size_t pos = 0;
  char16_t* start{utf16_output};

  return utf16_output - start;
}

} // utf32_to_utf16 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif