template <endianness big_endian>
simdutf_really_inline size_t icelake_utf8_length_from_utf16(const char16_t *in,
                                                            size_t size) {

  using vector_u16 = simd16<uint16_t>;
  constexpr size_t N = vector_u16::ELEMENTS; // 32 on AVX-512
  if (N + 1 > size) {
    return scalar::utf16::utf8_length_from_utf16_with_replacement<big_endian>(
        in, size);
  } // special case for short inputs
  size_t pos = 0;

  const __m512i byteflip = _mm512_setr_epi64(
      0x0607040502030001, 0x0e0f0c0d0a0b0809, 0x0607040502030001,
      0x0e0f0c0d0a0b0809, 0x0607040502030001, 0x0e0f0c0d0a0b0809,
      0x0607040502030001, 0x0e0f0c0d0a0b0809);

  size_t count = 0;

  for (; pos < size / (2 * N) * (2 * N); pos += 2 * N) {

    __m512i input1 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos));
    __m512i input2 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos + N));

    if (!match_system(big_endian)) {
      input1 = _mm512_shuffle_epi8(input1, byteflip);
      input2 = _mm512_shuffle_epi8(input2, byteflip);
    }
    // 0xd800 .. 0xdbff - low surrogate
    // 0xdc00 .. 0xdfff - high surrogate
    __mmask32 is_surrogate1 = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(input1, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 is_surrogate2 = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(input2, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));
    // c0 - chars that yield 2- or 3-byte UTF-8 codes
    __mmask32 c01 =
        _mm512_test_epi16_mask(input1, _mm512_set1_epi16(uint16_t(0xff80)));
    __mmask32 c02 =
        _mm512_test_epi16_mask(input2, _mm512_set1_epi16(uint16_t(0xff80)));

    // c1 - chars that yield 3-byte UTF-8 codes (including surrogates)
    __mmask32 c11 =
        _mm512_test_epi16_mask(input1, _mm512_set1_epi16(uint16_t(0xf800)));
    __mmask32 c12 =
        _mm512_test_epi16_mask(input2, _mm512_set1_epi16(uint16_t(0xf800)));
    count += count_ones32(c01);
    count += count_ones32(c11);
    count -= count_ones32(is_surrogate1);
    count += count_ones32(c02);
    count += count_ones32(c12);
    count -= count_ones32(is_surrogate2);
  }
  if (pos + N <= size) {
    __m512i input =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos));
    if (!match_system(big_endian)) {
      input = _mm512_shuffle_epi8(input, byteflip);
    }
    // 0xd800 .. 0xdbff - low surrogate
    // 0xdc00 .. 0xdfff - high surrogate
    __mmask32 is_surrogate = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));

    // c0 - chars that yield 2- or 3-byte UTF-8 codes
    __mmask32 c0 =
        _mm512_test_epi16_mask(input, _mm512_set1_epi16(uint16_t(0xff80)));

    // c1 - chars that yield 3-byte UTF-8 codes (including surrogates)
    __mmask32 c1 =
        _mm512_test_epi16_mask(input, _mm512_set1_epi16(uint16_t(0xf800)));
    count += count_ones32(c0);
    count += count_ones32(c1);
    count -= count_ones32(is_surrogate);
    pos += N;
  }
  // At this point, we have processed 'pos' char16 values and we have less than
  // N remaining.
  __mmask32 remaining_mask =
      0xFFFFFFFFULL >>
      (32 - (size - pos)); // mask for the remaining char16 values
  __m512i input = _mm512_maskz_loadu_epi16(remaining_mask, in + pos);
  if (!match_system(big_endian)) {
    input = _mm512_shuffle_epi8(input, byteflip);
  }
  // 0xd800 .. 0xdbff - low surrogate
  // 0xdc00 .. 0xdfff - high surrogate
  __mmask32 is_surrogate = _mm512_cmpeq_epi16_mask(
      _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))),
      _mm512_set1_epi16(uint16_t(0xd800)));

  // c0 - chars that yield 2- or 3-byte UTF-8 codes
  __mmask32 c0 =
      _mm512_test_epi16_mask(input, _mm512_set1_epi16(uint16_t(0xff80)));

  // c1 - chars that yield 3-byte UTF-8 codes (including surrogates)
  __mmask32 c1 =
      _mm512_test_epi16_mask(input, _mm512_set1_epi16(uint16_t(0xf800)));
  count += count_ones32(c0);
  count += count_ones32(c1);
  count -= count_ones32(is_surrogate);
  pos = size;

  count += pos;
  return count;
}

