namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16 {

simdutf_really_inline void
change_endianness_utf16(const char16_t *in, size_t size, char16_t *output) {
  size_t pos = 0;

  while (pos < size / 32 * 32) {
    simd16x32<uint16_t> input(reinterpret_cast<const uint16_t *>(in + pos));
    input.swap_bytes();
    input.store(reinterpret_cast<uint16_t *>(output));
    pos += 32;
    output += 32;
  }

  scalar::utf16::change_endianness_utf16(in + pos, size - pos, output);
}

} // namespace utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
