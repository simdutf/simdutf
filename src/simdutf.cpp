#include "simdutf.h"
// We include base64_tables once.
#include "tables/base64_tables.h"
#include "implementation.cpp"
#include "encoding_types.cpp"
#include "error.cpp"
// The large tables should be included once and they
// should not depend on a kernel.
#include "tables/utf8_to_utf16_tables.h"
#include "tables/utf16_to_utf8_tables.h"
// End of tables.

// The scalar routines should be included once.
#include "scalar/ascii.h"
#include "scalar/utf8.h"
#include "scalar/utf16.h"
#include "scalar/utf32.h"
#include "scalar/latin1.h"
#include "scalar/base64.h"

#include "scalar/utf32_to_utf8/valid_utf32_to_utf8.h"
#include "scalar/utf32_to_utf8/utf32_to_utf8.h"

#include "scalar/utf32_to_utf16/valid_utf32_to_utf16.h"
#include "scalar/utf32_to_utf16/utf32_to_utf16.h"

#include "scalar/utf16_to_utf8/valid_utf16_to_utf8.h"
#include "scalar/utf16_to_utf8/utf16_to_utf8.h"

#include "scalar/utf16_to_utf32/valid_utf16_to_utf32.h"
#include "scalar/utf16_to_utf32/utf16_to_utf32.h"

#include "scalar/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "scalar/utf8_to_utf16/utf8_to_utf16.h"

#include "scalar/utf8_to_utf32/valid_utf8_to_utf32.h"
#include "scalar/utf8_to_utf32/utf8_to_utf32.h"

#include "scalar/latin1_to_utf8/latin1_to_utf8.h"
#include "scalar/latin1_to_utf16/latin1_to_utf16.h"
#include "scalar/latin1_to_utf32/latin1_to_utf32.h"

#include "scalar/utf8_to_latin1/utf8_to_latin1.h"
#include "scalar/utf16_to_latin1/utf16_to_latin1.h"
#include "scalar/utf32_to_latin1/utf32_to_latin1.h"

#include "scalar/utf8_to_latin1/valid_utf8_to_latin1.h"
#include "scalar/utf16_to_latin1/valid_utf16_to_latin1.h"
#include "scalar/utf32_to_latin1/valid_utf32_to_latin1.h"

SIMDUTF_PUSH_DISABLE_WARNINGS
SIMDUTF_DISABLE_UNDESIRED_WARNINGS

#if SIMDUTF_IMPLEMENTATION_ARM64
  #include "arm64/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_FALLBACK
  #include "fallback/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_ICELAKE
  #include "icelake/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_HASWELL
  #include "haswell/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_PPC64
  #include "ppc64/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_RVV
  #include "rvv/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_WESTMERE
  #include "westmere/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_LSX
  #include "lsx/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_LASX
  #include "lasx/implementation.cpp"
#endif

SIMDUTF_POP_DISABLE_WARNINGS

namespace temporary {

bool match_system2(simdutf::endianness e) {
#if SIMDUTF_IS_BIG_ENDIAN
  return e == endianness::BIG;
#else
  return e == simdutf::endianness::LITTLE;
#endif
}
inline simdutf_warn_unused uint16_t swap_bytes(const uint16_t word) {
  return uint16_t((word >> 8) | (word << 8));
}
template <simdutf::endianness big_endian>
inline simdutf_warn_unused simdutf::result
validate_with_errors(const char16_t *data, size_t len) noexcept {
  size_t pos = 0;
  while (pos < len) {
    char16_t word =
        !match_system(big_endian) ? swap_bytes(data[pos]) : data[pos];
    if ((word & 0xF800) == 0xD800) {
      if (pos + 1 >= len) {
        return simdutf::result(simdutf::error_code::SURROGATE, pos);
      }
      char16_t diff = char16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return simdutf::result(simdutf::error_code::SURROGATE, pos);
      }
      char16_t next_word =
          !match_system(big_endian) ? swap_bytes(data[pos + 1]) : data[pos + 1];
      char16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return simdutf::result(simdutf::error_code::SURROGATE, pos);
      }
      pos += 2;
    } else {
      pos++;
    }
  }
  return simdutf::result(simdutf::error_code::SUCCESS, pos);
}
simdutf_warn_unused simdutf::result
validate_utf16le_with_errors(const char16_t *buf, size_t len) noexcept {
  return validate_with_errors<simdutf::endianness::LITTLE>(buf, len);
}
} // namespace temporary
