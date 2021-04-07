#include "scalar/utf8_to_utf16/utf8_to_utf16.h"


namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_to_utf16 {

using namespace simd;


simdutf_warn_unused size_t convert_valid(const char* input, size_t size,
    char16_t* utf16_output) noexcept {
  // The implementation is not specific to haswell and should be moved to the generic directory.
  size_t pos = 0;
  char16_t* start{utf16_output};
  while(pos + 64 <= size) {
    // this loop could be unrolled further. For example, we could process the mask
    // far more than 64 bytes.
    //
    // For pure ASCII inputs, this function is not optimally fast because they are
    // faster ways to just check for ASCII than to compute the continuation mask.
    // However, the continuation mask is more informative. There might be a trade-off
    // involved.
    //
    simd8x64<int8_t> in(reinterpret_cast<const int8_t *>(input + pos));
    uint64_t utf8_continuation_mask = in.lt(-65 + 1);
    // -65 is 0b10111111 in two-complement's, so largest possible continuation byte
    if(utf8_continuation_mask != 0) {
      // Slow path. We hope that the compiler will recognize that this is a slow path.
      // Anything that is not a continuation mask is a 'leading byte', that is, the
      // start of a new code point.
      uint64_t utf8_leading_mask = ~utf8_continuation_mask;
      // The *start* of code points is not so useful, rather, we want the *end* of code points.
      uint64_t utf8_end_of_code_point_mask = utf8_leading_mask>>1;
      size_t max_starting_point = (pos + 64) - 12 - 1;
      while(pos <= max_starting_point) {
        size_t consumed = convert_masked_utf8_to_utf16(input + pos,
                            utf8_end_of_code_point_mask, utf16_output);
        pos += consumed;
        utf8_end_of_code_point_mask >>= consumed;
      }
    } else {
      in.store_ascii_as_utf16(utf16_output);
      utf16_output += 64;
      pos += 64;
    }
  }
  utf16_output += scalar::utf8_to_utf16::convert_valid(input + pos, size - pos, utf16_output);
  return utf16_output - start;
}


} // namespace utf8_to_utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
