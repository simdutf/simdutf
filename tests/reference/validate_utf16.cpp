#include "validate_utf16.h"

namespace simdutf {
namespace tests {
namespace reference {

simdutf_warn_unused bool validate_utf16(const char16_t *buf, size_t len) noexcept {
  const char16_t* curr = buf;
  const char16_t* end = buf + len;

  while (curr != end) {
      const uint16_t W1 = *curr;
      curr += 1;

      if (W1 < 0xd800 || W1 > 0xdfff) { // fast path, code point is equal to character's value
        continue;
      }

      if (W1 > 0xdbff) { // W1 must be in range 0xd800 .. 0xdbff
        return false;
      }

      if (curr == end) { // required the next word, but we're already at the end of data
        return false;
      }

      const uint16_t W2 = *curr;
      if (W2 < 0xdc00 || W2 > 0xdfff) // W2 = 0xdc00 .. 0xdfff
        return false;

      curr += 1;
  }

  return true;
}

} // namespace reference
} // namespace tests
} // namespace simdutf


