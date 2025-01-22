#ifndef SIMDUTF_UTF32_H
#define SIMDUTF_UTF32_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf32 {

inline simdutf_warn_unused bool validate(const char32_t *buf,
                                         size_t len) noexcept {
  const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
  uint64_t pos = 0;
  for (; pos < len; pos++) {
    uint32_t word = data[pos];
    if (word > 0x10FFFF || (word >= 0xD800 && word <= 0xDFFF)) {
      return false;
    }
  }
  return true;
}

inline simdutf_warn_unused result validate_with_errors(const char32_t *buf,
                                                       size_t len) noexcept {
  const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
  size_t pos = 0;
  for (; pos < len; pos++) {
    uint32_t word = data[pos];
    if (word > 0x10FFFF) {
      return result(error_code::TOO_LARGE, pos);
    }
    if (word >= 0xD800 && word <= 0xDFFF) {
      return result(error_code::SURROGATE, pos);
    }
  }
  return result(error_code::SUCCESS, pos);
}

inline size_t utf8_length_from_utf32(const char32_t *buf, size_t len) {
  // We are not BOM aware.
  const uint32_t *p = reinterpret_cast<const uint32_t *>(buf);
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    // credit: @ttsugriy  for the vectorizable approach
    counter++;                                     // ASCII
    counter += static_cast<size_t>(p[i] > 0x7F);   // two-byte
    counter += static_cast<size_t>(p[i] > 0x7FF);  // three-byte
    counter += static_cast<size_t>(p[i] > 0xFFFF); // four-bytes
  }
  return counter;
}

inline size_t utf16_length_from_utf32(const char32_t *buf, size_t len) {
  // We are not BOM aware.
  const uint32_t *p = reinterpret_cast<const uint32_t *>(buf);
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    counter++;                                     // non-surrogate word
    counter += static_cast<size_t>(p[i] > 0xFFFF); // surrogate pair
  }
  return counter;
}

} // namespace utf32
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
