// file included directly

/*
 * Adapted from ffutf by Peter Fors (MIT licensed):
 * https://github.com/vtlmks/ffutf
 */
namespace utf8_to_utf16_fast {

alignas(64) constexpr uint8_t zip16[64] = {
    0,  64, 1,  65, 2,  66, 3,  67, 4,  68, 5,  69, 6,  70, 7,  71,
    8,  72, 9,  73, 10, 74, 11, 75, 12, 76, 13, 77, 14, 78, 15, 79,
    16, 80, 17, 81, 18, 82, 19, 83, 20, 84, 21, 85, 22, 86, 23, 87,
    24, 88, 25, 89, 26, 90, 27, 91, 28, 92, 29, 93, 30, 94, 31, 95};

template <endianness big_endian>
simdutf_really_inline __m512i to_endian(__m512i words) {
  if (match_system(big_endian)) {
    return words;
  }
  return _mm512_shuffle_epi8(
      words, _mm512_broadcast_i32x4(_mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8,
                                                  11, 10, 13, 12, 15, 14)));
}

template <endianness big_endian>
simdutf_really_inline void widen_ascii(__m512i bytes, uint64_t active,
                                       char16_t *output) {
  _mm512_mask_storeu_epi16(output, __mmask32(active),
                           to_endian<big_endian>(_mm512_cvtepu8_epi16(
                               _mm512_castsi512_si256(bytes))));
  if (active >> 32) {
    _mm512_mask_storeu_epi16(output + 32, __mmask32(active >> 32),
                             to_endian<big_endian>(_mm512_cvtepu8_epi16(
                                 _mm512_extracti64x4_epi64(bytes, 1))));
  }
}

template <endianness big_endian>
simdutf_really_inline size_t convert_block(__m512i bytes, __m512i prev1,
                                           __m512i prev2, __m512i prev3,
                                           uint64_t active, char16_t *output) {
  const uint64_t cont =
      _mm512_cmplt_epi8_mask(bytes, _mm512_set1_epi8(char(0xc0)));
  const uint64_t first =
      _mm512_cmpge_epu8_mask(prev1, _mm512_set1_epi8(char(0xc0)));
  const uint64_t second =
      _mm512_cmpge_epu8_mask(prev2, _mm512_set1_epi8(char(0xe0)));
  const uint64_t third =
      _mm512_cmpge_epu8_mask(prev3, _mm512_set1_epi8(char(0xf0)));
  const __m512i bounds = _mm512_permutexvar_epi8(
      prev1,
      _mm512_load_si512((const __m512i *)utf8_fast_validation::second_bounds));
  const __m512i delta = _mm512_maskz_sub_epi8(first, bytes, bounds);
  if ((((cont ^ (first | second | third)) | _mm512_movepi8_mask(delta)) &
       active) != 0) {
    return SIZE_MAX;
  }
  const uint64_t ascii = active & ~uint64_t(_mm512_movepi8_mask(bytes));
  const uint64_t long_first =
      _mm512_cmpge_epu8_mask(prev1, _mm512_set1_epi8(char(0xe0)));
  const uint64_t high_surrogate =
      _mm512_cmpge_epu8_mask(prev2, _mm512_set1_epi8(char(0xf0)));
  const uint64_t take =
      ascii | (active & ((first & ~long_first) | second | third));
  __m512i lo = _mm512_ternarylogic_epi32(_mm512_slli_epi16(prev1, 6), bytes,
                                         _mm512_set1_epi8(char(0xc0)), 0xe4);
  const __m512i mid =
      _mm512_and_si512(_mm512_srli_epi16(prev1, 2), _mm512_set1_epi8(15));
  const __m512i top = _mm512_maskz_mov_epi8(
      second, _mm512_and_si512(_mm512_slli_epi16(prev2, 4),
                               _mm512_set1_epi8(char(0xf0))));
  __m512i hi = _mm512_maskz_mov_epi8(cont, _mm512_or_si512(mid, top));
  lo = _mm512_mask_mov_epi8(lo, ascii, bytes);
  if (high_surrogate | third) {
    const __m512i high_lo = _mm512_sub_epi8(
        _mm512_ternarylogic_epi32(_mm512_slli_epi16(prev1, 2),
                                  _mm512_srli_epi16(bytes, 4),
                                  _mm512_set1_epi8(char(0xfc)), 0xe4),
        _mm512_set1_epi8(0x40));
    const uint64_t borrow =
        _mm512_cmplt_epu8_mask(prev1, _mm512_set1_epi8(char(0x90)));
    __m512i high_hi =
        _mm512_add_epi8(_mm512_and_si512(prev2, _mm512_set1_epi8(7)),
                        _mm512_set1_epi8(char(0xd8)));
    high_hi =
        _mm512_mask_sub_epi8(high_hi, borrow, high_hi, _mm512_set1_epi8(1));
    const __m512i low_hi =
        _mm512_or_si512(_mm512_and_si512(mid, _mm512_set1_epi8(3)),
                        _mm512_set1_epi8(char(0xdc)));
    lo = _mm512_mask_mov_epi8(lo, high_surrogate, high_lo);
    hi = _mm512_mask_mov_epi8(hi, high_surrogate, high_hi);
    hi = _mm512_mask_mov_epi8(hi, third, low_hi);
  }
  const __m512i low = _mm512_maskz_compress_epi8(take, lo);
  const __m512i high = _mm512_maskz_compress_epi8(take, hi);
  const size_t count = size_t(count_ones(take));
  const __m512i zip = _mm512_load_si512((const __m512i *)zip16);
  const __m512i first_half = match_system(big_endian)
                                 ? _mm512_permutex2var_epi8(low, zip, high)
                                 : _mm512_permutex2var_epi8(high, zip, low);
  if (count > 32) {
    _mm512_storeu_si512(output, first_half);
    const __m512i zip_upper = _mm512_add_epi8(zip, _mm512_set1_epi8(32));
    const __m512i second_half =
        match_system(big_endian)
            ? _mm512_permutex2var_epi8(low, zip_upper, high)
            : _mm512_permutex2var_epi8(high, zip_upper, low);
    _mm512_mask_storeu_epi16(output + 32, __mmask32(UINT64_MAX >> (96 - count)),
                             second_half);
  } else {
    _mm512_mask_storeu_epi16(output, __mmask32((UINT64_C(1) << count) - 1),
                             first_half);
  }
  return count;
}

