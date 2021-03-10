#include "scalar/utf16.h"
#include <iostream>
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16 {

simdutf_really_inline size_t count_code_points(const char16_t* in, size_t size) {
    size_t pos = 0;
    size_t count = 0;
    for(;pos + 32 <= size; pos += 32) {
      simd8x64<uint16_t> input(reinterpret_cast<const uint16_t *>(in + pos));
      uint64_t not_pair = input.not_in_range(0xDC00, 0xDFFF);
      count += count_ones(not_pair) / 2;
    }
    return count + scalar::utf16::count_code_points(in + pos, size - pos);
}
} // utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
