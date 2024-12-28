// file included directly

bool validate_utf32(const char32_t *buf, size_t len) {
  if (len == 0) {
    return true;
  }
  const char32_t *end = buf + len;

  const __m512i offset = _mm512_set1_epi32((uint32_t)0xffff2000);
  __m512i currentmax = _mm512_setzero_si512();
  __m512i currentoffsetmax = _mm512_setzero_si512();

  while (buf < end - 16) {
    __m512i utf32 = _mm512_loadu_si512((const __m512i *)buf);
    buf += 16;
    currentoffsetmax =
        _mm512_max_epu32(_mm512_add_epi32(utf32, offset), currentoffsetmax);
    currentmax = _mm512_max_epu32(utf32, currentmax);
  }

  __m512i utf32 =
      _mm512_maskz_loadu_epi32(__mmask16((1 << (end - buf)) - 1), buf);
  currentoffsetmax =
      _mm512_max_epu32(_mm512_add_epi32(utf32, offset), currentoffsetmax);
  currentmax = _mm512_max_epu32(utf32, currentmax);

  const __m512i standardmax = _mm512_set1_epi32((uint32_t)0x10ffff);
  const __m512i standardoffsetmax = _mm512_set1_epi32((uint32_t)0xfffff7ff);
  __m512i is_zero =
      _mm512_xor_si512(_mm512_max_epu32(currentmax, standardmax), standardmax);
  if (_mm512_test_epi8_mask(is_zero, is_zero) != 0) {
    return false;
  }
  is_zero = _mm512_xor_si512(
      _mm512_max_epu32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
  if (_mm512_test_epi8_mask(is_zero, is_zero) != 0) {
    return false;
  }

  return true;
}
