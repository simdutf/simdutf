#include "internal/utf16_decode.h"

// Daniel: This should go in the fallback kernel TODO
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16_validation {

bool scalar_validate_utf16(const char16_t* buf, size_t len) {
    auto consumer = [](uint32_t /**/){};
    auto error_handler = [](const char16_t* /**/, const char16_t* /**/, ::simdutf::internal::utf16::Error /**/){
        return false; // do nothing, just tell the decoder to break decoding
    };
    return ::simdutf::internal::utf16::decode(buf, len, consumer, error_handler);
}

} // namespace utf16_validation
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
