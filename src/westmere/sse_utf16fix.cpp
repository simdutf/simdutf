/*
 * Process one block of 8 characters.  If in_place is false,
 * copy the block from in to out.  If there is a sequencing
 * error in the block, overwrite the illsequenced characters
 * with the replacement character.  This function reads one
 * character before the beginning of the buffer as a lookback.
 * If that character is illsequenced, it too is overwritten.
 */
template <endianness big_endian, bool in_place>
simdutf_really_inline void utf16fix_block_sse(char16_t *out,
                                              const char16_t *in) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  auto swap_if_needed = [](uint16_t c) -> uint16_t {
    return !simdutf::match_system(big_endian) ? scalar::u16_swap_bytes(c) : c;
  };

  __m128i lookback, block, lb_masked, block_masked, lb_is_high, block_is_low;
  __m128i illseq, lb_illseq, block_illseq;

  lookback = _mm_loadu_si128((const __m128i *)(in - 1));
  block = _mm_loadu_si128((const __m128i *)in);
  lb_masked = _mm_and_si128(lookback, _mm_set1_epi16(swap_if_needed(0xfc00U)));
  block_masked = _mm_and_si128(block, _mm_set1_epi16(swap_if_needed(0xfc00U)));
  lb_is_high =
      _mm_cmpeq_epi16(lb_masked, _mm_set1_epi16(swap_if_needed(0xd800U)));
  block_is_low =
      _mm_cmpeq_epi16(block_masked, _mm_set1_epi16(swap_if_needed(0xdc00U)));

  illseq = _mm_xor_si128(lb_is_high, block_is_low);
  if (_mm_movemask_epi8(illseq) != 0) {
    int lb;

    /* compute the cause of the illegal sequencing */
    lb_illseq = _mm_andnot_si128(block_is_low, lb_is_high);
    block_illseq = _mm_or_si128(_mm_andnot_si128(lb_is_high, block_is_low),
                                _mm_bsrli_si128(lb_illseq, 2));

    /* fix illegal sequencing in the lookback */
    lb = _mm_cvtsi128_si32(lb_illseq);
    lb = (lb & replacement) | (~lb & out[-1]);
    out[-1] = char16_t(lb);
    /* fix illegal sequencing in the main block */
    block =
        _mm_or_si128(_mm_andnot_si128(block_illseq, block),
                     _mm_and_si128(block_illseq, _mm_set1_epi16(replacement)));
    _mm_storeu_si128((__m128i *)out, block);
  } else if (!in_place) {
    _mm_storeu_si128((__m128i *)out, block);
  }
}

template <endianness big_endian>
void utf16fix_sse(const char16_t *in, size_t n, char16_t *out) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  size_t i;
  if (n < 9) {
    scalar::utf16::to_well_formed_utf16<big_endian>(in, n, out);
    return;
  }

  out[0] =
      scalar::utf16::is_low_surrogate<big_endian>(in[0]) ? replacement : in[0];

  /* duplicate code to have the compiler specialise utf16fix_block() */
  if (in == out) {
    for (i = 1; i + 8 < n; i += 8) {
      utf16fix_block_sse<big_endian, true>(out + i, in + i);
    }

    utf16fix_block_sse<big_endian, true>(out + n - 8, in + n - 8);
  } else {
    for (i = 1; i + 8 < n; i += 8) {
      utf16fix_block_sse<big_endian, false>(out + i, in + i);
    }

    utf16fix_block_sse<big_endian, false>(out + n - 8, in + n - 8);
  }

  out[n - 1] = scalar::utf16::is_high_surrogate<big_endian>(out[n - 1])
                   ? replacement
                   : out[n - 1];
}
