
const char32_t *arm_validate_utf32le(const char32_t *input, size_t size) {
  const char32_t *end = input + size;

  const uint32x4_t standardmax = vmovq_n_u32(0x10ffff);
  const uint32x4_t offset = vmovq_n_u32(0xffff2000);
  const uint32x4_t standardoffsetmax = vmovq_n_u32(0xfffff7ff);
  uint32x4_t currentmax = vmovq_n_u32(0x0);
  uint32x4_t currentoffsetmax = vmovq_n_u32(0x0);

  while (end - input >= 4) {
    const uint32x4_t in = vld1q_u32(reinterpret_cast<const uint32_t *>(input));
    currentmax = vmaxq_u32(in, currentmax);
    currentoffsetmax = vmaxq_u32(vaddq_u32(in, offset), currentoffsetmax);
    input += 4;
  }

  const uint32x4_t too_large = vcgtq_u32(currentmax, standardmax);
  const uint32x4_t surrogate = vcgtq_u32(currentoffsetmax, standardoffsetmax);
  if (any_lane_set(vreinterpretq_u16_u32(vorrq_u32(too_large, surrogate)))) {
    return nullptr;
  }

  return input;
}

const result arm_validate_utf32le_with_errors(const char32_t *input,
                                              size_t size) {
  const char32_t *start = input;
  const char32_t *end = input + size;

  const uint32x4_t standardmax = vmovq_n_u32(0x10ffff);
  const uint32x4_t offset = vmovq_n_u32(0xffff2000);
  const uint32x4_t standardoffsetmax = vmovq_n_u32(0xfffff7ff);
  uint32x4_t currentmax = vmovq_n_u32(0x0);
  uint32x4_t currentoffsetmax = vmovq_n_u32(0x0);

  while (end - input >= 4) {
    const uint32x4_t in = vld1q_u32(reinterpret_cast<const uint32_t *>(input));
    currentmax = vmaxq_u32(in, currentmax);
    currentoffsetmax = vmaxq_u32(vaddq_u32(in, offset), currentoffsetmax);

    // Both accumulators are running maxima, so a single test per iteration is
    // enough: we only need to tell the two error kinds apart once we know that
    // one of them occurred.
    const uint32x4_t too_large = vcgtq_u32(currentmax, standardmax);
    const uint32x4_t surrogate = vcgtq_u32(currentoffsetmax, standardoffsetmax);
    if (simdutf_unlikely(any_lane_set(
            vreinterpretq_u16_u32(vorrq_u32(too_large, surrogate))))) {
      if (any_lane_set(vreinterpretq_u16_u32(too_large))) {
        return result(error_code::TOO_LARGE, input - start);
      }
      return result(error_code::SURROGATE, input - start);
    }

    input += 4;
  }

  return result(error_code::SUCCESS, input - start);
}
