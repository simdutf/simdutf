namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_to_utf32 {

using namespace simd;

simdutf_warn_unused size_t convert_valid(const char *input, size_t size,
                                         char32_t *utf32_output) noexcept {
  size_t pos = 0;
  char32_t *start{utf32_output};
  // Two margins, both proved in formal/safety_margins for the kernels that
  // compile this header, on the valid input the contract requires. The fixed
  // margin counts bytes. The dynamic margin counts leading bytes back from
  // the end of the input, as in utf8_to_utf32.h; it is at least
  // leading_bytes + 1 bytes, and it is smaller than the fixed one on text of
  // short characters. A block runs if either admits it. The backward scan
  // costs a few nanoseconds, so it runs at most once, only when the fixed
  // margin stops the loop and the dynamic one could still admit a block.
  // Where the fixed margin is at most leading_bytes + 1, there is no scan.
#if defined(SIMDUTF_IS_LSX) || defined(SIMDUTF_IS_LASX)
  constexpr size_t fixed_margin = 4;
  constexpr size_t leading_bytes = 2;
#else
  constexpr size_t fixed_margin = 12;
  constexpr size_t leading_bytes = 3;
#endif
  size_t safety_margin = fixed_margin;
  bool scanned = fixed_margin <= leading_bytes + 1;
  while (true) {
    if (pos + 64 + safety_margin > size) {
      if (scanned || pos + 64 + leading_bytes + 1 > size) {
        break;
      }
      scanned = true;
      size_t leading_byte = 0;
      size_t margin = size;
      for (; margin > 0 && leading_byte < leading_bytes; margin--) {
        leading_byte += (int8_t(input[margin - 1]) > -65);
      }
      // If the input is long enough, margin is the position of the
      // leading_bytes-th last leading byte.
      const size_t dynamic_margin = size - margin + 1;
      if (pos + 64 + dynamic_margin > size) {
        break;
      }
      safety_margin = dynamic_margin;
    }
    simd8x64<int8_t> in(reinterpret_cast<const int8_t *>(input + pos));
    if (in.is_ascii()) {
      in.store_ascii_as_utf32(utf32_output);
      utf32_output += 64;
      pos += 64;
    } else {
      // -65 is 0b10111111 in two-complement's, so largest possible continuation
      // byte
      uint64_t utf8_continuation_mask = in.lt(-65 + 1);
      uint64_t utf8_leading_mask = ~utf8_continuation_mask;
      uint64_t utf8_end_of_code_point_mask = utf8_leading_mask >> 1;
      size_t max_starting_point = (pos + 64) - 12;
      while (pos < max_starting_point) {
        size_t consumed = convert_masked_utf8_to_utf32(
            input + pos, utf8_end_of_code_point_mask, utf32_output);
        pos += consumed;
        utf8_end_of_code_point_mask >>= consumed;
      }
    }
  }
  utf32_output += scalar::utf8_to_utf32::convert_valid(input + pos, size - pos,
                                                       utf32_output);
  return utf32_output - start;
}

} // namespace utf8_to_utf32
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
