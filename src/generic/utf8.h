#include "scalar/utf8.h"

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8 {

using namespace simd;

simdutf_really_inline size_t count_code_points(const char* in, size_t size) {
    size_t pos = 0;
    size_t count = 0;
    for(;pos + 64 <= size; pos += 64) {
      simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + pos));
      uint64_t utf8_continuation_mask = input.lt(-65 + 1);
      count += 64 - count_ones(utf8_continuation_mask);
    }
    return count + scalar::utf8::count_code_points(in + pos, size - pos);
}

} // utf8 namespace
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
