#include "validate_latin1.h"

namespace simdutf {
namespace tests {
namespace reference {

simdutf_warn_unused bool validate_latin1(const char *buf, size_t len) noexcept {
  const char* curr = buf;
  const char* end = buf + len;

  while (curr != end) {
      const unsigned char word = static_cast<unsigned char>(*curr);

      if (word > 0xFF) {
        return false;
      }

      curr++;
  }

  return true;
}

} // namespace reference
} // namespace tests
} // namespace simdutf
