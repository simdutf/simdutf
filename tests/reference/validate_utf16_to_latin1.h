#include "simdutf/common_defs.h"
#include "simdutf/encoding_types.h"

namespace simdutf {
namespace tests {
namespace reference {
simdutf_warn_unused bool
validate_utf16_to_latin1(simdutf::endianness utf16_endianness,
                         const char16_t *buf, size_t len) noexcept;

}
} // namespace tests
} // namespace simdutf
