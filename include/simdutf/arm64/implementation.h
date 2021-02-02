#ifndef SIMDUTF_ARM64_IMPLEMENTATION_H
#define SIMDUTF_ARM64_IMPLEMENTATION_H

#include "simdutf.h"
#include "simdutf/internal/isadetection.h"

namespace simdutf {
namespace arm64 {

namespace {
using namespace simdutf;
using namespace simdutf::dom;
}

class implementation final : public simdutf::implementation {
public:
  simdutf_really_inline implementation() : simdutf::implementation("arm64", "ARM NEON", internal::instruction_set::NEON) {}
  simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace arm64
} // namespace simdutf

#endif // SIMDUTF_ARM64_IMPLEMENTATION_H
