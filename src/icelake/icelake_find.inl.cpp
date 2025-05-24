
simdutf_really_inline const char *util_find(const char *start, const char *end,
                                            char character) noexcept {
  // Handle empty or invalid range
  if (start >= end)
    return end;

  // Process 64 bytes (512 bits) at a time with AVX-512
  const size_t step = 64;
  __m512i char_vec = _mm512_set1_epi8(character);

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

  // Main loop for full 32-element chunks
  while (size_t(end - start) >= step) {
    __m512i data = _mm512_loadu_si512(reinterpret_cast<const __m512i *>(start));
    __mmask32 mask = _mm512_cmpeq_epi16_mask(data, char_vec);

    if (mask != 0) {
      // Found a match, return the first one
      size_t index = _tzcnt_u64(mask);
      return start + index;
    }

    start += step;
  }

  // Handle remaining elements with masked load
  size_t remaining = end - start;
  if (remaining > 0) {
    // Create a mask for the remaining elements using shifted 0xFFFFFFFF
    __mmask32 load_mask = 0xFFFFFFFF >> (32 - remaining);
    __m512i data = _mm512_maskz_loadu_epi16(
        load_mask, reinterpret_cast<const __m512i *>(start));
    __mmask32 match_mask = _mm512_cmpeq_epi16_mask(data, char_vec);

    // Apply load mask to avoid false positives
    match_mask &= load_mask;

    if (match_mask != 0) {
      // Found a match in the remaining elements
      size_t index = _tzcnt_u64(match_mask);
      return start + index;
    }
  }

  return end;
}
