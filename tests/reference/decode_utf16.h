#pragma once

#include <cstdio>
#include "simdutf/encoding_types.h"

namespace simdutf {
namespace tests {
namespace reference {
namespace utf16 {

enum class Error {
  high_surrogate_out_of_range,
  low_surrogate_out_of_range,
  missing_low_surrogate
};

template <typename CONSUMER, typename ERROR_HANDLER>
bool decode(endianness utf16_endianness, const char16_t *codepoints,
            size_t size, CONSUMER consumer, ERROR_HANDLER error_handler) {
  const char16_t *curr = codepoints;
  const char16_t *end = codepoints + size;

  // RFC2781, chapter 2.2
  while (curr != end) {
    uint16_t W1;
    if (!match_system(utf16_endianness)) {
      W1 = char16_t((uint16_t(*curr) << 8) | (uint16_t(*curr) >> 8));
    } else {
      W1 = *curr;
    }

    curr += 1;

    if (W1 < 0xd800 ||
        W1 > 0xdfff) { // fast path, code point is equal to character's value
      consumer(W1);
      continue;
    }

    if (W1 > 0xdbff) { // W1 must be in range 0xd800 .. 0xdbff
      if (!error_handler(codepoints, curr, Error::high_surrogate_out_of_range))
        return false;

      continue;
    }

    if (curr ==
        end) { // required the next word, but we're already at the end of data
      if (!error_handler(codepoints, curr, Error::missing_low_surrogate))
        return false;

      break;
    }

    uint16_t W2;
    if (!match_system(utf16_endianness)) {
      W2 = char16_t((uint16_t(*curr) << 8) | (uint16_t(*curr) >> 8));
    } else {
      W2 = *curr;
    }
    if (W2 < 0xdc00 || W2 > 0xdfff) { // W2 = 0xdc00 .. 0xdfff
      if (!error_handler(codepoints, curr, Error::low_surrogate_out_of_range))
        return false;
    } else {
      const uint32_t hi = W1 & 0x3ff; // take lower 10 bits of W1 and W2
      const uint32_t lo = W2 & 0x3ff;
      const uint32_t tmp = lo | (hi << 10); // build a 20-bit temporary value U'

      consumer(tmp + 0x10000);
    }

    curr += 1;
  }

  return true;
}

} // namespace utf16
} // namespace reference
} // namespace tests
} // namespace simdutf
