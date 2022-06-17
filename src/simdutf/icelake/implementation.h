#ifndef SIMDUTF_ICELAKE_IMPLEMENTATION_H
#define SIMDUTF_ICELAKE_IMPLEMENTATION_H

#include "simdutf/implementation.h"

namespace simdutf {
namespace icelake {

namespace {
using namespace simdutf;
}

class implementation final : public simdutf::implementation {
public:
  simdutf_really_inline implementation() : simdutf::implementation(
      "icelake",
      "Intel AVX512 (AVX-512BW, AVX-512CD, AVX-512VL, AVX-512VBMI2 extensions)",
      internal::instruction_set::AVX2 | internal::instruction_set::PCLMULQDQ | internal::instruction_set::BMI1 | internal::instruction_set::BMI2 | internal::instruction_set::AVX512BW | internal::instruction_set::AVX512CD | internal::instruction_set::AVX512VL | internal::instruction_set::AVX512VBMI2 ) {}
  simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused bool validate_ascii(const char *buf, size_t len) const noexcept final;
  simdutf_warn_unused bool validate_utf16(const char16_t *buf, size_t len) const noexcept final;
  simdutf_warn_unused bool validate_utf32(const char32_t *buf, size_t len) const noexcept final;
  simdutf_warn_unused size_t convert_utf8_to_utf16(const char * buf, size_t len, char16_t* utf16_output) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_utf8_to_utf32(const char * buf, size_t len, char32_t* utf32_output) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf8_to_utf32(const char * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_utf32_to_utf8(const char32_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf32_to_utf8(const char32_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_utf32_to_utf16(const char32_t * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf32_to_utf16(const char32_t * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_utf16_to_utf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  simdutf_warn_unused size_t convert_valid_utf16_to_utf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  simdutf_warn_unused size_t count_utf16(const char16_t * buf, size_t length) const noexcept;
  simdutf_warn_unused size_t count_utf8(const char * buf, size_t length) const noexcept;
  simdutf_warn_unused size_t utf8_length_from_utf16(const char16_t * input, size_t length) const noexcept;
  simdutf_warn_unused size_t utf32_length_from_utf16(const char16_t * input, size_t length) const noexcept;
  simdutf_warn_unused size_t utf16_length_from_utf8(const char * input, size_t length) const noexcept;
  simdutf_warn_unused size_t utf8_length_from_utf32(const char32_t * input, size_t length) const noexcept;
  simdutf_warn_unused size_t utf16_length_from_utf32(const char32_t * input, size_t length) const noexcept;
  simdutf_warn_unused size_t utf32_length_from_utf8(const char * input, size_t length) const noexcept;
};

} // namespace icelake
} // namespace simdutf

#endif // SIMDUTF_ICELAKE_IMPLEMENTATION_H
