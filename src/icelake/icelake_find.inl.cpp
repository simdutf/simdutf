simdutf_really_inline const char *util_find(const char *start, const char *end,
                                            char character) noexcept {
  // Handle empty or invalid range
  if (start >= end)
    return end;
  const size_t step = 64;
  __m512i char_vec = _mm512_set1_epi8(character);

  // Handle unaligned beginning with a masked load
  uintptr_t misalignment = reinterpret_cast<uintptr_t>(start) % step;
  if (misalignment != 0) {
    size_t adjustment = step - misalignment;
    if (size_t(end - start) < adjustment) {
      adjustment = end - start;
    }
    __mmask64 load_mask = 0xFFFFFFFFFFFFFFFF >> (64 - adjustment);
    __m512i data = _mm512_maskz_loadu_epi8(
        load_mask, reinterpret_cast<const __m512i *>(start));
    __mmask64 match_mask = _mm512_cmpeq_epi8_mask(data, char_vec);

    if (match_mask != 0) {
      size_t index = _tzcnt_u64(match_mask);
      return start + index;
    }
    start += adjustment;
  }
  // Process 64 bytes (512 bits) at a time with AVX-512
  // Main loop for full 128-byte chunks
  while (size_t(end - start) >= 2 * step) {
    __m512i data1 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(start));
    __mmask64 mask1 = _mm512_cmpeq_epi8_mask(data1, char_vec);

    __m512i data2 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(start + step));
    __mmask64 mask2 = _mm512_cmpeq_epi8_mask(data2, char_vec);
    if (!_kortestz_mask64_u8(mask1, mask2)) {
      if (mask1 != 0) {
        // Found a match, return the first one
        size_t index = _tzcnt_u64(mask1);
        return start + index;
      }
      size_t index = _tzcnt_u64(mask2);
      return start + index + step;
    }
    start += 2 * step;
  }

  // Main loop for full 64-byte chunks
  while (size_t(end - start) >= step) {
    __m512i data = _mm512_loadu_si512(reinterpret_cast<const __m512i *>(start));
    __mmask64 mask = _mm512_cmpeq_epi8_mask(data, char_vec);

    if (mask != 0) {
      // Found a match, return the first one
      size_t index = _tzcnt_u64(mask);
      return start + index;
    }

    start += step;
  }

  // Handle remaining bytes with masked load
  size_t remaining = end - start;
  if (remaining > 0) {
    // Create a mask for the remaining bytes using shifted 0xFFFFFFFFFFFFFFFF
    __mmask64 load_mask = 0xFFFFFFFFFFFFFFFF >> (64 - remaining);
    __m512i data = _mm512_maskz_loadu_epi8(
        load_mask, reinterpret_cast<const __m512i *>(start));
    __mmask64 match_mask = _mm512_cmpeq_epi8_mask(data, char_vec);

    // Apply load mask to avoid false positives
    match_mask &= load_mask;

    if (match_mask != 0) {
      // Found a match in the remaining bytes
      size_t index = _tzcnt_u64(match_mask);
      return start + index;
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

  // Process 32 char16_t (64 bytes, 512 bits) at a time with AVX-512
  const size_t step = 32;
  __m512i char_vec = _mm512_set1_epi16(character);

  // Handle unaligned beginning with a masked load
  uintptr_t misalignment =
      reinterpret_cast<uintptr_t>(start) % (step * sizeof(char16_t));
  if (misalignment != 0 && misalignment % 2 == 0) {
    size_t adjustment =
        (step * sizeof(char16_t) - misalignment) / sizeof(char16_t);
    if (size_t(end - start) < adjustment) {
      adjustment = end - start;
    }
    __mmask32 load_mask = 0xFFFFFFFF >> (32 - adjustment);
    __m512i data = _mm512_maskz_loadu_epi16(
        load_mask, reinterpret_cast<const __m512i *>(start));
    __mmask32 match_mask = _mm512_cmpeq_epi16_mask(data, char_vec);

    if (match_mask != 0) {
      size_t index = _tzcnt_u32(match_mask);
      return start + index;
    }
    start += adjustment;
  }

  // Main loop for full 32-element chunks
  while (size_t(end - start) >= step) {
    __m512i data = _mm512_loadu_si512(reinterpret_cast<const __m512i *>(start));
    __mmask32 mask = _mm512_cmpeq_epi16_mask(data, char_vec);

    if (mask != 0) {
      // Found a match, return the first one
      size_t index = _tzcnt_u32(mask);
      return start + index;
    }

    start += step;
  }

  // Handle remaining elements with masked load
  size_t remaining = end - start;
  if (remaining > 0) {
    __mmask32 load_mask = 0xFFFFFFFF >> (32 - remaining);
    __m512i data = _mm512_maskz_loadu_epi16(
        load_mask, reinterpret_cast<const __m512i *>(start));
    __mmask32 match_mask = _mm512_cmpeq_epi16_mask(data, char_vec);

    if (match_mask != 0) {
      size_t index = _tzcnt_u32(match_mask);
      return start + index;
    }
  }

  return end;
}
