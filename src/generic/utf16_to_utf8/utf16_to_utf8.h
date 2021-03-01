#include "internal/utf16_decode.h"
#include "internal/utf8_encode.h"

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16_to_utf8 {

size_t scalar_convert(const char16_t* buf, size_t len, char* utf8_output) {
  auto consumer = [&utf8_output](uint32_t value) {
    ::simdutf::internal::utf8::encode(value, [&utf8_output](uint8_t byte) {
      *utf8_output++ = byte;
    });
  };

  auto error_handler = [](const char16_t* /**/, const char16_t* /**/, ::simdutf::internal::utf16::Error /**/){
    return false; // do nothing, just tell the decoder to break decoding
  };

  char* start = utf8_output; // Note: utf8_output is modified inside consumer
  if (::simdutf::internal::utf16::decode(buf, len, consumer, error_handler))
    return utf8_output - start;
  else
    return 0;
}

} // utf8_to_utf16 namespace
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
