#include <algorithm> // std::find

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace util {

simdutf_really_inline const char *find(const char *start, const char *end,
                                       char character) noexcept {
  // Handle empty or invalid range
  if (start >= end)
    return end;
  // Align the start pointer to 64 bytes
  uintptr_t misalignment = reinterpret_cast<uintptr_t>(start) % 64;
  if (misalignment != 0) {
    size_t adjustment = 64 - misalignment;
    if (size_t(end - start) < adjustment) {
      adjustment = end - start;
    }
    for (size_t i = 0; i < adjustment; i++) {
      if (start[i] == character) {
        return start + i;
      }
    }
    start += adjustment;
  }

  // Main loop for 64-byte aligned data
  for (; size_t(end - start) >= 64; start += 64) {
    simd8x64<uint8_t> input(reinterpret_cast<const uint8_t *>(start));
    uint64_t matches = input.eq(uint8_t(character));
    if (matches != 0) {
      // Found a match, return the first one
      int index = trailing_zeroes(matches);
      return start + index;
    }
  }
  // Handle remaining bytes with scalar loop
  for (; start < end; ++start) {
    if (*start == character) {
      return start;
    }
  }
  return end;
}

simdutf_really_inline const char16_t *
find(const char16_t *start, const char16_t *end, char16_t character) noexcept {
  // Handle empty or invalid range
  if (start >= end)
    return end;
  // Align the start pointer to 64 bytes if misalignment is even
  uintptr_t misalignment = reinterpret_cast<uintptr_t>(start) % 64;
  if (misalignment != 0 && misalignment % 2 == 0) {
    size_t adjustment = (64 - misalignment) / sizeof(char16_t);
    if (size_t(end - start) < adjustment) {
      adjustment = end - start;
    }
    for (size_t i = 0; i < adjustment; i++) {
      if (start[i] == character) {
        return start + i;
      }
    }
    start += adjustment;
  }

  // Main loop for 64-byte aligned data
  for (; size_t(end - start) >= 32; start += 32) {
    simd16x32<uint16_t> input(reinterpret_cast<const uint16_t *>(start));
    uint64_t matches = input.eq(uint16_t(character));
    if (matches != 0) {
      // Found a match, return the first one
      int index = trailing_zeroes(matches) / 2;
      return start + index;
    }
  }
  // Handle remaining elements with scalar loop
  for (; start < end; ++start) {
    if (*start == character) {
      return start;
    }
  }
  return end;
}

} // namespace util
} // namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