simdutf_really_inline char16_t last_continuation(const uint8_t *input) {
  const uint32_t last = input[0];
  const uint32_t prev1 = input[-1];
  const uint32_t prev2 = input[-2];
  const uint32_t prev3 = input[-3];
  uint32_t prefix;
  if (prev1 >= 0xc0) {
    if (uint8_t(last - utf8_fast_validation::second_bounds[prev1 & 63]) >=
        0x80) {
      return 0;
    }
    prefix = (prev1 & 0x1f) << 6;
  } else if (prev2 >= 0xe0) {
    prefix = ((prev2 & 0x0f) << 12) | ((prev1 & 0x3f) << 6);
  } else if (prev3 >= 0xf0) {
    prefix = 0xdc00 | ((prev1 & 0x0f) << 6);
  } else {
    return 0;
  }
  return char16_t(prefix | (last & 0x3f));
}

simdutf_really_inline bool incomplete_end(const uint8_t *input, size_t len) {
  return (len && input[len - 1] >= 0xc0) ||
         (len >= 2 && input[len - 2] >= 0xe0) ||
         (len >= 3 && input[len - 3] >= 0xf0);
}

template <endianness big_endian>
simdutf_really_inline size_t convert_mixed(const uint8_t *input, size_t len,
                                           char16_t *output,
                                           const uint8_t **fail,
                                           size_t *written_before_fail) {
  size_t written = 0;
  for (; len >= 128; input += 128, len -= 128, written += 128) {
    const __m512i first = _mm512_loadu_si512((const __m512i *)input);
    const __m512i second = _mm512_loadu_si512((const __m512i *)(input + 64));
    if (_mm512_movepi8_mask(_mm512_or_si512(first, second))) {
      break;
    }
    widen_ascii<big_endian>(first, UINT64_MAX, output + written);
    widen_ascii<big_endian>(second, UINT64_MAX, output + written + 64);
  }
  if (!len) {
    return written;
  }
  uint64_t active = UINT64_MAX >> (64 - (len < 64 ? len : 64));
  __m512i bytes = _mm512_maskz_loadu_epi8(active, (const __m512i *)input);
  if (!_mm512_movepi8_mask(bytes)) {
    if (simdutf_unlikely(len > 64 && len < 128)) {
      const __m512i last =
          _mm512_loadu_si512((const __m512i *)(input + len - 64));
      if (!_mm512_movepi8_mask(last)) {
        widen_ascii<big_endian>(bytes, UINT64_MAX, output + written);
        widen_ascii<big_endian>(last, UINT64_MAX, output + written + len - 64);
        return written + len;
      }
    }
    widen_ascii<big_endian>(bytes, active, output + written);
    if (len <= 64) {
      return written + len;
    }
    written += 64;
  } else {
    const __m512i index =
        _mm512_load_si512((const __m512i *)utf8_fast_validation::next_index);
    const __m512i prev1 = _mm512_maskz_permutexvar_epi8(
        UINT64_MAX << 1, _mm512_sub_epi8(index, _mm512_set1_epi8(2)), bytes);
    const __m512i prev2 = _mm512_maskz_permutexvar_epi8(
        UINT64_MAX << 2, _mm512_sub_epi8(index, _mm512_set1_epi8(3)), bytes);
    const __m512i prev3 = _mm512_maskz_permutexvar_epi8(
        UINT64_MAX << 3, _mm512_sub_epi8(index, _mm512_set1_epi8(4)), bytes);
    const size_t count = convert_block<big_endian>(bytes, prev1, prev2, prev3,
                                                   active, output + written);
    if (count == SIZE_MAX) {
      goto invalid;
    }
    written += count;
    if (len <= 64) {
      return written;
    }
  }
  input += 64;
  len -= 64;
  for (; len >= 64; input += 64, len -= 64) {
    bytes = _mm512_loadu_si512((const __m512i *)input);
    const __m512i prev1 = _mm512_loadu_si512((const __m512i *)(input - 1));
    const __m512i prev2 = _mm512_loadu_si512((const __m512i *)(input - 2));
    const __m512i prev3 = _mm512_loadu_si512((const __m512i *)(input - 3));
    const size_t count = convert_block<big_endian>(
        bytes, prev1, prev2, prev3, UINT64_MAX, output + written);
    if (count == SIZE_MAX) {
      goto invalid;
    }
    written += count;
  }
  if (len) {
    active = (UINT64_C(1) << len) - 1;
    bytes = _mm512_maskz_loadu_epi8(active, (const __m512i *)input);
    if (!_mm512_movepi8_mask(bytes)) {
      if (input[-1] >= 0xc0 || input[-2] >= 0xe0 || input[-3] >= 0xf0) {
        goto invalid;
      }
      widen_ascii<big_endian>(bytes, active, output + written);
      return written + len;
    }
    if (len == 1) {
      const char16_t word = last_continuation(input);
      if (word == 0) {
        goto invalid;
      }
      output[written] = char16_t(
          match_system(big_endian) ? word : scalar::u16_swap_bytes(word));
      return written + 1;
    }
    const __m512i prev1 = _mm512_maskz_loadu_epi8((active << 1) | 1,
                                                  (const __m512i *)(input - 1));
    const __m512i prev2 = _mm512_maskz_loadu_epi8((active << 2) | 3,
                                                  (const __m512i *)(input - 2));
    const __m512i prev3 = _mm512_maskz_loadu_epi8((active << 3) | 7,
                                                  (const __m512i *)(input - 3));
    const size_t count = convert_block<big_endian>(bytes, prev1, prev2, prev3,
                                                   active, output + written);
    if (count == SIZE_MAX) {
      goto invalid;
    }
    return written + count;
  }
  return written;
invalid:
  *fail = input;
  *written_before_fail = written;
  return SIZE_MAX;
}

