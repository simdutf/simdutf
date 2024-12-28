#include "simdutf.h"

#include <cstdint>

namespace temporary {

bool match_system2(temporary::endianness e) {
#define SIMDUTF_IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#if SIMDUTF_IS_BIG_ENDIAN
  return e == endianness::BIG;
#else
  return e == temporary::endianness::LITTLE;
#endif
}
inline uint16_t swap_bytes(const uint16_t word) {
  return uint16_t((word >> 8) | (word << 8));
}
template <temporary::endianness big_endian>
inline temporary::result validate_with_errors(const char16_t *data,
                                              size_t len) noexcept {
  size_t pos = 0;
  while (pos < len) {
    char16_t word =
        !match_system2(big_endian) ? swap_bytes(data[pos]) : data[pos];
    if ((word & 0xF800) == 0xD800) {
      if (pos + 1 >= len) {
        return temporary::result(temporary::error_code::SURROGATE, pos);
      }
      char16_t diff = char16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return temporary::result(temporary::error_code::SURROGATE, pos);
      }
      char16_t next_word = !match_system2(big_endian)
                               ? swap_bytes(data[pos + 1])
                               : data[pos + 1];
      char16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return temporary::result(temporary::error_code::SURROGATE, pos);
      }
      pos += 2;
    } else {
      pos++;
    }
  }
  return temporary::result(temporary::error_code::SUCCESS, pos);
}
temporary::result validate_utf16le_with_errors(const char16_t *buf,
                                               size_t len) noexcept {
  return validate_with_errors<temporary::endianness::LITTLE>(buf, len);
}
} // namespace temporary
