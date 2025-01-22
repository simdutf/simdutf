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
      _mm512_mask_compressstoreu_epi16(utf16_output, output_mask, in);
      utf16_output += _mm_popcnt_u32(output_mask);
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
      _mm512_mask_compressstoreu_epi16(utf16_output, output_mask, in);
      utf16_output += _mm_popcnt_u32(output_mask);
      buf += remaining_len;
    }
  }

  // check for invalid input
  if (forbidden_bytemask != 0) {
    return std::make_pair(nullptr, utf16_output);
  }

  return std::make_pair(buf, utf16_output);
}

// Todo: currently, this is just the haswell code, optimize for icelake kernel.
template <endianness big_endian>
std::pair<result, char16_t *>
avx512_convert_utf32_to_utf16_with_errors(const char32_t *buf, size_t len,
                                          char16_t *utf16_output) {
  const char32_t *start = buf;
  const char32_t *end = buf + len;

  const size_t safety_margin =
      12; // to avoid overruns, see issue
          // https://github.com/simdutf/simdutf/issues/92

  while (end - buf >= std::ptrdiff_t(8 + safety_margin)) {
    __m256i in = _mm256_loadu_si256((__m256i *)buf);

    const __m256i v_00000000 = _mm256_setzero_si256();
    const __m256i v_ffff0000 = _mm256_set1_epi32((int32_t)0xffff0000);

    // no bits set above 16th bit <=> can pack to UTF16 without surrogate pairs
    const __m256i saturation_bytemask =
        _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff0000), v_00000000);
    const uint32_t saturation_bitmask =
        static_cast<uint32_t>(_mm256_movemask_epi8(saturation_bytemask));

    if (saturation_bitmask == 0xffffffff) {
      const __m256i v_f800 = _mm256_set1_epi32((uint32_t)0xf800);
      const __m256i v_d800 = _mm256_set1_epi32((uint32_t)0xd800);
      const __m256i forbidden_bytemask =
          _mm256_cmpeq_epi32(_mm256_and_si256(in, v_f800), v_d800);
      if (static_cast<uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) !=
          0x0) {
        return std::make_pair(result(error_code::SURROGATE, buf - start),
                              utf16_output);
      }

      __m128i utf16_packed = _mm_packus_epi32(_mm256_castsi256_si128(in),
                                              _mm256_extractf128_si256(in, 1));
      if (big_endian) {
        const __m128i swap =
            _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
        utf16_packed = _mm_shuffle_epi8(utf16_packed, swap);
      }
      _mm_storeu_si128((__m128i *)utf16_output, utf16_packed);
      utf16_output += 8;
      buf += 8;
    } else {
      size_t forward = 7;
      size_t k = 0;
      if (size_t(end - buf) < forward + 1) {
        forward = size_t(end - buf - 1);
      }
      for (; k < forward; k++) {
        uint32_t word = buf[k];
        if ((word & 0xFFFF0000) == 0) {
          // will not generate a surrogate pair
          if (word >= 0xD800 && word <= 0xDFFF) {
            return std::make_pair(
                result(error_code::SURROGATE, buf - start + k), utf16_output);
          }
          *utf16_output++ =
              big_endian
                  ? char16_t((uint16_t(word) >> 8) | (uint16_t(word) << 8))
                  : char16_t(word);
        } else {
          // will generate a surrogate pair
          if (word > 0x10FFFF) {
            return std::make_pair(
                result(error_code::TOO_LARGE, buf - start + k), utf16_output);
          }
          word -= 0x10000;
          uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
          uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
          if (big_endian) {
            high_surrogate =
                uint16_t((high_surrogate >> 8) | (high_surrogate << 8));
            low_surrogate =
                uint16_t((low_surrogate >> 8) | (low_surrogate << 8));
          }
          *utf16_output++ = char16_t(high_surrogate);
          *utf16_output++ = char16_t(low_surrogate);
        }
      }
      buf += k;
    }
  }

  return std::make_pair(result(error_code::SUCCESS, buf - start), utf16_output);
}
