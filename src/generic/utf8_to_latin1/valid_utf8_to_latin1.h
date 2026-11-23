namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_to_latin1 {
using namespace simd;

simdutf_really_inline size_t convert_valid(const char *in, size_t size,
                                           char *latin1_output) {
  size_t pos = 0;
  char *start{latin1_output};
  // The kernel can write up to 8 bytes past its output
  // when calling convert_1_2_byte_utf8_to_latin1_16. If you skip the last
  // 16 bytes, then it is entirely safe because 16 valid UTF-8 bytes generate
  // at least 4 bytes. We go back from the end counting 8 leading bytes, which
  // gives us a good margin.
  size_t leading_byte = 0;
  size_t margin = size;
  for (; margin > 0 && leading_byte < 8; margin--) {
    leading_byte += (int8_t(in[margin - 1]) >
                     -65); // twos complement of -65 is 1011 1111 ...
  }
  // If the input is long enough, then we have that margin is the eighth last
  // leading byte.
  const size_t safety_margin = size - margin + 1; // to avoid overruns!
  while (pos + 64 + safety_margin <= size) {
    simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + pos));
    if (input.is_ascii()) {
      input.store((int8_t *)latin1_output);
      latin1_output += 64;
      pos += 64;
    } else {
      // you might think that a for-loop would work, but under Visual Studio, it
      // is not good enough.
      uint64_t utf8_continuation_mask =
          input.lt(-65 + 1); // -64 is 1100 0000 in twos complement. Note: in
                             // this case, we also have ASCII to account for.
      pos += convert_1_2_byte_utf8_block_to_latin1(
          in + pos, ~utf8_continuation_mask, latin1_output);
    }
  }
  const size_t ascii =
      validating_transcoder::convert_ascii_tail(in, pos, size, latin1_output);
  pos += ascii;
  latin1_output += ascii;
  if (pos < size) {
    size_t howmany = scalar::utf8_to_latin1::convert_valid(in + pos, size - pos,
                                                           latin1_output);
    latin1_output += howmany;
  }
  return latin1_output - start;
}

} // namespace utf8_to_latin1
} // namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
  // namespace simdutf
