#ifndef SIMDUTF_H
#define SIMDUTF_H
#include <cstring>

#include "simdutf/compiler_check.h"
#include "simdutf/common_defs.h"
#include "simdutf/encoding_types.h"
#include "simdutf/error.h"


namespace temporary {

enum endianness { LITTLE = 0, BIG = 1 };

simdutf::result validate_utf16le_with_errors(
    const char16_t * buf, std::size_t len) noexcept;
}

#endif // SIMDUTF_H
