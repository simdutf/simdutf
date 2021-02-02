#ifndef SIMDUTF_PPC64_BITMANIPULATION_H
#define SIMDUTF_PPC64_BITMANIPULATION_H

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
NO_SANITIZE_UNDEFINED
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

/* result might be undefined when input_num is zero */
simdutf_really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num - 1);
}

/* result might be undefined when input_num is zero */
simdutf_really_inline int leading_zeroes(uint64_t input_num) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB)
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif // SIMDUTF_REGULAR_VISUAL_STUDIO
}

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
simdutf_really_inline int count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num); // Visual Studio wants two underscores
}
#else
simdutf_really_inline int count_ones(uint64_t input_num) {
  return __builtin_popcountll(input_num);
}
#endif

simdutf_really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                         uint64_t *result) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  *result = value1 + value2;
  return *result < value1;
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   reinterpret_cast<unsigned long long *>(result));
#endif
}

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#endif // SIMDUTF_PPC64_BITMANIPULATION_H
