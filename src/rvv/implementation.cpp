#include "scalar/utf8.h"
#include "scalar/utf16.h"
#include "scalar/latin1.h"
#include "scalar/utf8_to_latin1/valid_utf8_to_latin1.h"
#include "scalar/utf8_to_latin1/utf8_to_latin1.h"

#include "scalar/utf16_to_utf8/valid_utf16_to_utf8.h"
#include "scalar/utf16_to_utf8/utf16_to_utf8.h"

#include "scalar/utf16_to_utf32/valid_utf16_to_utf32.h"
#include "scalar/utf16_to_utf32/utf16_to_utf32.h"

#include "scalar/utf32_to_utf8/valid_utf32_to_utf8.h"
#include "scalar/utf32_to_utf8/utf32_to_utf8.h"

#include "scalar/utf32_to_utf16/valid_utf32_to_utf16.h"
#include "scalar/utf32_to_utf16/utf32_to_utf16.h"

#include "simdutf/rvv/begin.h"
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
#ifndef SIMDUTF_RVV_H
#error "rvv.h must be included"
#endif

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

//
// Implementation-specific overrides
//
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

#include "rvv/rvv_length_from.inl.cpp"
#include "rvv/rvv_validate.inl.cpp"

#include "rvv/rvv_latin1_to.inl.cpp"
#include "rvv/rvv_utf8_to.inl.cpp"
#include "rvv/rvv_utf16_to.inl.cpp"
#include "rvv/rvv_utf32_to.inl.cpp"

simdutf_warn_unused int implementation::detect_encodings(const char *input, size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = simdutf::BOM::check_bom(input, length);
  if (bom_encoding != encoding_type::unspecified)
    return bom_encoding;
  int out = 0;
  if (validate_utf8(input, length))
    out |= encoding_type::UTF8;
  if (length % 2 == 0) {
    if (validate_utf16(reinterpret_cast<const char16_t*>(input), length/2))
      out |= encoding_type::UTF16_LE;
  }
  if (length % 4 == 0) {
    if (validate_utf32(reinterpret_cast<const char32_t*>(input), length/4))
      out |= encoding_type::UTF32_LE;
  }

  return out;
}

template<simdutf_ByteFlip bflip>
simdutf_really_inline static void rvv_change_endianness_utf16(const char16_t *src, size_t len, char16_t *dst) {
  for (size_t vl; len > 0; len -= vl, src += vl, dst += vl) {
    vl = __riscv_vsetvl_e16m8(len);
    vuint16m8_t v = __riscv_vle16_v_u16m8((uint16_t*)src, vl);
    __riscv_vse16_v_u16m8((uint16_t *)dst, simdutf_byteflip<bflip>(v, vl), vl);
  }
}

void implementation::change_endianness_utf16(const char16_t *src, size_t len, char16_t *dst) const noexcept {
  if (supports_zvbb())
    return rvv_change_endianness_utf16<simdutf_ByteFlip::ZVBB>(src, len, dst);
  else
    return rvv_change_endianness_utf16<simdutf_ByteFlip::V>(src, len, dst);
}

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/rvv/end.h"
