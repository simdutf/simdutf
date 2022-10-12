// file included directly

const char* validate_ascii(const char* buf, size_t len) {
  const char* end = buf + len;
  const __m512i ascii = _mm512_set1_epi8((uint8_t)0x80);
  __m512i running_or = _mm512_setzero_si512();
  for (; buf + 64 <= end; buf += 64) {
    const __m512i utf8 = _mm512_loadu_si512((const __m512i*)buf);
    running_or = _mm512_ternarylogic_epi32(running_or, utf8, ascii, 0xf8); // running_or | (utf8 & ascii)
  }
  if (_mm512_test_epi8_mask(running_or, running_or) != 0) {
    return nullptr;
  } else {
    return buf;
  }
}