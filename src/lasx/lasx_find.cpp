simdutf_really_inline const char *util_find(const char *start, const char *end,
                                            char character) noexcept {
  if (start >= end)
    return end;

  const int step = 32;
  __m256i char_vec = __lasx_xvreplgr2vr_b(static_cast<uint16_t>(character));

  while (end - start >= step) {
    __m256i data = __lasx_xvld(reinterpret_cast<const __m256i *>(start), 0);
    __m256i cmp = __lasx_xvseq_b(data, char_vec);
    if (__lasx_xbnz_v(cmp)) {
      __m256i res = __lasx_xvmsknz_b(cmp);
      uint32_t mask0 = __lasx_xvpickve2gr_wu(res, 0);
      uint32_t mask1 = __lasx_xvpickve2gr_wu(res, 4);
      uint32_t mask = (mask0 | (mask1 << 16));
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

  const int step = 16;
  __m256i char_vec = __lasx_xvreplgr2vr_h(static_cast<uint16_t>(character));

  while (end - start >= step) {
    __m256i data = __lasx_xvld(reinterpret_cast<const __m256i *>(start), 0);
    __m256i cmp = __lasx_xvseq_h(data, char_vec);
    if (__lasx_xbnz_v(cmp)) {
      __m256i res = __lasx_xvmsknz_b(cmp);
      uint32_t mask0 = __lasx_xvpickve2gr_wu(res, 0);
      uint32_t mask1 = __lasx_xvpickve2gr_wu(res, 4);
      uint32_t mask = (mask0 | (mask1 << 16));
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
