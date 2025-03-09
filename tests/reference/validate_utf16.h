#include "simdutf/common_defs.h"
#include "simdutf/encoding_types.h"

namespace simdutf {
namespace tests {
namespace reference {
// validate UTF-16
simdutf_warn_unused bool validate_utf16(endianness utf16_endianness,
                                        const char16_t *buf,
                                        size_t len) noexcept;

} // namespace reference
} // namespace tests
} // namespace simdutf
