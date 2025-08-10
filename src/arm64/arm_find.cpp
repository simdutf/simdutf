simdutf_really_inline const char *util_find(const char *start, const char *end,
                                            char character) noexcept {
  // Handle empty or invalid range
  if (start >= end)
    return end;

  const size_t widestep = 64;
  const size_t step = 16;
  uint8x16_t char_vec = vdupq_n_u8(static_cast<uint8_t>(character));

  // Handle unaligned beginning
  uintptr_t misalignment = reinterpret_cast<uintptr_t>(start) % step;
  if (misalignment != 0) {
    size_t adjustment = step - misalignment;
    if (size_t(end - start) < adjustment) {
      adjustment = end - start;
    }
    for (size_t i = 0; i < adjustment; ++i) {
      if (start[i] == character) {
        return start + i;
      }
    }
    start += adjustment;
  }

  // Main loop for full 64-byte chunks
  while (size_t(end - start) >= widestep) {
    uint8x16_t data1 = vld1q_u8(reinterpret_cast<const uint8_t *>(start));
    uint8x16_t data2 = vld1q_u8(reinterpret_cast<const uint8_t *>(start) + 16);
    uint8x16_t data3 = vld1q_u8(reinterpret_cast<const uint8_t *>(start) + 32);
    uint8x16_t data4 = vld1q_u8(reinterpret_cast<const uint8_t *>(start) + 48);

    uint8x16_t cmp1 = vceqq_u8(data1, char_vec);
    uint8x16_t cmp2 = vceqq_u8(data2, char_vec);
    uint8x16_t cmp3 = vceqq_u8(data3, char_vec);
    uint8x16_t cmp4 = vceqq_u8(data4, char_vec);
    uint8x16_t cmpall = vorrq_u8(vorrq_u8(cmp1, cmp2), vorrq_u8(cmp3, cmp4));

    uint64_t mask = vget_lane_u64(
        vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmpall), 4)), 0);

    if (mask != 0) {
      // Found a match, return the first one
      uint64_t mask1 = vget_lane_u64(
          vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp1), 4)), 0);
      if (mask1 != 0) {
        // Found a match in the first chunk
        int index = trailing_zeroes(mask1) / 4; // Each character maps to 4 bits
        return start + index;
      }
      uint64_t mask2 = vget_lane_u64(
          vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp2), 4)), 0);
      if (mask2 != 0) {
        // Found a match in the second chunk
        int index = trailing_zeroes(mask2) / 4; // Each character maps to 4 bits
        return start + index + 16;
      }
      uint64_t mask3 = vget_lane_u64(
          vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp3), 4)), 0);
      if (mask3 != 0) {
        // Found a match in the third chunk
        int index = trailing_zeroes(mask3) / 4; // Each character maps to 4 bits
        return start + index + 32;
      }
      uint64_t mask4 = vget_lane_u64(
          vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp4), 4)), 0);
      if (mask4 != 0) {
        // Found a match in the fourth chunk
        int index = trailing_zeroes(mask4) / 4; // Each character maps to 4 bits
        return start + index + 48;
      }
    }

    start += widestep;
  }

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

  const size_t step = 8;
  uint16x8_t char_vec = vdupq_n_u16(character);

  // Handle unaligned beginning
  uintptr_t misalignment =
      reinterpret_cast<uintptr_t>(start) % (step * sizeof(char16_t));
  if (misalignment != 0 && misalignment % 2 == 0) {
    size_t adjustment =
        (step * sizeof(char16_t) - misalignment) / sizeof(char16_t);
    if (size_t(end - start) < adjustment) {
      adjustment = end - start;
    }
    for (size_t i = 0; i < adjustment; ++i) {
      if (start[i] == character) {
        return start + i;
      }
    }
    start += adjustment;
  }

  // Main loop for full 8-element chunks with unrolling
  while (size_t(end - start) >= 4 * step) {
    uint16x8_t data1 = vld1q_u16(reinterpret_cast<const uint16_t *>(start));
    uint16x8_t data2 =
        vld1q_u16(reinterpret_cast<const uint16_t *>(start) + step);
    uint16x8_t data3 =
        vld1q_u16(reinterpret_cast<const uint16_t *>(start) + 2 * step);
    uint16x8_t data4 =
        vld1q_u16(reinterpret_cast<const uint16_t *>(start) + 3 * step);

    uint16x8_t cmp1 = vceqq_u16(data1, char_vec);
    uint16x8_t cmp2 = vceqq_u16(data2, char_vec);
    uint16x8_t cmp3 = vceqq_u16(data3, char_vec);
    uint16x8_t cmp4 = vceqq_u16(data4, char_vec);

    uint64_t mask1 = vget_lane_u64(
        vreinterpret_u64_u16(vshrn_n_u32(vreinterpretq_u32_u16(cmp1), 4)), 0);
    if (mask1 != 0) {
      int index = trailing_zeroes(mask1) / 8;
      return start + index;
    }

    uint64_t mask2 = vget_lane_u64(
        vreinterpret_u64_u16(vshrn_n_u32(vreinterpretq_u32_u16(cmp2), 4)), 0);
    if (mask2 != 0) {
      int index = trailing_zeroes(mask2) / 8;
      return start + index + step;
    }

    uint64_t mask3 = vget_lane_u64(
        vreinterpret_u64_u16(vshrn_n_u32(vreinterpretq_u32_u16(cmp3), 4)), 0);
    if (mask3 != 0) {
      int index = trailing_zeroes(mask3) / 8;
      return start + index + 2 * step;
    }

    uint64_t mask4 = vget_lane_u64(
        vreinterpret_u64_u16(vshrn_n_u32(vreinterpretq_u32_u16(cmp4), 4)), 0);
    if (mask4 != 0) {
      int index = trailing_zeroes(mask4) / 8;
      return start + index + 3 * step;
    }

    start += 4 * step;
  }

  // Main loop for full 8-element chunks
  while (size_t(end - start) >= step) {
    uint16x8_t data = vld1q_u16(reinterpret_cast<const uint16_t *>(start));
    uint16x8_t cmp = vceqq_u16(data, char_vec);
    uint64_t mask = vget_lane_u64(
        vreinterpret_u64_u16(vshrn_n_u32(vreinterpretq_u32_u16(cmp), 4)), 0);

    if (mask != 0) {
      int index = trailing_zeroes(mask) / 8;
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
