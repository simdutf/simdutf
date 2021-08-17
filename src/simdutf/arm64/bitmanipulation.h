#ifndef SIMDUTF_ARM64_BITMANIPULATION_H
#define SIMDUTF_ARM64_BITMANIPULATION_H

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

/* result might be undefined when input_num is zero */
simdutf_really_inline int count_ones(uint64_t input_num) {
   return vaddv_u8(vcnt_u8(vcreate_u8(input_num)));
}

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#endif // SIMDUTF_ARM64_BITMANIPULATION_H
