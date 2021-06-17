#ifndef SIMDUTF_UTF16_H
#define SIMDUTF_UTF16_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16 {

inline simdutf_warn_unused bool validate(const char16_t *buf, size_t len) noexcept {
  const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
  uint64_t pos = 0;
  while (pos < len) {
    uint16_t word = data[pos];
    if((word &0xF800) == 0xD800) {
        if(pos + 1 >= len) { return false; }
        uint16_t diff = uint16_t(word - 0xD800);
        if(diff >= 0x3FF) { return false; }
        uint16_t next_word = data[pos + 1];
        uint16_t diff2 = uint16_t(next_word - 0xDC00);
        if(diff2 > 0x3FF) { return false; }
        pos += 2;
    } else {
        pos++;
    }

  }
  return true;
}


inline size_t count_code_points(const char16_t* buf, size_t len) {
    // We are not BOM aware.
    const uint16_t * p = reinterpret_cast<const uint16_t *>(buf);
    size_t counter{0};
    for(size_t i = 0; i < len; i++) {
        counter += ((p[i] & 0xFC00) != 0xDC00);
    }
    return counter;
}

} // utf16 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif