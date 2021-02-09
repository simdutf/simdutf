#ifndef SIMDUTF_HASWELL_IMPLEMENTATION_H
#define SIMDUTF_HASWELL_IMPLEMENTATION_H

#include "simdutf/implementation.h"

// The constructor may be executed on any host, so we take care not to use SIMDUTF_TARGET_REGION
namespace simdutf {
namespace haswell {

using namespace simdutf;

class implementation final : public simdutf::implementation {
public:
  simdutf_really_inline implementation() : simdutf::implementation(
      "haswell",
      "Intel/AMD AVX2",
      internal::instruction_set::AVX2 | internal::instruction_set::PCLMULQDQ | internal::instruction_set::BMI1 | internal::instruction_set::BMI2
  ) {}
  simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused bool validate_utf16(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused size_t convert_utf8_to_utf16(const char * buf, size_t len, char* utf16_output) const noexcept final;
};

} // namespace haswell
} // namespace simdutf

#endif // SIMDUTF_HASWELL_IMPLEMENTATION_H
