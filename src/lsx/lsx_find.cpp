simdutf_really_inline const char *util_find(const char *start, const char *end,
                                            char character) noexcept {
  if (start >= end)
    return end;

  const int step = 16;
  __m128i char_vec = __lsx_vreplgr2vr_b(static_cast<uint8_t>(character));

  while (end - start >= step) {
    __m128i data = __lsx_vld(reinterpret_cast<const __m128i *>(start), 0);
    __m128i cmp = __lsx_vseq_b(data, char_vec);
    if (__lsx_bnz_v(cmp)) {
      uint16_t mask =
          static_cast<uint16_t>(__lsx_vpickve2gr_hu(__lsx_vmsknz_b(cmp), 0));
      return start + trailing_zeroes(mask);
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
  if (start >= end)
    return end;

  const int step = 8;
  __m128i char_vec = __lsx_vreplgr2vr_h(static_cast<uint16_t>(character));

  while (end - start >= step) {
    __m128i data = __lsx_vld(reinterpret_cast<const __m128i *>(start), 0);
    __m128i cmp = __lsx_vseq_h(data, char_vec);
    if (__lsx_bnz_v(cmp)) {
      uint16_t mask =
          static_cast<uint16_t>(__lsx_vpickve2gr_hu(__lsx_vmsknz_b(cmp), 0));
      return start + trailing_zeroes(mask) / 2;
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
