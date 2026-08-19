// file included directly

bool validate_utf32(const char32_t *buf, size_t len) {
  if (simdutf_unlikely(len == 0)) {
    return true;
  }
  const char32_t *end = buf + len;

  const __m512i offset = _mm512_set1_epi32((uint32_t)0xffff2000);
  // Four independent accumulator pairs: in the 2x version below every block
  // fed the same two accumulators, so the vpmaxud chains were serialized.
  __m512i max0 = _mm512_setzero_si512();
  __m512i max1 = _mm512_setzero_si512();
  __m512i max2 = _mm512_setzero_si512();
  __m512i max3 = _mm512_setzero_si512();
  __m512i off0 = _mm512_setzero_si512();
  __m512i off1 = _mm512_setzero_si512();
  __m512i off2 = _mm512_setzero_si512();
  __m512i off3 = _mm512_setzero_si512();

  // Get the reads onto a 64-byte boundary: a 512-bit load whose address
  // straddles a cache line costs two accesses, and this loop is load-bound.
  // There is no state carried between blocks, so the head is simply a shorter
  // first block: the zero fill of a masked load is itself a valid code point
  // and can raise neither maximum.
  if (len >= 16) {
    const uintptr_t misalignment = reinterpret_cast<uintptr_t>(buf) % 64;
    if (misalignment != 0) {
      const size_t adjustment = (64 - misalignment) / sizeof(char32_t);
      const __m512i head = _mm512_maskz_loadu_epi32(
          __mmask16((1U << adjustment) - 1), (const __m512i *)buf);
      off0 = _mm512_max_epu32(_mm512_add_epi32(head, offset), off0);
      max0 = _mm512_max_epu32(head, max0);
      buf += adjustment;
    }
  }

  // Process 64 values (4x 512-bit) per iteration.
  while (end - buf >= 64) {
    __m512i utf32_1 = _mm512_loadu_si512((const __m512i *)buf);
    __m512i utf32_2 = _mm512_loadu_si512((const __m512i *)(buf + 16));
    __m512i utf32_3 = _mm512_loadu_si512((const __m512i *)(buf + 32));
    __m512i utf32_4 = _mm512_loadu_si512((const __m512i *)(buf + 48));
    buf += 64;

    off0 = _mm512_max_epu32(_mm512_add_epi32(utf32_1, offset), off0);
    max0 = _mm512_max_epu32(utf32_1, max0);
    off1 = _mm512_max_epu32(_mm512_add_epi32(utf32_2, offset), off1);
    max1 = _mm512_max_epu32(utf32_2, max1);
    off2 = _mm512_max_epu32(_mm512_add_epi32(utf32_3, offset), off2);
    max2 = _mm512_max_epu32(utf32_3, max2);
    off3 = _mm512_max_epu32(_mm512_add_epi32(utf32_4, offset), off3);
    max3 = _mm512_max_epu32(utf32_4, max3);
  }

  __m512i currentmax = _mm512_max_epu32(_mm512_max_epu32(max0, max1),
                                        _mm512_max_epu32(max2, max3));
  __m512i currentoffsetmax = _mm512_max_epu32(_mm512_max_epu32(off0, off1),
                                              _mm512_max_epu32(off2, off3));

  // Handle remaining 16-63 values
  while (end - buf >= 16) {
    __m512i utf32 = _mm512_loadu_si512((const __m512i *)buf);
    buf += 16;
    currentoffsetmax =
        _mm512_max_epu32(_mm512_add_epi32(utf32, offset), currentoffsetmax);
    currentmax = _mm512_max_epu32(utf32, currentmax);
  }

  // Handle remaining 0-15 values with masked load
  if (buf < end) {
    __m512i utf32 =
        _mm512_maskz_loadu_epi32(__mmask16((1U << (end - buf)) - 1), buf);
    currentoffsetmax =
        _mm512_max_epu32(_mm512_add_epi32(utf32, offset), currentoffsetmax);
    currentmax = _mm512_max_epu32(utf32, currentmax);
  }

  const __m512i standardmax = _mm512_set1_epi32((uint32_t)0x10ffff);
  const __m512i standardoffsetmax = _mm512_set1_epi32((uint32_t)0xfffff7ff);
  const auto outside_range = _mm512_cmpgt_epu32_mask(currentmax, standardmax);
  if (outside_range != 0) {
    return false;
  }

  const auto surrogate =
      _mm512_cmpgt_epu32_mask(currentoffsetmax, standardoffsetmax);
  if (surrogate != 0) {
    return false;
  }

  return true;
}
