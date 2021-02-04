#ifndef SIMDUTF_ARM64_BITMASK_H
#define SIMDUTF_ARM64_BITMASK_H

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdutf_really_inline uint64_t prefix_xor(uint64_t bitmask) {
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
}

} // unnamed namespace
} // namespace arm64
} // namespace simdutf

#endif