template <endianness big_endian>
simdutf_really_inline size_t icelake_utf8_length_from_utf16_with_replacement(
    const char16_t *in, size_t size) {
  ///////
  // We repeat 3 times the same algorithm.
  // First, we proceed with an unrolled loop of 2*N char16 values (for speed).
  // Second, we process N char16 values.
  // Finally, we process the remaining char16 values (less than N).
  ///////
  using vector_u16 = simd16<uint16_t>;
  constexpr size_t N = vector_u16::ELEMENTS; // 32 on AVX-512
  if (N + 1 > size) {
    return scalar::utf16::utf8_length_from_utf16_with_replacement<big_endian>(
        in, size);
  } // special case for short inputs
  size_t pos = 0;

  const __m512i byteflip = _mm512_setr_epi64(
      0x0607040502030001, 0x0e0f0c0d0a0b0809, 0x0607040502030001,
      0x0e0f0c0d0a0b0809, 0x0607040502030001, 0x0e0f0c0d0a0b0809,
      0x0607040502030001, 0x0e0f0c0d0a0b0809);

  const uint32_t straddle_mask =
      match_system(big_endian) ? 0xfc00fc00 : 0x00fc00fc;
  const uint32_t straddle_pair =
      match_system(big_endian) ? 0xdc00d800 : 0x00dc00d8;

  size_t count = 0;
  // We assume all surrogates are mismatched and count here the matched
  // ones.
  size_t matches = 0;

  for (; pos < (size - 1) / (2 * N) * (2 * N); pos += 2 * N) {
    __m512i current1 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos));
    if (!match_system(big_endian)) {
      current1 = _mm512_shuffle_epi8(current1, byteflip);
    }
    __m512i current2 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos + N));
    if (!match_system(big_endian)) {
      current2 = _mm512_shuffle_epi8(current2, byteflip);
    }

    __mmask32 is_surrogate1 = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(current1, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 is_surrogate2 = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(current2, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 c01 =
        _mm512_test_epi16_mask(current1, _mm512_set1_epi16(uint16_t(0xff80)));
    __mmask32 c11 =
        _mm512_test_epi16_mask(current1, _mm512_set1_epi16(uint16_t(0xf800)));
    __mmask32 c02 =
        _mm512_test_epi16_mask(current2, _mm512_set1_epi16(uint16_t(0xff80)));
    __mmask32 c12 =
        _mm512_test_epi16_mask(current2, _mm512_set1_epi16(uint16_t(0xf800)));
    count += count_ones32(c01);
    count += count_ones32(c11);
    count += count_ones32(c02);
    count += count_ones32(c12);
    if (_kor_mask32(is_surrogate1, is_surrogate2)) {
      __m512i lb_masked1 =
          _mm512_and_si512(current1, _mm512_set1_epi16(uint16_t(0xfc00)));
      __mmask32 hi_surrogates1 = _mm512_cmpeq_epi16_mask(
          lb_masked1,
          _mm512_set1_epi16(uint16_t(0xd800)));
      __mmask32 lo_surrogates1 = _mm512_cmpeq_epi16_mask(
          lb_masked1,
          _mm512_set1_epi16(uint16_t(0xdc00)));
      __m512i lb_masked2 =
          _mm512_and_si512(current2, _mm512_set1_epi16(uint16_t(0xfc00)));
      __mmask32 hi_surrogates2 = _mm512_cmpeq_epi16_mask(
          lb_masked2,
          _mm512_set1_epi16(uint16_t(0xd800)));
      __mmask32 lo_surrogates2 = _mm512_cmpeq_epi16_mask(
          lb_masked2,
          _mm512_set1_epi16(uint16_t(0xdc00)));
      matches += count_ones32(
          _kand_mask32(_kshiftli_mask32(hi_surrogates1, 1), lo_surrogates1));
      matches += count_ones32(
          _kand_mask32(_kshiftli_mask32(hi_surrogates2, 1), lo_surrogates2));
      uint32_t straddle1, straddle2;
      memcpy(&straddle1, in + pos + 1 * N - 1, sizeof(uint32_t));
      memcpy(&straddle2, in + pos + 2 * N - 1, sizeof(uint32_t));
      matches +=
          ((straddle1 & straddle_mask) == straddle_pair) +
          ((straddle2 & straddle_mask) == straddle_pair);
    }
  }
  if (pos + N + 1 <= size) {
    __m512i input =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos));
    if (!match_system(big_endian)) {
      input = _mm512_shuffle_epi8(input, byteflip);
    }

    __mmask32 is_surrogate = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 c0 =
        _mm512_test_epi16_mask(input, _mm512_set1_epi16(uint16_t(0xff80)));
    __mmask32 c1 =
        _mm512_test_epi16_mask(input, _mm512_set1_epi16(uint16_t(0xf800)));
    count += count_ones32(c0);
    count += count_ones32(c1);
    if (is_surrogate) {
      __m512i lb_masked =
          _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xfc00)));
      __mmask32 hi_surrogates =
          _mm512_cmpeq_epi16_mask(lb_masked,
                                  _mm512_set1_epi16(uint16_t(0xd800)));
      __mmask32 lo_surrogates =
          _mm512_cmpeq_epi16_mask(lb_masked,
                                  _mm512_set1_epi16(uint16_t(0xdc00)));
      matches +=
          count_ones32(
              _kand_mask32(_kshiftli_mask32(hi_surrogates, 1), lo_surrogates));
      uint32_t straddle;
      memcpy(&straddle, in + pos + N - 1, sizeof(uint32_t));
      matches += (straddle & straddle_mask) == straddle_pair;
    }
    pos += N;
  }

  size_t overshoot = 32 - (size - pos);
  __mmask32 remaining_mask = 0xFFFFFFFFULL << overshoot;
  __m512i input =
      _mm512_maskz_loadu_epi16(remaining_mask, in + pos - overshoot);
  if (!match_system(big_endian)) {
    input = _mm512_shuffle_epi8(input, byteflip);
  }

  __mmask32 is_surrogate = _mm512_cmpeq_epi16_mask(
      _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))),
      _mm512_set1_epi16(uint16_t(0xd800)));
  __mmask32 c0 =
      _mm512_test_epi16_mask(input, _mm512_set1_epi16(uint16_t(0xff80)));
  __mmask32 c1 =
      _mm512_test_epi16_mask(input, _mm512_set1_epi16(uint16_t(0xf800)));

  count += count_ones32(c0);
  count += count_ones32(c1);
  if (is_surrogate) {
    __m512i lb_masked =
        _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xfc00)));
    __mmask32 hi_surrogates =
        _mm512_cmpeq_epi16_mask(lb_masked, _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 lo_surrogates =
        _mm512_cmpeq_epi16_mask(lb_masked, _mm512_set1_epi16(uint16_t(0xdc00)));
    matches +=
        count_ones32(
            _kand_mask32(_kshiftli_mask32(hi_surrogates, 1), lo_surrogates));
  }
  pos = size;
  count += pos;

  count -= 2 * matches;
  return count;
}
