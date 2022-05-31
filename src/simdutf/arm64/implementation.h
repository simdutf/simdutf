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
  simdutf_warn_unused bool validate_ascii(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused bool validate_utf16(const char16_t *buf, size_t len) const noexcept final;
  simdutf_warn_unused size_t convert_utf8_to_utf16(const char * buf, size_t len, char16_t* utf16_output) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_utf32_to_utf8(const char32_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf32_to_utf8(const char32_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t count_utf16(const char16_t * buf, size_t length) const noexcept;
  simdutf_warn_unused size_t count_utf8(const char * buf, size_t length) const noexcept;
  simdutf_warn_unused size_t utf8_length_from_utf16(const char16_t * input, size_t length) const noexcept;
  simdutf_warn_unused size_t utf16_length_from_utf8(const char * input, size_t length) const noexcept;
};

} // namespace arm64
} // namespace simdutf

#endif // SIMDUTF_ARM64_IMPLEMENTATION_H
