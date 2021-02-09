#ifndef SIMDUTF_ARM64_IMPLEMENTATION_H
#define SIMDUTF_ARM64_IMPLEMENTATION_H

#include "simdutf.h"
#include "simdutf/internal/isadetection.h"

namespace simdutf {
namespace arm64 {

namespace {
using namespace simdutf;
}

class implementation final : public simdutf::implementation {
public:
  simdutf_really_inline implementation() : simdutf::implementation("arm64", "ARM NEON", internal::instruction_set::NEON) {}
  simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused bool validate_utf16(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused size_t convert_utf8_to_utf16(const char * buf, size_t len, char* utf16_output) const noexcept final;
};

} // namespace arm64
} // namespace simdutf

#endif // SIMDUTF_ARM64_IMPLEMENTATION_H
