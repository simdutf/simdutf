// This is translation of `utf8_length_from_utf16_bytemask` from
// `generic/utf16.h`
template <endianness big_endian>
simdutf_really_inline size_t icelake_utf8_length_from_utf16(const char16_t *in,
                                                            size_t size) {
  size_t pos = 0;

  using vector_u16 = simd16<uint16_t>;
  constexpr size_t N = vector_u16::ELEMENTS;

  const auto one = vector_u16::splat(1);

  auto v_count = vector_u16::zero();

  // each char16 yields at least one byte
  size_t count = size / N * N;

  // in a single iteration the increment is 0, 1 or 2, despite we have
  // three additions
  constexpr size_t max_iterations = 65535 / 2;
  size_t iteration = max_iterations;

  for (; pos < size / N * N; pos += N) {
    auto input = vector_u16::load(reinterpret_cast<const uint16_t *>(in + pos));
    if (!match_system(big_endian)) {
      input = input.swap_bytes();
    }

    // not_surrogate[i] = non-zero if i-th element is not a surrogate word
    const auto not_surrogate = (input & uint16_t(0xf800)) ^ uint16_t(0xd800);

    // not_surrogate[i] = 1 if surrogate word, 0 otherwise
    const auto is_surrogate = min(not_surrogate, one) ^ one;

    // c0 - chars that yield 2- or 3-byte UTF-8 codes
    const auto c0 = min(input & uint16_t(0xff80), one);

    // c1 - chars that yield 3-byte UTF-8 codes (including surrogates)
    const auto c1 = min(input & uint16_t(0xf800), one);

    /*
        Explanation how the counting works.

        In the case of a non-surrogate character we count:
        * always 1 -- see how `count` is initialized above;
        * c0 = 1 if the current char yields 2 or 3 bytes;
        * c1 = 1 if the current char yields 3 bytes.

        Thus, we always have correct count for the current char:
        from 1, 2 or 3 bytes.

        A trickier part is how we count surrogate pairs. Whether
        we encounter a surrogate (low or high), we count it as
        3 chars and then minus 1 (`is_surrogate` is -1 or 0).
        Each surrogate char yields 2. A surrogate pair, that
        is a low surrogate followed by a high one, yields
        the expected 4 bytes.

        It also correctly handles cases when low surrogate is
        processed by the this loop, but high surrogate is counted
        by the scalar procedure. The scalar procedure uses exactly
        the described approach, thanks to that for valid UTF-16
        strings it always count correctly.
    */
    v_count += c0;
    v_count += c1;
    v_count -= is_surrogate;

    iteration -= 1;
    if (iteration == 0) {
      count += v_count.sum();
      v_count = vector_u16::zero();

      iteration = max_iterations;
    }
  }

  if (iteration > 0) {
    count += v_count.sum();
  }

  return count + scalar::utf16::utf8_length_from_utf16<big_endian>(in + pos,
                                                                   size - pos);
}

// This next function is implemented with intrinsics directly which is
// not the usual style. We should convert it to the usual style.
template <endianness big_endian>
simdutf_really_inline size_t icelake_utf8_length_from_utf16_with_replacement(
    const char16_t* in, size_t size) {


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

  for (; pos < (size - 1) / N * N; pos += N) {
    __m512i input = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(in + pos));
    if (!match_system(big_endian)) {
      input = _mm512_shuffle_epi8(input, byteflip);
    }
    __m512i input_next = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(in + pos + 1));
    if (!match_system(big_endian)) {
      input_next = _mm512_shuffle_epi8(input_next, byteflip);
    }

    __m512i lb_masked = _mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xfc00)));
    __m512i block_masked = _mm512_and_si512(input_next, _mm512_set1_epi16(uint16_t(0xfc00)));

    __mmask32 lb_is_high = _mm512_cmpeq_epi16_mask(lb_masked, _mm512_set1_epi16(uint16_t(0xd800)));
    __mmask32 block_is_low = _mm512_cmpeq_epi16_mask(block_masked, _mm512_set1_epi16(uint16_t(0xdc00)));

    __mmask32 illseq = _kxor_mask32(lb_is_high, block_is_low);
    // 0xd800 .. 0xdbff - low surrogate
    // 0xdc00 .. 0xdfff - high surrogate
    __mmask32 is_surrogate = _mm512_cmpeq_epi16_mask(_mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))), _mm512_set1_epi16(uint16_t(0xd800)));

    // c0 - chars that yield 2- or 3-byte UTF-8 codes
    __mmask32 c0 = _mm512_cmpneq_epi16_mask(_mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xff80))), _mm512_setzero_si512());

    // c1 - chars that yield 3-byte UTF-8 codes (including surrogates)
    __mmask32 c1 = _mm512_cmpneq_epi16_mask(_mm512_and_si512(input, _mm512_set1_epi16(uint16_t(0xf800))), _mm512_setzero_si512());
    count += _popcnt32(c0);
    count += _popcnt32(c1);
    count -= _popcnt32(is_surrogate);
    mismatched_count += _popcnt32(illseq);
  }

  if (scalar::utf16::is_low_surrogate<big_endian>(in[pos])) {
    if (!scalar::utf16::is_high_surrogate<big_endian>(in[pos - 1])) {
      mismatched_count -= 1;
      count += 2;
      pos += 1;
    }
  }
  count += pos;
  count += mismatched_count;
  if (scalar::utf16::is_high_surrogate<big_endian>(in[pos - 1])) {
    if (scalar::utf16::is_low_surrogate<big_endian>(in[pos])) {
      pos += 1;
      count += 2;
    }
  }
  return count + scalar::utf16::utf8_length_from_utf16_with_replacement<big_endian>(in + pos,
                                                                   size - pos);
}