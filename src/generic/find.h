
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace util {

simdutf_really_inline const char *find(const char *start, const char *end,
                                       char character) noexcept {
  for (; std::distance(start, end) >= 64; start += 64) {
    simd8x64<uint8_t> input(reinterpret_cast<const uint8_t *>(start));
    uint64_t matches = input.eq(uint8_t(character));
    if (matches != 0) {
      // Found a match, return the first one
      int index = trailing_zeroes(matches);
      return start + index;
    }
  }
  return std::find(start, end, character);
}

simdutf_really_inline const char16_t *
find(const char16_t *start, const char16_t *end, char16_t character) noexcept {
  for (; std::distance(start, end) >= 32; start += 32) {
    simd16x32<uint16_t> input(reinterpret_cast<const uint16_t *>(start));
    uint64_t matches = input.eq(uint16_t(character));
    if (matches != 0) {
      // Found a match, return the first one
      int index = trailing_zeroes(matches) / 2;
      return start + index;
    }
  }
  return std::find(start, end, character);
}
} // namespace util
} // namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
