// Note: no include guard on purpose. This header is included once inside each
// SIMD kernel's translation unit (and re-expanded per kernel in the
// amalgamation), matching the other generic/ transcoder headers.
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16_to_utf8 {

// Substitutes U+FFFD for each unpaired surrogate. convert_with_details reports
// the bytes written alongside the input position, so the converted prefix never
// has to be re-walked.
template <typename ConvertWithDetails>
simdutf_really_inline size_t convert_with_replacement_via(
    ConvertWithDetails convert_with_details, const char16_t *buf, size_t len,
    char *utf8_output) {
  char *const start = utf8_output;
  size_t pos = 0;
  while (pos < len) {
    full_result r = convert_with_details(buf + pos, len - pos, utf8_output);
    utf8_output += r.output_count;
    if (r.error != error_code::SURROGATE) {
      break;
    }
    pos += r.input_count + 1;
    utf8_output[0] = char(0xef);
    utf8_output[1] = char(0xbf);
    utf8_output[2] = char(0xbd);
    utf8_output += 3;
  }
  return size_t(utf8_output - start);
}

} // namespace utf16_to_utf8
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
