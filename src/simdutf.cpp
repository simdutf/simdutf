#include "simdutf.h"

#include <cstdint>

namespace temporary {

std::size_t validate_utf16le_with_errors(const char16_t *data,
                                         size_t len) noexcept {
  size_t pos = 0;
  while (pos < len) {
    char16_t word = data[pos];
    if ((word & 0xF800) == 0xD800) {
      if (pos + 1 >= len) {
        return pos;
      }
      char16_t diff = char16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return pos;
      }
      char16_t next_word = data[pos + 1];
      char16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return pos;
      }
      pos += 2;
    } else {
      pos++;
    }
  }
  return pos;
}

std::vector<char16_t> get_test_data() { return {34086, 23924, 65167}; }

} // namespace temporary
