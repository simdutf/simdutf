#ifndef SIMDUTF_FALLBACK_BITMANIPULATION_H
#define SIMDUTF_FALLBACK_BITMANIPULATION_H

#include "simdutf.h"
#include <limits>

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

#if defined(_MSC_VER) && !defined(_M_ARM64) && !defined(_M_X64)
static inline unsigned char _BitScanForward64(unsigned long* ret, uint64_t x) {
  unsigned long x0 = (unsigned long)x, top, bottom;
  _BitScanForward(&top, (unsigned long)(x >> 32));
  _BitScanForward(&bottom, x0);
  *ret = x0 ? bottom : 32 + top;
  return x != 0;
}
static unsigned char _BitScanReverse64(unsigned long* ret, uint64_t x) {
  unsigned long x1 = (unsigned long)(x >> 32), top, bottom;
  _BitScanReverse(&top, x1);
  _BitScanReverse(&bottom, (unsigned long)x);
  *ret = x1 ? top + 32 : bottom;
  return x != 0;
}
#endif

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#endif // SIMDUTF_FALLBACK_BITMANIPULATION_H
