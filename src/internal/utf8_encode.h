#pragma once

#include <cstdint>
#include <cstddef>

namespace simdutf {
namespace internal {
namespace utf8 {

  template <typename CONSUMER>
  int encode(uint32_t value, CONSUMER consumer) {
    if (value < 0x00000080) {
      consumer(uint8_t(value));
      return 1;
    }

    if (value < 0x00000800) {
      consumer(0xc0 | uint8_t(value >> 6));
      consumer(0x80 | uint8_t(value & 0x3f));
      return 2;
    }

    if (value < 0x00010000) {
      consumer(0xe0 | uint8_t(value >> 12));
      consumer(0x80 | uint8_t((value >> 6) & 0x3f));
      consumer(0x80 | uint8_t(value & 0x3f));
      return 3;
    }

    {
      consumer(0xf0 | uint8_t(value >> 18));
      consumer(0x80 | uint8_t((value >> 12) & 0x3f));
      consumer(0x80 | uint8_t((value >> 6) & 0x3f));
      consumer(0x80 | uint8_t(value & 0x3f));
      return 4;
    }
  }

} // namespace utf8
} // namespace internal
} // namespace simdutf
