// file included directly

template <endianness big_endian>
std::pair<const char32_t *, char16_t *>
avx512_convert_utf32_to_utf16(const char32_t *buf, size_t len,
                              char16_t *utf16_output) {
  const char32_t *end = buf + len;
  __mmask32 forbidden_bytemask = 0;
  const __m512i v_00000000 = _mm512_setzero_si512();
  const __m512i v_ffff0000 = _mm512_set1_epi32((int32_t)0xffff0000);
  const __m512i v_f800 = _mm512_set1_epi32((uint32_t)0xf800);
  const __m512i v_d800 = _mm512_set1_epi32((uint32_t)0xd800);
  const __m512i v_10ffff = _mm512_set1_epi32(0x10FFFF);
  const __m512i v_10000 = _mm512_set1_epi32(0x10000);
  const __m512i v_3ff0000 = _mm512_set1_epi32(0x3FF0000);
  const __m512i v_3ff = _mm512_set1_epi32(0x3FF);
  const __m512i v_dc00d800 = _mm512_set1_epi32((int32_t)0xDC00D800);

  while (end - buf >= std::ptrdiff_t(16)) {
    __m512i in = _mm512_loadu_si512(buf);

    // no bits set above 16th bit <=> can pack to UTF16 without surrogate pairs
    const __mmask16 saturation_bitmask =
        _mm512_cmpeq_epi32_mask(_mm512_and_si512(in, v_ffff0000), v_00000000);

    if (saturation_bitmask == 0xffff) {
      forbidden_bytemask |=
          _mm512_cmpeq_epi32_mask(_mm512_and_si512(in, v_f800), v_d800);

      __m256i utf16_packed = _mm512_cvtepi32_epi16(in);
      if (big_endian) {
        const __m256i swap = _mm256_setr_epi8(
            1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 1, 0, 3, 2, 5,
            4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
        utf16_packed = _mm256_shuffle_epi8(utf16_packed, swap);
      }
      _mm256_storeu_si256((__m256i *)utf16_output, utf16_packed);
      utf16_output += 16;
      buf += 16;
    } else {
      // saturation_bitmask == 1 words will generate 1 utf16 char,
      // and saturation_bitmask == 0 words will generate 2 utf16 chars assuming
      // no errors. Thus we need a output_mask which has the structure b_2i = 1,
      // b_2i+1 = !saturation_bitmask_i
      const __mmask32 output_mask = ~_pdep_u32(saturation_bitmask, 0xAAAAAAAA);
      const __mmask16 surrogate_bitmask = __mmask16(~saturation_bitmask);
      __mmask32 error = _mm512_mask_cmpeq_epi32_mask(
          saturation_bitmask, _mm512_and_si512(in, v_f800), v_d800);
      error |= _mm512_mask_cmpgt_epu32_mask(surrogate_bitmask, in, v_10ffff);
      if (simdutf_unlikely(error)) {
        return std::make_pair(nullptr, utf16_output);
      }
      __m512i v1, v2, v;
      // for the bits saturation_bitmask == 0, we need to unpack the 32-bit word
      // into two 16 bit words corresponding to high_surrogate and
      // low_surrogate. Once the bits are unpacked and merged, the output will
      // be compressed as per output_mask.
      in = _mm512_mask_sub_epi32(in, surrogate_bitmask, in, v_10000);
      v1 = _mm512_mask_slli_epi32(in, surrogate_bitmask, in, 16);
      v1 = _mm512_mask_and_epi32(in, surrogate_bitmask, v1, v_3ff0000);
      v2 = _mm512_mask_srli_epi32(in, surrogate_bitmask, in, 10);
      v2 = _mm512_mask_and_epi32(in, surrogate_bitmask, v2, v_3ff);
      v = _mm512_or_si512(v1, v2);
      in = _mm512_mask_add_epi32(in, surrogate_bitmask, v, v_dc00d800);
      if (big_endian) {
        const __m512i swap_512 = _mm512_set_epi8(
            14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12,
            13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8,
            9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5,
            2, 3, 0, 1);
        in = _mm512_shuffle_epi8(in, swap_512);
      }
      // we deliberately avoid _mm512_mask_compressstoreu_epi16 for portability
      // (AMD Zen4 has terrible performance with it, it is effectively broken)
      __m512i compressed = _mm512_maskz_compress_epi16(output_mask, in);
      auto written_out = _mm_popcnt_u32(output_mask);
      _mm512_mask_storeu_epi16(utf16_output, _bzhi_u32(0xFFFFFFFF, written_out),
                               compressed);
      //_mm512_mask_compressstoreu_epi16(utf16_output, output_mask, in);
      utf16_output += written_out;
      buf += 16;
    }
  }

  size_t remaining_len = size_t(end - buf);
  if (remaining_len) {
    __mmask16 input_mask = __mmask16((1 << remaining_len) - 1);
    __m512i in = _mm512_maskz_loadu_epi32(input_mask, buf);
    const __mmask16 saturation_bitmask =
        _mm512_cmpeq_epi32_mask(_mm512_and_si512(in, v_ffff0000), v_00000000) &
        input_mask;
    if (saturation_bitmask == input_mask) {
      forbidden_bytemask |=
          _mm512_cmpeq_epi32_mask(_mm512_and_si512(in, v_f800), v_d800);

      __m256i utf16_packed = _mm512_cvtepi32_epi16(in);
      if (big_endian) {
        const __m256i swap = _mm256_setr_epi8(
            1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 1, 0, 3, 2, 5,
            4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
        utf16_packed = _mm256_shuffle_epi8(utf16_packed, swap);
      }
      _mm256_mask_storeu_epi16(utf16_output, input_mask, utf16_packed);
      utf16_output += remaining_len;
      buf += remaining_len;
    } else {
      const __mmask32 output_max_mask = (1 << (remaining_len * 2)) - 1;
      const __mmask32 output_mask =
          (~_pdep_u32(saturation_bitmask, 0xAAAAAAAA)) & output_max_mask;
      const __mmask16 surrogate_bitmask =
          __mmask16(~saturation_bitmask) & input_mask;
      __mmask32 error = _mm512_mask_cmpeq_epi32_mask(
          saturation_bitmask, _mm512_and_si512(in, v_f800), v_d800);
      error |= _mm512_mask_cmpgt_epu32_mask(surrogate_bitmask, in, v_10ffff);
      if (simdutf_unlikely(error)) {
        return std::make_pair(nullptr, utf16_output);
      }
      __m512i v1, v2, v;
      in = _mm512_mask_sub_epi32(in, surrogate_bitmask, in, v_10000);
      v1 = _mm512_mask_slli_epi32(in, surrogate_bitmask, in, 16);
      v1 = _mm512_mask_and_epi32(in, surrogate_bitmask, v1, v_3ff0000);
      v2 = _mm512_mask_srli_epi32(in, surrogate_bitmask, in, 10);
      v2 = _mm512_mask_and_epi32(in, surrogate_bitmask, v2, v_3ff);
      v = _mm512_or_si512(v1, v2);
      in = _mm512_mask_add_epi32(in, surrogate_bitmask, v, v_dc00d800);
      if (big_endian) {
        const __m512i swap_512 = _mm512_set_epi8(
            14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12,
            13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8,
            9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5,
            2, 3, 0, 1);
        in = _mm512_shuffle_epi8(in, swap_512);
      }
      // we deliberately avoid _mm512_mask_compressstoreu_epi16 for portability
      // (AMD Zen4 has terrible performance with it, it is effectively broken)
      __m512i compressed = _mm512_maskz_compress_epi16(output_mask, in);
      auto written_out = _mm_popcnt_u32(output_mask);
      _mm512_mask_storeu_epi16(utf16_output, _bzhi_u32(0xFFFFFFFF, written_out),
                               compressed);
      //_mm512_mask_compressstoreu_epi16(utf16_output, output_mask, in);
      utf16_output += written_out;
      buf += remaining_len;
    }
  }

  // check for invalid input
  if (forbidden_bytemask != 0) {
    return std::make_pair(nullptr, utf16_output);
  }

  return std::make_pair(buf, utf16_output);
}

template <endianness big_endian>
std::pair<result, char16_t *>
avx512_convert_utf32_to_utf16_with_errors(const char32_t *buf, size_t len,
                                          char16_t *utf16_output) {
  const char32_t *start = buf;
  const char32_t *end = buf + len;
  const __m512i v_00000000 = _mm512_setzero_si512();
  const __m512i v_ffff0000 = _mm512_set1_epi32((int32_t)0xffff0000);
  const __m512i v_f800 = _mm512_set1_epi32((uint32_t)0xf800);
  const __m512i v_d800 = _mm512_set1_epi32((uint32_t)0xd800);
  const __m512i v_10ffff = _mm512_set1_epi32(0x10FFFF);
  const __m512i v_10000 = _mm512_set1_epi32(0x10000);
  const __m512i v_3ff0000 = _mm512_set1_epi32(0x3FF0000);
  const __m512i v_3ff = _mm512_set1_epi32(0x3FF);
  const __m512i v_dc00d800 = _mm512_set1_epi32((int32_t)0xDC00D800);
  int error_idx = 0;
  error_code code = error_code::SUCCESS;
  bool err = false;

  while (end - buf >= std::ptrdiff_t(16)) {
    __m512i in = _mm512_loadu_si512(buf);

    // no bits set above 16th bit <=> can pack to UTF16 without surrogate pairs
    const __mmask16 saturation_bitmask =
        _mm512_cmpeq_epi32_mask(_mm512_and_si512(in, v_ffff0000), v_00000000);

    if (saturation_bitmask == 0xffff) {
      __mmask32 forbidden_bytemask =
          _mm512_cmpeq_epi32_mask(_mm512_and_si512(in, v_f800), v_d800);

      __m256i utf16_packed = _mm512_cvtepi32_epi16(in);
      if (big_endian) {
        const __m256i swap = _mm256_setr_epi8(
            1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 1, 0, 3, 2, 5,
            4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
        utf16_packed = _mm256_shuffle_epi8(utf16_packed, swap);
      }
      if (simdutf_unlikely(forbidden_bytemask)) {
        int idx = _tzcnt_u32(forbidden_bytemask);
        _mm256_mask_storeu_epi16(
            utf16_output, __mmask16(_blsmsk_u32(forbidden_bytemask) >> 1),
            utf16_packed);
        return std::make_pair(result(error_code::SURROGATE, buf - start + idx),
                              utf16_output + idx);
      }
      _mm256_storeu_si256((__m256i *)utf16_output, utf16_packed);
      utf16_output += 16;
    } else {
      __mmask32 output_mask = ~_pdep_u32(saturation_bitmask, 0xAAAAAAAA);
      const __mmask16 surrogate_bitmask = __mmask16(~saturation_bitmask);
      __mmask32 error_surrogate = _mm512_mask_cmpeq_epi32_mask(
          saturation_bitmask, _mm512_and_si512(in, v_f800), v_d800);
      __mmask32 error_too_large =
          _mm512_mask_cmpgt_epu32_mask(surrogate_bitmask, in, v_10ffff);
      if (simdutf_unlikely(error_surrogate || error_too_large)) {
        // Need to find the lowest set bit between the two error masks
        // Need to also write the partial chunk until the error index to output.
        int large_idx = _tzcnt_u32(error_too_large);
        int surrogate_idx = _tzcnt_u32(error_surrogate);
        err = true;
        if (large_idx < surrogate_idx) {
          code = error_code::TOO_LARGE;
          error_idx = large_idx;
        } else {
          code = error_code::SURROGATE;
          error_idx = surrogate_idx;
        }
        output_mask &= ((1 << (2 * error_idx)) - 1);
      }
      __m512i v1, v2, v;
      in = _mm512_mask_sub_epi32(in, surrogate_bitmask, in, v_10000);
      v1 = _mm512_mask_slli_epi32(in, surrogate_bitmask, in, 16);
      v1 = _mm512_mask_and_epi32(in, surrogate_bitmask, v1, v_3ff0000);
      v2 = _mm512_mask_srli_epi32(in, surrogate_bitmask, in, 10);
      v2 = _mm512_mask_and_epi32(in, surrogate_bitmask, v2, v_3ff);
      v = _mm512_or_si512(v1, v2);
      in = _mm512_mask_add_epi32(in, surrogate_bitmask, v, v_dc00d800);
      if (big_endian) {
        const __m512i swap_512 = _mm512_set_epi8(
            14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12,
            13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8,
            9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5,
            2, 3, 0, 1);
        in = _mm512_shuffle_epi8(in, swap_512);
      }
      // we deliberately avoid _mm512_mask_compressstoreu_epi16 for portability
      // (AMD Zen4 has terrible performance with it, it is effectively broken)
      __m512i compressed = _mm512_maskz_compress_epi16(output_mask, in);
      auto written_out = _mm_popcnt_u32(output_mask);
      _mm512_mask_storeu_epi16(utf16_output, _bzhi_u32(0xFFFFFFFF, written_out),
                               compressed);
      //_mm512_mask_compressstoreu_epi16(utf16_output, output_mask, in);
      utf16_output += written_out;
      if (simdutf_unlikely(err)) {
        return std::make_pair(result(code, buf - start + error_idx),
                              utf16_output);
      }
    }
    buf += 16;
  }

  size_t remaining_len = size_t(end - buf);
  if (remaining_len) {
    __mmask16 input_mask = __mmask16((1 << remaining_len) - 1);
    __m512i in = _mm512_maskz_loadu_epi32(input_mask, buf);
    const __mmask16 saturation_bitmask =
        _mm512_cmpeq_epi32_mask(_mm512_and_si512(in, v_ffff0000), v_00000000) &
        input_mask;
    if (saturation_bitmask == input_mask) {
      __mmask32 forbidden_bytemask =
          _mm512_cmpeq_epi32_mask(_mm512_and_si512(in, v_f800), v_d800);
      __m256i utf16_packed = _mm512_cvtepi32_epi16(in);
      if (big_endian) {
        const __m256i swap = _mm256_setr_epi8(
            1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 1, 0, 3, 2, 5,
            4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
        utf16_packed = _mm256_shuffle_epi8(utf16_packed, swap);
      }
      if (simdutf_unlikely(forbidden_bytemask)) {
        int idx = _tzcnt_u32(forbidden_bytemask);
        _mm256_mask_storeu_epi16(
            utf16_output, __mmask16(_blsmsk_u32(forbidden_bytemask) >> 1),
            utf16_packed);
        return std::make_pair(result(error_code::SURROGATE, buf - start + idx),
                              utf16_output + idx);
      }
      _mm256_mask_storeu_epi16(utf16_output, input_mask, utf16_packed);
      utf16_output += remaining_len;
    } else {
      const __mmask32 output_max_mask = (1 << (remaining_len * 2)) - 1;
      __mmask32 output_mask =
          (~_pdep_u32(saturation_bitmask, 0xAAAAAAAA)) & output_max_mask;
      const __mmask16 surrogate_bitmask =
          __mmask16(~saturation_bitmask) & input_mask;
      __mmask32 error_surrogate = _mm512_mask_cmpeq_epi32_mask(
          saturation_bitmask, _mm512_and_si512(in, v_f800), v_d800);
      __mmask32 error_too_large =
          _mm512_mask_cmpgt_epu32_mask(surrogate_bitmask, in, v_10ffff);
      if (simdutf_unlikely(error_surrogate || error_too_large)) {
        int large_idx = _tzcnt_u32(error_too_large);
        int surrogate_idx = _tzcnt_u32(error_surrogate);
        err = true;
        if (large_idx < surrogate_idx) {
          code = error_code::TOO_LARGE;
          error_idx = large_idx;
        } else {
          code = error_code::SURROGATE;
          error_idx = surrogate_idx;
        }
        output_mask &= ((1 << (2 * error_idx)) - 1);
      }
      __m512i v1, v2, v;
      in = _mm512_mask_sub_epi32(in, surrogate_bitmask, in, v_10000);
      v1 = _mm512_mask_slli_epi32(in, surrogate_bitmask, in, 16);
      v1 = _mm512_mask_and_epi32(in, surrogate_bitmask, v1, v_3ff0000);
      v2 = _mm512_mask_srli_epi32(in, surrogate_bitmask, in, 10);
      v2 = _mm512_mask_and_epi32(in, surrogate_bitmask, v2, v_3ff);
      v = _mm512_or_si512(v1, v2);
      in = _mm512_mask_add_epi32(in, surrogate_bitmask, v, v_dc00d800);
      if (big_endian) {
        const __m512i swap_512 = _mm512_set_epi8(
            14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12,
            13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8,
            9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5,
            2, 3, 0, 1);
        in = _mm512_shuffle_epi8(in, swap_512);
      }
      // we deliberately avoid _mm512_mask_compressstoreu_epi16 for portability
      // (AMD Zen4 has terrible performance with it, it is effectively broken)
      __m512i compressed = _mm512_maskz_compress_epi16(output_mask, in);
      auto written_out = _mm_popcnt_u32(output_mask);
      _mm512_mask_storeu_epi16(utf16_output, _bzhi_u32(0xFFFFFFFF, written_out),
                               compressed);
      //_mm512_mask_compressstoreu_epi16(utf16_output, output_mask, in);
      utf16_output += written_out;
      if (simdutf_unlikely(err)) {
        return std::make_pair(result(code, buf - start + error_idx),
                              utf16_output);
      }
    }
    buf += remaining_len;
  }

  return std::make_pair(result(error_code::SUCCESS, buf - start), utf16_output);
}