template <endianness big_endian>
simdutf_really_inline __m512i decode_quad(__m512i bytes, __m512i *invalid) {
  const __m512i tags = _mm512_xor_si512(
      _mm512_and_si512(bytes, _mm512_set1_epi32(int32_t(0xc0c0c0f8u))),
      _mm512_set1_epi32(int32_t(0x808080f0u)));
  const __m512i indices = _mm512_broadcast_i32x4(
      _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12));
  const __m512i payload = _mm512_and_si512(_mm512_shuffle_epi8(bytes, indices),
                                           _mm512_set1_epi32(0x073f3f3f));
  const __m512i pairs =
      _mm512_maddubs_epi16(payload, _mm512_set1_epi32(0x40014001));
  const __m512i points =
      _mm512_sub_epi32(_mm512_madd_epi16(pairs, _mm512_set1_epi32(0x10000001)),
                       _mm512_set1_epi32(0x10000));
  *invalid = _mm512_or_si512(
      tags, _mm512_and_si512(points, _mm512_set1_epi32(-0x100000)));
  const __m512i high = _mm512_srli_epi32(points, 10);
  const __m512i low =
      _mm512_slli_epi32(_mm512_and_si512(points, _mm512_set1_epi32(0x3ff)), 16);
  return to_endian<big_endian>(_mm512_ternarylogic_epi32(
      high, low, _mm512_set1_epi32(int32_t(0xdc00d800u)), 0xfe));
}

