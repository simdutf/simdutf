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

  // each char16 yields at least one byte
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

  // each char16 yields at least one byte
  size_t count = 0;
  size_t mismatched_count = 0;

  if (scalar::utf16::is_low_surrogate<big_endian>(in[0])) {
    mismatched_count += 1;
  }

  for (; pos < (size - 1) / (2 * N) * (2 * N); pos += 2 * N) {
    __m512i current1 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos));
    if (!match_system(big_endian)) {
      current1 = _mm512_shuffle_epi8(current1, byteflip);
    }
    __m512i next1 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos + 1));
    if (!match_system(big_endian)) {
      next1 = _mm512_shuffle_epi8(next1, byteflip);
    }
    __m512i current2 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos + N));
    if (!match_system(big_endian)) {
      current2 = _mm512_shuffle_epi8(current2, byteflip);
    }
    __m512i next2 =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos + N + 1));
    if (!match_system(big_endian)) {
      next2 = _mm512_shuffle_epi8(next2, byteflip);
    }

    __m512i lb_masked1 =
        _mm512_and_si512(current1, _mm512_set1_epi16(uint16_t(0xfc00)));
    __m512i block_masked1 =
        _mm512_and_si512(next1, _mm512_set1_epi16(uint16_t(0xfc00)));
    __mmask32 lb_is_high1 = _mm512_cmpeq_epi16_mask(
        lb_masked1, _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 block_is_low1 = _mm512_cmpeq_epi16_mask(
        block_masked1, _mm512_set1_epi16(uint16_t(0xdc00)));
    __mmask32 illseq1 = _kxor_mask32(lb_is_high1, block_is_low1);
    __mmask32 is_surrogate1 = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(current1, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 c01 = _mm512_cmpneq_epi16_mask(
        _mm512_and_si512(current1, _mm512_set1_epi16(uint16_t(0xff80))),
        _mm512_setzero_si512());
    __mmask32 c11 = _mm512_cmpneq_epi16_mask(
        _mm512_and_si512(current1, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_setzero_si512());

    __m512i lb_masked2 =
        _mm512_and_si512(current2, _mm512_set1_epi16(uint16_t(0xfc00)));
    __m512i block_masked2 =
        _mm512_and_si512(next2, _mm512_set1_epi16(uint16_t(0xfc00)));
    __mmask32 lb_is_high2 = _mm512_cmpeq_epi16_mask(
        lb_masked2, _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 block_is_low2 = _mm512_cmpeq_epi16_mask(
        block_masked2, _mm512_set1_epi16(uint16_t(0xdc00)));
    __mmask32 illseq2 = _kxor_mask32(lb_is_high2, block_is_low2);
    __mmask32 is_surrogate2 = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(current2, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 c02 = _mm512_cmpneq_epi16_mask(
        _mm512_and_si512(current2, _mm512_set1_epi16(uint16_t(0xff80))),
        _mm512_setzero_si512());
    __mmask32 c12 = _mm512_cmpneq_epi16_mask(
        _mm512_and_si512(current2, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_setzero_si512());

    count += count_ones32(c01);
    count += count_ones32(c11);
    count -= count_ones32(is_surrogate1);
    count += count_ones32(c02);
    count += count_ones32(c12);
    count -= count_ones32(is_surrogate2);
    mismatched_count += count_ones32(illseq1);
    mismatched_count += count_ones32(illseq2);
  }
  if (pos + N + 1 <= size) {
    __m512i input =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos));
    if (!match_system(big_endian)) {
      input = _mm512_shuffle_epi8(input, byteflip);
    }
    __m512i input_next =
        _mm512_loadu_si512(reinterpret_cast<const __m512i *>(in + pos + 1));
    if (!match_system(big_endian)) {
      input_next = _mm512_shuffle_epi8(input_next, byteflip);
    }

    __m512i lb_masked =
        _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xfc00)));
    __m512i block_masked =
        _mm512_and_si512(input_next, _mm512_set1_epi16(uint16_t(0xfc00)));
    __mmask32 lb_is_high =
        _mm512_cmpeq_epi16_mask(lb_masked, _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 block_is_low = _mm512_cmpeq_epi16_mask(
        block_masked, _mm512_set1_epi16(uint16_t(0xdc00)));
    __mmask32 illseq = _kxor_mask32(lb_is_high, block_is_low);
    __mmask32 is_surrogate = _mm512_cmpeq_epi16_mask(
        _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 c0 = _mm512_cmpneq_epi16_mask(
        _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xff80))),
        _mm512_setzero_si512());
    __mmask32 c1 = _mm512_cmpneq_epi16_mask(
        _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))),
        _mm512_setzero_si512());

    count += count_ones32(c0);
    count += count_ones32(c1);
    count -= count_ones32(is_surrogate);
    mismatched_count += count_ones32(illseq);
    pos += N;
  }

  __mmask32 remaining_mask =
      0xFFFFFFFFULL >>
      (32 - (size - pos)); // mask for the remaining char16 values
  // for remaining_next_mask, we start from 0x7FFFFFFFULL instead of
  // 0xFFFFFFFFULL We could also just shift right by one remaining_mask.
  __mmask32 remaining_next_mask = 0x7FFFFFFFULL >> (32 - (size - pos));
  __m512i input = _mm512_maskz_loadu_epi16(remaining_mask, in + pos);
  if (!match_system(big_endian)) {
    input = _mm512_shuffle_epi8(input, byteflip);
  }
  __m512i input_next =
      _mm512_maskz_loadu_epi16(remaining_next_mask, in + pos + 1);
  if (!match_system(big_endian)) {
    input_next = _mm512_shuffle_epi8(input_next, byteflip);
  }

  __m512i lb_masked =
      _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xfc00)));
  __m512i block_masked =
      _mm512_and_si512(input_next, _mm512_set1_epi16(uint16_t(0xfc00)));
  __mmask32 lb_is_high =
      _mm512_cmpeq_epi16_mask(lb_masked, _mm512_set1_epi16(uint16_t(0xd800)));
  __mmask32 block_is_low = _mm512_cmpeq_epi16_mask(
      block_masked, _mm512_set1_epi16(uint16_t(0xdc00)));
  __mmask32 illseq = _kxor_mask32(lb_is_high, block_is_low);
  __mmask32 is_surrogate = _mm512_cmpeq_epi16_mask(
      _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))),
      _mm512_set1_epi16(uint16_t(0xd800)));
  __mmask32 c0 = _mm512_cmpneq_epi16_mask(
      _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xff80))),
      _mm512_setzero_si512());
  __mmask32 c1 = _mm512_cmpneq_epi16_mask(
      _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))),
      _mm512_setzero_si512());

  count += count_ones32(c0);
  count += count_ones32(c1);
  count -= count_ones32(is_surrogate);
  mismatched_count += count_ones32(illseq);
  pos = size;
  count += pos;

  count += mismatched_count;
  return count;
}
