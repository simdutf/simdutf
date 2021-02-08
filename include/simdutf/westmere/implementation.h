#ifndef SIMDUTF_WESTMERE_IMPLEMENTATION_H
#define SIMDUTF_WESTMERE_IMPLEMENTATION_H

#include "simdutf/implementation.h"

// The constructor may be executed on any host, so we take care not to use SIMDUTF_TARGET_REGION
namespace simdutf {
namespace westmere {

namespace {
using namespace simdutf;
}

class implementation final : public simdutf::implementation {
public:
  simdutf_really_inline implementation() : simdutf::implementation("westmere", "Intel/AMD SSE4.2", internal::instruction_set::SSE42 | internal::instruction_set::PCLMULQDQ) {}
  simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused bool validate_utf16(const char *buf, size_t len) const noexcept final;
};

} // namespace westmere
} // namespace simdutf

#endif // SIMDUTF_WESTMERE_IMPLEMENTATION_H
