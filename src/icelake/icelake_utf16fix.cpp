__m512i swap_endianness(__m512i x) {
  const __m512i swap = _mm512_setr_epi64(
      0x0607040502030001, 0x0e0f0c0d0a0b0809, 0x0607040502030001,
      0x0e0f0c0d0a0b0809, 0x0607040502030001, 0x0e0f0c0d0a0b0809,
      0x0607040502030001, 0x0e0f0c0d0a0b0809);
  return _mm512_shuffle_epi8(x, swap);
}

/*
 * Process one block of 32 characters.  If in_place is false,
 * copy the block from in to out.  If there is a sequencing
 * error in the block, overwrite the illsequenced characters
 * with the replacement character.  This function reads one
 * character before the beginning of the buffer as a lookback.
 * If that character is illsequenced, it too is overwritten.
 */
template <endianness big_endian>
static void utf16fix_block(char16_t *out, const char16_t *in, bool in_place) {
  const char16_t replacement = 0xfffd;
  __m512i lookback, block, lb_masked, block_masked;
  __mmask32 lb_is_high, block_is_low, illseq;

  lookback = _mm512_loadu_si512((const __m512i *)(in - 1));
  block = _mm512_loadu_si512((const __m512i *)in);
  if (!simdutf::match_system(big_endian)) {
    lookback = swap_endianness(lookback);
    block = swap_endianness(block);
  }
  lb_masked = _mm512_and_epi32(lookback, _mm512_set1_epi16(0xfc00U));
  block_masked = _mm512_and_epi32(block, _mm512_set1_epi16(0xfc00U));

  lb_is_high = _mm512_cmpeq_epi16_mask(lb_masked, _mm512_set1_epi16(0xd800U));
  block_is_low =
      _mm512_cmpeq_epi16_mask(block_masked, _mm512_set1_epi16(0xdc00U));
  illseq = _kxor_mask32(lb_is_high, block_is_low);
  if (!_ktestz_mask32_u8(illseq, illseq)) {
    __mmask32 lb_illseq, block_illseq;

    /* compute the cause of the illegal sequencing */
    lb_illseq = _kandn_mask32(block_is_low, lb_is_high);
    block_illseq = _kor_mask32(_kandn_mask32(lb_is_high, block_is_low),
                               _kshiftri_mask32(lb_illseq, 1));

    /* fix illegal sequencing in the lookback */
    lb_illseq = _kand_mask32(lb_illseq, _cvtu32_mask32(1));
    _mm512_mask_storeu_epi16(out - 1, lb_illseq,
                             _mm512_set1_epi16(replacement));

    /* fix illegal sequencing in the main block */
    if (in_place) {
      _mm512_mask_storeu_epi16(out, block_illseq,
                               _mm512_set1_epi16(replacement));
    } else {
      _mm512_storeu_epi32(
          out, _mm512_mask_blend_epi16(block_illseq, block,
                                       _mm512_set1_epi16(replacement)));
    }
  } else if (!in_place) {
    if (!simdutf::match_system(big_endian)) {
      block = swap_endianness(block);
    }
    _mm512_storeu_si512((__m512i *)out, block);
  }
}

/*
 * Special case for inputs of 0--32 bytes.  Works for both in-place and
 * out-of-place operation.
 */
template <endianness big_endian>
void utf16fix_runt(const char16_t *in, size_t n, char16_t *out) {
  const char16_t replacement =
      !match_system(big_endian) ? scalar::u16_swap_bytes(0xfffd) : 0xfffd;
  __m512i lookback, block, lb_masked, block_masked;
  __mmask32 lb_is_high, block_is_low, illseq;
  unsigned long long mask;

  mask = (1ULL << n) - 1;
  lookback = _mm512_maskz_loadu_epi16(_cvtmask32_u32(mask << 1),
                                      (const void *)(in - 1));
  block = _mm512_maskz_loadu_epi16(_cvtmask32_u32(mask), (const void *)in);
  lb_masked = _mm512_and_epi32(lookback, _mm512_set1_epi16(0xfc00));
  block_masked = _mm512_and_epi32(block, _mm512_set1_epi16(0xfc00));

  lb_is_high = _mm512_cmpeq_epi16_mask(lb_masked, _mm512_set1_epi16(0xd800));
  block_is_low =
      _mm512_cmpeq_epi16_mask(block_masked, _mm512_set1_epi16(0xdc00));
  illseq = _kxor_mask32(lb_is_high, block_is_low);
  if (!_ktestz_mask32_u8(illseq, illseq)) {
    __mmask32 lb_illseq, block_illseq;

    /* compute the cause of the illegal sequencing */
    lb_illseq = _kandn_mask32(block_is_low, lb_is_high);
    block_illseq = _kor_mask32(_kandn_mask32(lb_is_high, block_is_low),
                               _kshiftri_mask32(lb_illseq, 1));

    /* fix illegal sequencing in the main block */
    _mm512_mask_storeu_epi16(
        (uint16_t *)out, _cvtmask32_u32(mask),
        _mm512_mask_blend_epi16(block_illseq, block,
                                _mm512_set1_epi16(replacement)));
  } else {
    _mm512_mask_storeu_epi16((uint16_t *)out, _cvtmask32_u32(mask), block);
  }
  out[n - 1] =
      is_high_surrogate<big_endian>(out[n - 1]) ? replacement : out[n - 1];
}

template <endianness big_endian>
void utf16fix_avx512(const char16_t *in, size_t n, char16_t *out) {
  const char16_t replacement =
      !match_system(big_endian) ? scalar::u16_swap_bytes(0xfffd) : 0xfffd;
  size_t i;

  if (n == 0)
    return;
  else if (n < 33) {
    utf16fix_runt<big_endian>(out, in, n);
    return;
  }
  out[0] = is_low_surrogate<big_endian>(in[0]) ? replacement : in[0];

  /* duplicate code to have the compiler specialise utf16fix_block() */
  if (in == out) {
    for (i = 1; i + 32 < n; i += 32) {
      utf16fix_block<big_endian>(out + i, in + i, true);
    }

    utf16fix_block<big_endian>(out + n - 32, in + n - 32, true);
  } else {
    for (i = 1; i + 32 < n; i += 32) {
      utf16fix_block<big_endian>(out + i, in + i, false);
    }

    utf16fix_block<big_endian>(out + n - 32, in + n - 32, false);
  }

  out[n - 1] =
      is_high_surrogate<big_endian>(out[n - 1]) ? replacement : out[n - 1];
}
