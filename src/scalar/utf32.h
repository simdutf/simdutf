#ifndef SIMDUTF_UTF32_H
#define SIMDUTF_UTF32_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf32 {

inline simdutf_warn_unused bool validate(const char32_t *buf, size_t len) noexcept {
  const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
  uint64_t pos = 0;
  while (pos < len) {
    uint32_t word = data[pos];
    if(word > 0x10FFFF || (word >= 0xD800 && word <= 0xDFFF)) {
        return false;
    } else {
        pos++;
    }
  }
  return true;
}

} // utf32 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif