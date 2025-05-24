
simdutf_really_inline const char *util_find(const char *start, const char *end,
                                            char character) noexcept {
  // Handle empty or invalid range
  if (start >= end)
    return end;

  // Process 16 bytes (128 bits) at a time with NEON
  const size_t step = 16;
  uint8x16_t char_vec = vdupq_n_u8(static_cast<uint8_t>(character));

  // Main loop for full 16-byte chunks
  while (size_t(end - start) >= step) {
    uint8x16_t data = vld1q_u8(reinterpret_cast<const uint8_t *>(start));
    uint8x16_t cmp = vceqq_u8(data, char_vec);
    uint64_t mask = vget_lane_u64(
        vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp), 4)), 0);

    if (mask != 0) {
      // Found a match, return the first one
      int index = trailing_zeroes(mask) / 4; // Each character maps to 4 bits
      return start + index;
    }

    start += step;
  }

  // Handle remaining bytes with scalar loop
  for (; start < end; ++start) {
    if (*start == character) {
      return start;
    }
  }

  return end;
}

simdutf_really_inline const char16_t *util_find(const char16_t *start,
                                                const char16_t *end,
                                                char16_t character) noexcept {
  // Handle empty or invalid range
  if (start >= end)
    return end;

  // Process 8 char16_t (16 bytes, 128 bits) at a time with NEON
  const size_t step = 8;
  uint16x8_t char_vec = vdupq_n_u16(character);

  // Main loop for full 8-element chunks
  while (size_t(end - start) >= step) {
    uint16x8_t data = vld1q_u16(reinterpret_cast<const uint16_t *>(start));
    uint16x8_t cmp = vceqq_u16(data, char_vec);
    uint64_t mask = vget_lane_u64(
        vreinterpret_u64_u16(vshrn_n_u32(vreinterpretq_u32_u16(cmp), 4)), 0);

    if (mask != 0) {
      // Found a match, return the first one
      int index = trailing_zeroes(mask) / 8; // Each character maps to 8 bits
      return start + index;
    }

    start += step;
  }

  // Handle remaining elements with scalar loop
  for (; start < end; ++start) {
    if (*start == character) {
      return start;
    }
  }

  return end;
}
