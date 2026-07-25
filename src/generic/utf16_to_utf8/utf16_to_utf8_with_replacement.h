// Note: no include guard on purpose. This header is included once inside each
// SIMD kernel's translation unit (and re-expanded per kernel in the
// amalgamation), matching the other generic/ transcoder headers.
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16_to_utf8 {

// Convert possibly ill-formed UTF-16 to UTF-8, substituting each unpaired
// surrogate with U+FFFD (0xEF 0xBF 0xBD). Runs the SIMD *_with_errors converter
// at full speed and only pays extra where an unpaired surrogate is found.
//
// convert_with_errors behaves like convert_utf16{le,be}_to_utf8_with_errors: on
// SUCCESS, result.count is the number of UTF-8 bytes written; on a SURROGATE
// error, result.count is the index of the first unpaired surrogate.
// utf8_length is utf8_length_from_utf16{le,be}; only ever called on a prefix
// already proved valid, so it matches the bytes just written.
template <typename ConvertWithErrors, typename Utf8Length>
simdutf_really_inline size_t convert_with_replacement_via(
    ConvertWithErrors convert_with_errors, Utf8Length utf8_length,
    const char16_t *buf, size_t len, char *utf8_output) {
  char *const start = utf8_output;
  size_t pos = 0;
  while (pos < len) {
    result r = convert_with_errors(buf + pos, len - pos, utf8_output);
    if (r.error != error_code::SURROGATE) {
      utf8_output += r.count; // SUCCESS: r.count == UTF-8 bytes written
      break;
    }
    // buf[pos + r.count] is unpaired; the valid prefix is already written.
    const size_t valid_units = r.count;
    utf8_output += utf8_length(buf + pos, valid_units);
    pos += valid_units;
    // Emit U+FFFD and skip the offending code unit.
    utf8_output[0] = char(0xef);
    utf8_output[1] = char(0xbf);
    utf8_output[2] = char(0xbd);
    utf8_output += 3;
    pos += 1;
  }
  return size_t(utf8_output - start);
}

} // namespace utf16_to_utf8
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