template <endianness big_endian>
simdutf_really_inline size_t convert_dense(const uint8_t *input, size_t len,
                                           char16_t *output) {
  size_t written = 0;
  if (len >= 512) {
    __m512i first = _mm512_loadu_si512((const __m512i *)input);
    __m512i second = _mm512_loadu_si512((const __m512i *)(input + 64));
    __m512i third = _mm512_loadu_si512((const __m512i *)(input + 128));
    __m512i fourth = _mm512_loadu_si512((const __m512i *)(input + 192));
    for (; len >= 512; input += 256, len -= 256, written += 128) {
      const __m512i next_first =
          _mm512_loadu_si512((const __m512i *)(input + 256));
      const __m512i next_second =
          _mm512_loadu_si512((const __m512i *)(input + 320));
      const __m512i next_third =
          _mm512_loadu_si512((const __m512i *)(input + 384));
      const __m512i next_fourth =
          _mm512_loadu_si512((const __m512i *)(input + 448));
      __m512i invalid_first, invalid_second, invalid_third, invalid_fourth;
      const __m512i words_first =
          decode_quad<big_endian>(first, &invalid_first);
      const __m512i words_second =
          decode_quad<big_endian>(second, &invalid_second);
      const __m512i words_third =
          decode_quad<big_endian>(third, &invalid_third);
      const __m512i words_fourth =
          decode_quad<big_endian>(fourth, &invalid_fourth);
      const __m512i invalid =
          _mm512_or_si512(_mm512_or_si512(invalid_first, invalid_second),
                          _mm512_or_si512(invalid_third, invalid_fourth));
      if (_mm512_test_epi32_mask(invalid, invalid)) {
        break;
      }
      _mm512_storeu_si512(output + written, words_first);
      _mm512_storeu_si512(output + written + 32, words_second);
      _mm512_storeu_si512(output + written + 64, words_third);
      _mm512_storeu_si512(output + written + 96, words_fourth);
      first = next_first;
      second = next_second;
      third = next_third;
      fourth = next_fourth;
    }
  }
  for (; len >= 64; input += 64, len -= 64, written += 32) {
    __m512i invalid;
    const __m512i words = decode_quad<big_endian>(
        _mm512_loadu_si512((const __m512i *)input), &invalid);
    if (_mm512_test_epi32_mask(invalid, invalid)) {
      break;
    }
    _mm512_storeu_si512(output + written, words);
  }
  return written * 2;
}

template <endianness big_endian>
simdutf_really_inline size_t convert(const uint8_t *input, size_t len,
                                     char16_t *output, const uint8_t **fail,
                                     size_t *written_before_fail) {
  size_t consumed = 0;
  if (simdutf_unlikely(len >= 64 && (input[0] & 0xf8) == 0xf0)) {
    consumed = convert_dense<big_endian>(input, len, output);
  }
  const size_t written = consumed / 2;
  if (consumed == len) {
    return written;
  }
  const size_t count =
      convert_mixed<big_endian>(input + consumed, len - consumed,
                                output + written, fail, written_before_fail);
  if (count == SIZE_MAX) {
    *written_before_fail += written;
    return SIZE_MAX;
  }
  return written + count;
}

template <endianness big_endian>
size_t convert_utf8_to_utf16(const char *buf, size_t len, char16_t *output) {
  const uint8_t *input = reinterpret_cast<const uint8_t *>(buf);
  if (incomplete_end(input, len)) {
    return 0;
  }
  const uint8_t *fail;
  size_t written_before_fail;
  const size_t count =
      convert<big_endian>(input, len, output, &fail, &written_before_fail);
  return count == SIZE_MAX ? 0 : count;
}

template <endianness big_endian>
simdutf::result convert_utf8_to_utf16_with_errors(const char *buf, size_t len,
                                                  char16_t *output) {
  const uint8_t *input = reinterpret_cast<const uint8_t *>(buf);
  size_t prefix = len;
  while (incomplete_end(input, prefix)) {
    prefix = prefix >= 3 && input[prefix - 3] >= 0xf0   ? prefix - 3
             : prefix >= 2 && input[prefix - 2] >= 0xe0 ? prefix - 2
                                                        : prefix - 1;
  }
  const uint8_t *fail = input + prefix;
  size_t written = 0;
  const size_t count =
      convert<big_endian>(input, prefix, output, &fail, &written);
  if (count != SIZE_MAX) {
    if (prefix == len) {
      return simdutf::result(error_code::SUCCESS, count);
    }
    written = count;
  }
  size_t restart = size_t(fail - input);
  if (restart > 0) {
    size_t lead = restart - 1;
    for (int back = 0; back < 3 && lead > 0 && (input[lead] & 0xc0) == 0x80;
         back++) {
      lead--;
    }
    const uint8_t c = input[lead];
    const size_t seq_len = c < 0xc0 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
    if (lead + seq_len > restart) {
      if (c >= 0xf0 && restart - lead == 3) {
        written--;
      }
      restart = lead;
    }
  }
  SIMDUTF_VZEROUPPER();
  simdutf::result res = scalar::utf8_to_utf16::convert_with_errors<big_endian>(
      buf + restart, len - restart, output + written);
  if (res.error) {
    res.count += restart;
  } else {
    res.count += written;
  }
  return res;
}

} // namespace utf8_to_utf16_fast
