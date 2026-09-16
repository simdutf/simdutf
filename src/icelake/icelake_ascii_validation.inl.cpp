// file included directly

bool validate_ascii(const char *buf, size_t len) {
  const char *end = buf + len;
  const __m512i ascii = _mm512_set1_epi8((uint8_t)0x80);
  // Four accumulators so the loads are not serialized behind a single
  // loop-carried vpternlogd, and 64-byte aligned reads: a 512-bit load whose
  // address straddles a cache line costs two accesses, and this loop does
  // nothing but load.
  __m512i or0 = _mm512_setzero_si512();
  __m512i or1 = _mm512_setzero_si512();
  __m512i or2 = _mm512_setzero_si512();
  __m512i or3 = _mm512_setzero_si512();
  // Reach the next 64-byte boundary with a masked load. There is no
  // cross-block state here and the zero fill is itself ASCII, so this is
  // simply a shorter first block.
  if (len >= 64) {
    const uintptr_t misalignment = reinterpret_cast<uintptr_t>(buf) % 64;
    if (misalignment != 0) {
      const size_t adjustment = 64 - misalignment;
      const __m512i head = _mm512_maskz_loadu_epi8(
          ~UINT64_C(0) >> (64 - adjustment), (const __m512i *)buf);
      or0 = _mm512_ternarylogic_epi32(or0, head, ascii, 0xf8);
      buf += adjustment;
    }
  }
  for (; end - buf >= 256; buf += 256) {
    or0 = _mm512_ternarylogic_epi32(
        or0, _mm512_loadu_si512((const __m512i *)buf), ascii, 0xf8);
    or1 = _mm512_ternarylogic_epi32(
        or1, _mm512_loadu_si512((const __m512i *)(buf + 64)), ascii, 0xf8);
    or2 = _mm512_ternarylogic_epi32(
        or2, _mm512_loadu_si512((const __m512i *)(buf + 128)), ascii, 0xf8);
    or3 = _mm512_ternarylogic_epi32(
        or3, _mm512_loadu_si512((const __m512i *)(buf + 192)), ascii, 0xf8);
  }
  for (; end - buf >= 64; buf += 64) {
    const __m512i utf8 = _mm512_loadu_si512((const __m512i *)buf);
    or0 = _mm512_ternarylogic_epi32(or0, utf8, ascii,
                                    0xf8); // or0 | (utf8 & ascii)
  }
  if (buf < end) {
    const __m512i utf8 = _mm512_maskz_loadu_epi8(
        (uint64_t(1) << (end - buf)) - 1, (const __m512i *)buf);
    or0 = _mm512_ternarylogic_epi32(or0, utf8, ascii,
                                    0xf8); // or0 | (utf8 & ascii)
  }
  const __m512i running_or =
      _mm512_or_si512(_mm512_or_si512(or0, or1), _mm512_or_si512(or2, or3));
  return (_mm512_test_epi8_mask(running_or, running_or) == 0);
}
