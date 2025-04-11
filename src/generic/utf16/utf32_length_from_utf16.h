namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16 {

template <endianness big_endian>
simdutf_really_inline size_t utf32_length_from_utf16(const char16_t *in,
                                                     size_t size) {
  return count_code_points<big_endian>(in, size);
}

} // namespace utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
