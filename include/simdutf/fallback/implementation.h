#ifndef SIMDUTF_FALLBACK_IMPLEMENTATION_H
#define SIMDUTF_FALLBACK_IMPLEMENTATION_H

#include "simdutf/implementation.h"

namespace simdutf {
namespace fallback {

namespace {
using namespace simdutf;
}

class implementation final : public simdutf::implementation {
public:
  simdutf_really_inline implementation() : simdutf::implementation(
      "fallback",
      "Generic fallback implementation",
      0
  ) {}

  simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused bool validate_utf16(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused size_t convert_utf8_to_utf16(const char * buf, size_t len, char* utf16_output) const noexcept final;
};

} // namespace fallback
} // namespace simdutf

#endif // SIMDUTF_FALLBACK_IMPLEMENTATION_H
