/*
 * Process one block of 16 characters.  If in_place is false,
 * copy the block from in to out.  If there is a sequencing
 * error in the block, overwrite the illsequenced characters
 * with the replacement character.  This function reads one
 * character before the beginning of the buffer as a lookback.
 * If that character is illsequenced, it too is overwritten.
 */
template <endianness big_endian, bool in_place>
void utf16fix_block(char16_t *out, const char16_t *in) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  auto swap_if_needed = [](uint16_t c) -> uint16_t {
    return !simdutf::match_system(big_endian) ? scalar::u16_swap_bytes(c) : c;
  };
  __m256i lookback, block, lb_masked, block_masked, lb_is_high, block_is_low;
  __m256i illseq, lb_illseq, block_illseq, lb_illseq_shifted;

  lookback = _mm256_loadu_si256((const __m256i *)(in - 1));
  block = _mm256_loadu_si256((const __m256i *)in);
  lb_masked =
      _mm256_and_si256(lookback, _mm256_set1_epi16(swap_if_needed(0xfc00u)));
  block_masked =
      _mm256_and_si256(block, _mm256_set1_epi16(swap_if_needed(0xfc00u)));
  lb_is_high =
      _mm256_cmpeq_epi16(lb_masked, _mm256_set1_epi16(swap_if_needed(0xd800u)));
  block_is_low = _mm256_cmpeq_epi16(block_masked,
                                    _mm256_set1_epi16(swap_if_needed(0xdc00u)));

  illseq = _mm256_xor_si256(lb_is_high, block_is_low);
  if (!_mm256_testz_si256(illseq, illseq)) {
    int lb;

    /* compute the cause of the illegal sequencing */
    lb_illseq = _mm256_andnot_si256(block_is_low, lb_is_high);
#if SIMDUTF_GCC9OROLDER
    // Old GCC versions are missing _mm256_zextsi128_si256, so we emulate it.
    __m128i tmp_legacygcc =
        _mm_bslli_si128(_mm256_extracti128_si256(lb_illseq, 1), 14);
    __m256i tmp_legacygcc256 =
        _mm256_set_m128i(_mm_setzero_si128(), tmp_legacygcc);
    lb_illseq_shifted =
        _mm256_or_si256(_mm256_bsrli_epi128(lb_illseq, 2), tmp_legacygcc256);
#else
    lb_illseq_shifted =
        _mm256_or_si256(_mm256_bsrli_epi128(lb_illseq, 2),
                        _mm256_zextsi128_si256(_mm_bslli_si128(
                            _mm256_extracti128_si256(lb_illseq, 1), 14)));
#endif // SIMDUTF_GCC9OROLDER
    block_illseq = _mm256_or_si256(
        _mm256_andnot_si256(lb_is_high, block_is_low), lb_illseq_shifted);

    /* fix illegal sequencing in the lookback */
#if SIMDUTF_GCC10 || SIMDUTF_GCC9OROLDER
    // GCC 10 is missing important intrinsics.
    lb = _mm_cvtsi128_si32(_mm256_extractf128_si256(lb_illseq, 0));
#else
    lb = _mm256_cvtsi256_si32(lb_illseq);
#endif
    lb = (lb & replacement) | (~lb & out[-1]);
    out[-1] = char16_t(lb);

    /* fix illegal sequencing in the main block */
    block =
        _mm256_blendv_epi8(block, _mm256_set1_epi16(replacement), block_illseq);
    _mm256_storeu_si256((__m256i *)out, block);
  } else if (!in_place) {
    _mm256_storeu_si256((__m256i *)out, block);
  }
}

template <endianness big_endian, bool in_place>
void utf16fix_block_sse(char16_t *out, const char16_t *in) {
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
    /* compute the cause of the illegal sequencing */
    lb_illseq = _mm_andnot_si128(block_is_low, lb_is_high);
    block_illseq = _mm_or_si128(_mm_andnot_si128(lb_is_high, block_is_low),
                                _mm_bsrli_si128(lb_illseq, 2));
    /* fix illegal sequencing in the lookback */
    int lb = _mm_cvtsi128_si32(lb_illseq);
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

template <endianness big_endian>
void utf16fix_avx(const char16_t *in, size_t n, char16_t *out) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  size_t i;

  if (n < 17) {
    utf16fix_sse<big_endian>(in, n, out);
    return;
  }

  out[0] =
      scalar::utf16::is_low_surrogate<big_endian>(in[0]) ? replacement : in[0];

  /* duplicate code to have the compiler specialise utf16fix_block() */
  if (in == out) {
    for (i = 1; i + 16 < n; i += 16) {
      utf16fix_block<big_endian, true>(out + i, in + i);
    }

    utf16fix_block<big_endian, true>(out + n - 16, in + n - 16);
  } else {
    for (i = 1; i + 16 < n; i += 16) {
      utf16fix_block<big_endian, false>(out + i, in + i);
    }

    utf16fix_block<big_endian, false>(out + n - 16, in + n - 16);
  }

  out[n - 1] = scalar::utf16::is_high_surrogate<big_endian>(out[n - 1])
                   ? replacement
                   : out[n - 1];
}
