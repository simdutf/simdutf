enum class ErrorChecking { disabled, enabled };

template <ErrorChecking ec>
std::pair<const char32_t *, char *>
    simdutf_really_inline ppc64_convert_utf32_to_latin1(const char32_t *buf,
                                                        size_t len,
                                                        char *latin1_output) {
  const size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 16

  const auto high_bytes_mask = vector_u32::splat(0xFFFFFF00);

  for (size_t i = 0; i < rounded_len; i += 16) {
    const auto in1 = vector_u32::load(buf);
    const auto in2 = vector_u32::load(buf + 4);
    const auto in3 = vector_u32::load(buf + 8);
    const auto in4 = vector_u32::load(buf + 12);

    if (ec == ErrorChecking::enabled) {
      const auto combined = in1 | in2 | in3 | in4;
      const auto too_big = (combined & high_bytes_mask) != uint32_t(0);

      if (simdutf_unlikely(too_big.any())) {
        // Scalar code will carry on from the beginning of the current block
        // and report the exact error position.
        break;
      }
    }

    // Note: element #1 contains 0, and is used to mask-out elements
    const auto shlo = vector_u8(0 + 3, 4 + 3, 8 + 3, 12 + 3, 16 + 3, 20 + 3,
                                24 + 3, 28 + 3, 1, 1, 1, 1, 1, 1, 1, 1);
    const auto shhi = vector_u8(1, 1, 1, 1, 1, 1, 1, 1, 0 + 3, 4 + 3, 8 + 3,
                                12 + 3, 16 + 3, 20 + 3, 24 + 3, 28 + 3);
    const auto lo = shlo.lookup_32(as_vector_u8(in1), as_vector_u8(in2));
    const auto hi = shhi.lookup_32(as_vector_u8(in3), as_vector_u8(in4));

    const auto merged = lo | hi;

    merged.store(latin1_output);
    latin1_output += 16;
    buf += 16;
  }

  return std::make_pair(buf, latin1_output);
}
