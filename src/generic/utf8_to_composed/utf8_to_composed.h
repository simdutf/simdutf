namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_to_composed {
using namespace simd;

template <ComposedForm form>
size_t normalize(const char *in, size_t length, char *out) {
  char **out_ptr = &out;
  char *start{out};

  // Rather large safety margin. This is in place so that oversized store
  // operations (i.e. arm_memcpy_small) are safe.
  constexpr const size_t SAFETY_MARGIN = 64;
  uint8_t last_ccc = 0;
  size_t p = 0;
  while (p + 64 + SAFETY_MARGIN <= length) {
    simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + p));
    uint64_t utf8_continuation_mask = input.lt(-65 + 1);
    uint64_t utf8_leading_mask = ~utf8_continuation_mask;
    uint64_t mask = utf8_leading_mask >> 1;
    // This gives better throughput on ASCII than if we use the `is_ascii`
    // method of simd8x64.
    if (mask == 0x7FFFFFFFFFFFFFFF) {
      input.store(reinterpret_cast<int8_t *>(*out_ptr));
      p += 63;
      *out_ptr += 63;
      last_ccc = 0;
      continue;
    }
    size_t pmax = (p + 64) - 12;
    while (p < pmax) {
      size_t consumed = normalize_masked_utf8_to_composed<form>(
          reinterpret_cast<const uint8_t *>(in + p),
          reinterpret_cast<const uint8_t *>(in), length, mask,
          reinterpret_cast<uint8_t **>(out_ptr), *out_ptr - start, &last_ccc);
      p += consumed;
      mask >>= consumed;
    }
  }

  if (p < length) {
    (void)scalar::utf8_to_composed::normalize_with_context<form>(
        in + p, in, length, out_ptr, length - p);
  }

  return *out_ptr - start;
}

template <ComposedForm form>
bool check(const char *in, size_t length, size_t *output_length) {
  *output_length = 0;
  bool is_qc = true;

  constexpr const size_t SAFETY_MARGIN = 64;
  size_t p = 0;
  uint8_t last_ccc = 0;
  while (p + 64 + SAFETY_MARGIN <= length) {
    // ASCII fast path
    simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + p));
    uint64_t utf8_continuation_mask = input.lt(-65 + 1);
    uint64_t utf8_leading_mask = ~utf8_continuation_mask;
    uint64_t mask = utf8_leading_mask >> 1;
    if (mask == 0x7FFFFFFFFFFFFFFF) {
      p += 63;
      *output_length += 63;
      last_ccc = 0;
      continue;
    }
    size_t pmax = (p + 64) - 12;
    while (p < pmax) {
      size_t consumed = normalize_masked_utf8_to_composed_check<form>(
          reinterpret_cast<const uint8_t *>(in + p), mask, output_length,
          &is_qc, &last_ccc);
      p += consumed;
      mask >>= consumed;
    }
  }

  // Check the rest using scalar code
  if (p < length) {
    is_qc &= scalar::utf8_to_composed::check_with_context<form>(
        in + p, length - p, output_length, &last_ccc);
  }
  return is_qc;
}

} // namespace utf8_to_composed

} // namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
