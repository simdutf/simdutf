#ifndef SIMDUTF_SWAP_BYTES_H
#define SIMDUTF_SWAP_BYTES_H

namespace simdutf {
namespace scalar {

simdutf_constexpr inline simdutf_warn_unused uint16_t
u16_swap_bytes(const uint16_t word) {
  return uint16_t((word >> 8) | (word << 8));
}

simdutf_constexpr inline simdutf_warn_unused uint32_t
u32_swap_bytes(const uint32_t word) {
  return ((word >> 24) & 0xff) |      // move byte 3 to byte 0
         ((word << 8) & 0xff0000) |   // move byte 1 to byte 2
         ((word >> 8) & 0xff00) |     // move byte 2 to byte 1
         ((word << 24) & 0xff000000); // byte 0 to byte 3
}

namespace utf32 {
template <endianness big_endian>
simdutf_constexpr uint32_t swap_if_needed(uint32_t c) {
  if simdutf_constexpr (!match_system(big_endian)) {
    return scalar::u32_swap_bytes(c);
  } else {
    return c;
  }
};
} // namespace utf32

namespace utf16 {
template <endianness big_endian>
simdutf_constexpr uint16_t swap_if_needed(uint16_t c) {
  if simdutf_constexpr (!match_system(big_endian)) {
    return scalar::u16_swap_bytes(c);
  } else {
    return c;
  }
};
} // namespace utf16

} // namespace scalar
} // namespace simdutf

#endif
