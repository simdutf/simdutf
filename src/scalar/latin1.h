#ifndef SIMDUTF_LATIN1_H
#define SIMDUTF_LATIN1_H

namespace simdutf {
namespace scalar {
namespace {
namespace latin1 {

inline size_t utf32_length_from_latin1(const char* buf, size_t len) {
  // We are not BOM aware.
  return len; // a utf32 will always represent 1 latin1 character
}




} // utf32 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif