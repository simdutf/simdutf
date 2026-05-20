#ifndef SIMDUTF_VALIDATE_UTF8_FOR_UTF16_TRANSCODING_H
#define SIMDUTF_VALIDATE_UTF8_FOR_UTF16_TRANSCODING_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_utf16 {

simdutf_really_inline simdutf_warn_unused full_result
validate_utf8_for_utf16_transcoding(const char *buf, size_t len) noexcept {
  utf8_result res = scalar::utf8::validate_utf8_with_counts(buf, len);
  return full_result(res.error, res.input_count,
                     res.input_count - res.continuation_count +
                         res.four_byte_count);
}

} // namespace utf8_to_utf16
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
