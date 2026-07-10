#ifndef SIMDUTF_ARM64_BITMANIPULATION_H
#define SIMDUTF_ARM64_BITMANIPULATION_H

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

/* result might be undefined when input_num is zero */
simdutf_really_inline int count_ones(uint64_t input_num) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  return vaddv_u8(vcnt_u8(vcreate_u8(input_num)));
#else
  // if the system supports SVE or CSSC, __builtin_popcountll
  // might be compiled to fewer single instructions. For CSSC,
  // __builtin_popcountll is compiled to a single instruction.
  return __builtin_popcountll(input_num);
#endif
}

#if SIMDUTF_NEED_TRAILING_ZEROES
simdutf_really_inline int trailing_zeroes(uint64_t input_num) {
  #ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  unsigned long ret;
  // Search the mask data from least significant bit (LSB)
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
  #else  // SIMDUTF_REGULAR_VISUAL_STUDIO
  return __builtin_ctzll(input_num);
  #endif // SIMDUTF_REGULAR_VISUAL_STUDIO
}
#endif
template <typename T> T clear_least_significant_bit(T x) {
  return (x & (x - 1));
}

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#endif // SIMDUTF_ARM64_BITMANIPULATION_H
