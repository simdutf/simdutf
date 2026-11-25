// file included directly

/*
 * Adapted from ffutf by Peter Fors (MIT licensed):
 * https://github.com/vtlmks/ffutf
 */
namespace utf16_to_utf8_fast {

alignas(64) constexpr uint8_t utf8_triplets[64] = {
    1,  2,  3,  5,  6,  7,  9,  10, 11, 13, 14, 15, 17, 18, 19, 21,
    22, 23, 25, 26, 27, 29, 30, 31, 33, 34, 35, 37, 38, 39, 41, 42,
    43, 45, 46, 47, 49, 50, 51, 53, 54, 55, 57, 58, 59, 61, 62, 63,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

alignas(64) constexpr uint16_t utf8_pairs[32] = {
    0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8,
    8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16};

template <endianness big_endian>
simdutf_really_inline __m512i load(const char16_t *input) {
  const __m512i words = _mm512_loadu_si512((const __m512i *)input);
  if (match_system(big_endian)) {
    return words;
  }
  return _mm512_shuffle_epi8(
      words, _mm512_broadcast_i32x4(_mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8,
                                                  11, 10, 13, 12, 15, 14)));
}

template <endianness big_endian>
simdutf_really_inline __m512i maskz_load(__mmask32 mask,
                                         const char16_t *input) {
  const __m512i words = _mm512_maskz_loadu_epi16(mask, input);
  if (match_system(big_endian)) {
    return words;
  }
  return _mm512_shuffle_epi8(
      words, _mm512_broadcast_i32x4(_mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8,
                                                  11, 10, 13, 12, 15, 14)));
}

template <endianness big_endian>
simdutf_really_inline uint64_t load_prefix(const char16_t *input) {
  uint64_t prefix;
  std::memcpy(&prefix, input, sizeof(prefix));
  if (!match_system(big_endian)) {
    prefix = ((prefix & UINT64_C(0x00ff00ff00ff00ff)) << 8) |
             ((prefix >> 8) & UINT64_C(0x00ff00ff00ff00ff));
  }
  return prefix;
}

simdutf_really_inline size_t pack(__m512i bytes, uint64_t keep, char *output) {
  const size_t count = size_t(count_ones(keep));
  _mm512_mask_storeu_epi8(output, (UINT64_C(1) << count) - 1,
                          _mm512_maskz_compress_epi8(keep, bytes));
  return count;
}

simdutf_really_inline __m512i payload(__m512i points, __m512i tags) {
  const __m512i bytes = _mm512_multishift_epi64_epi8(
      _mm512_set1_epi64(0x20262c3200060c12), points);
  return _mm512_ternarylogic_epi32(bytes, _mm512_set1_epi32(0x3f3f3f3f), tags,
                                   0xea);
}

simdutf_really_inline size_t encode_half(__m512i points, __mmask16 ascii,
                                         __mmask16 wide, __mmask16 high,
                                         __mmask16 emit, char *output) {
  __m512i tags =
      _mm512_mask_blend_epi32(wide, _mm512_set1_epi32(int32_t(0x80c00000u)),
                              _mm512_set1_epi32(int32_t(0x8080e000u)));
  tags = _mm512_mask_mov_epi32(tags, high,
                               _mm512_set1_epi32(int32_t(0x808080f0u)));
  tags = _mm512_mask_mov_epi32(tags, ascii, _mm512_set1_epi32(INT32_MIN));
  tags = _mm512_maskz_mov_epi32(emit, tags);
  const uint64_t keep = _mm512_movepi8_mask(tags);
  __m512i bytes = payload(points, tags);
  bytes = _mm512_mask_slli_epi32(bytes, ascii, points, 24);
  return pack(bytes, keep, output);
}

simdutf_really_inline __m512i surrogate_weights() {
  return _mm512_set1_epi32(0x00010400);
}

simdutf_really_inline __m512i surrogate_bias() {
  return _mm512_set1_epi32(0x10000 - (0xd800 - 0x10000) * 1024 -
                           (0xdc00 - 0x10000));
}

simdutf_really_inline __m512i encode_surrogates(__m512i words) {
  const __m512i points = _mm512_add_epi32(
      _mm512_madd_epi16(words, surrogate_weights()), surrogate_bias());
  return payload(points, _mm512_set1_epi32(int32_t(0x808080f0u)));
}

simdutf_really_inline size_t convert_block(__m512i words, uint32_t active,
                                           bool more, size_t *step,
                                           char *output) {
  const uint32_t multibyte =
      _mm512_cmpge_epu16_mask(words, _mm512_set1_epi16(0x80));
  if (!multibyte) {
    _mm512_mask_cvtepi16_storeu_epi8(output, active, words);
    return *step;
  }
  const uint32_t wide =
      _mm512_cmpge_epu16_mask(words, _mm512_set1_epi16(0x800));
  if (!wide) {
    const __m512i parts = _mm512_ternarylogic_epi32(
        _mm512_srli_epi16(words, 6), _mm512_slli_epi16(words, 8),
        _mm512_set1_epi16(0x3f3f), 0xa8);
    const __m512i bytes = _mm512_mask_add_epi16(
        words, multibyte, parts, _mm512_set1_epi16(int16_t(0x80c0)));
    const uint64_t keep =
        (_mm512_movepi8_mask(bytes) | UINT64_C(0x5555555555555555)) &
        (UINT64_MAX >> (64 - 2 * *step));
    if (keep == UINT64_MAX) {
      _mm512_storeu_si512(output, bytes);
      return 64;
    }
    return pack(bytes, keep, output);
  }
  const __m512i signature =
      _mm512_and_si512(words, _mm512_set1_epi16(int16_t(0xfc00)));
  uint32_t high =
      _mm512_cmpeq_epi16_mask(signature, _mm512_set1_epi16(int16_t(0xd800)));
  const uint32_t low =
      _mm512_cmpeq_epi16_mask(signature, _mm512_set1_epi16(int16_t(0xdc00)));
  if (high >> 31) {
    if (!more) {
      return SIZE_MAX;
    }
    high &= UINT32_MAX >> 1;
    active &= UINT32_MAX >> 1;
    --*step;
  }
  if ((high << 1) != low) {
    return SIZE_MAX;
  }
  if (high == UINT32_C(0x55555555)) {
    _mm512_storeu_si512(output, encode_surrogates(words));
    return 64;
  }
  __m512i first = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(words));
  __m512i second = _mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64(words, 1));
  if (high) {
    const __m512i indices = _mm512_load_si512((const __m512i *)utf8_pairs);
    const __m512i next = _mm512_add_epi16(indices, _mm512_set1_epi16(16));
    const __m512i pairs_first = _mm512_permutexvar_epi16(indices, words);
    const __m512i pairs_second = _mm512_permutexvar_epi16(next, words);
    first = _mm512_mask_add_epi32(
        first, __mmask16(high),
        _mm512_madd_epi16(pairs_first, surrogate_weights()), surrogate_bias());
    second = _mm512_mask_add_epi32(
        second, __mmask16(high >> 16),
        _mm512_madd_epi16(pairs_second, surrogate_weights()), surrogate_bias());
  } else if (active == UINT32_MAX && wide == UINT32_MAX) {
    const __m512i tags = _mm512_set1_epi32(int32_t(0x8080e000u));
    const __m512i indices = _mm512_load_si512((const __m512i *)utf8_triplets);
    const uint64_t mask = UINT64_MAX >> 16;
    _mm512_mask_storeu_epi8(
        output, mask, _mm512_permutexvar_epi8(indices, payload(first, tags)));
    _mm512_mask_storeu_epi8(
        output + 48, mask,
        _mm512_permutexvar_epi8(indices, payload(second, tags)));
    return 96;
  }
  const uint32_t ascii = ~multibyte;
  const uint32_t emit = active & ~low;
  size_t count = encode_half(first, __mmask16(ascii), __mmask16(wide),
                             __mmask16(high), __mmask16(emit), output);
  if (emit >> 16) {
    count += encode_half(second, __mmask16(ascii >> 16), __mmask16(wide >> 16),
                         __mmask16(high >> 16), __mmask16(emit >> 16),
                         output + count);
  }
  return count;
}

simdutf_really_inline bool surrogate_group(__m512i first, __m512i second,
                                           __m512i third, __m512i fourth) {
  const __m512i tags = _mm512_set1_epi32(int32_t(0xdc00d800u));
  const __m512i mask = _mm512_set1_epi32(int32_t(0xfc00fc00u));
  const __m512i mismatch =
      _mm512_or_si512(_mm512_ternarylogic_epi32(first, second, tags, 0x7e),
                      _mm512_ternarylogic_epi32(third, fourth, tags, 0x7e));
  return !_mm512_test_epi32_mask(mismatch, mask);
}

simdutf_really_inline void store_surrogate_group(char *output, __m512i first,
                                                 __m512i second, __m512i third,
                                                 __m512i fourth) {
  _mm512_storeu_si512(output, encode_surrogates(first));
  _mm512_storeu_si512(output + 64, encode_surrogates(second));
  _mm512_storeu_si512(output + 128, encode_surrogates(third));
  _mm512_storeu_si512(output + 192, encode_surrogates(fourth));
}

template <endianness big_endian>
size_t convert(const char16_t *input, size_t len, char *output,
               size_t *outlen) {
  const char16_t *const start = input;
  size_t written = 0;
  for (; len >= 128; input += 128, len -= 128, written += 128) {
    const __m512i first = load<big_endian>(input);
    const __m512i second = load<big_endian>(input + 32);
    const __m512i third = load<big_endian>(input + 64);
    const __m512i fourth = load<big_endian>(input + 96);
    const __m512i combined = _mm512_or_si512(
        _mm512_ternarylogic_epi32(first, second, third, 0xfe), fourth);
    if (_mm512_test_epi16_mask(combined, _mm512_set1_epi16(-128))) {
      break;
    }
    _mm512_mask_cvtepi16_storeu_epi8(output + written, UINT32_MAX, first);
    _mm512_mask_cvtepi16_storeu_epi8(output + written + 32, UINT32_MAX, second);
    _mm512_mask_cvtepi16_storeu_epi8(output + written + 64, UINT32_MAX, third);
    _mm512_mask_cvtepi16_storeu_epi8(output + written + 96, UINT32_MAX, fourth);
  }
  if (simdutf_unlikely(len >= 128 &&
                       (load_prefix<big_endian>(input) & 0xfc00) == 0xd800)) {
    __m512i first = load<big_endian>(input);
    __m512i second = load<big_endian>(input + 32);
    __m512i third = load<big_endian>(input + 64);
    __m512i fourth = load<big_endian>(input + 96);
    if (len >= 256) {
      __m512i lead = load<big_endian>(input + 128);
      for (; len >= 256; input += 128, len -= 128, written += 256) {
        if (!surrogate_group(first, second, third, fourth)) {
          break;
        }
        const __m512i next_first = lead;
        const __m512i next_second = load<big_endian>(input + 160);
        const __m512i next_third = load<big_endian>(input + 192);
        const __m512i next_fourth = load<big_endian>(input + 224);
        if (len >= 288) {
          lead = load<big_endian>(input + 256);
        }
        store_surrogate_group(output + written, first, second, third, fourth);
        first = next_first;
        second = next_second;
        third = next_third;
        fourth = next_fourth;
      }
    }
    if (len < 256 && surrogate_group(first, second, third, fourth)) {
      store_surrogate_group(output + written, first, second, third, fourth);
      input += 128;
      len -= 128;
      written += 256;
    }
  }
  if (len <= 256 && len >= 64) {
    const uint64_t prefix = load_prefix<big_endian>(input);
    if (simdutf_unlikely(len <= 96 && !(len & 1) &&
                         (uint16_t(prefix) & 0xfc00) == 0xd800)) {
      const __m512i first = load<big_endian>(input);
      const __m512i second = load<big_endian>(input + 32);
      const __m512i last = load<big_endian>(input + len - 32);
      if (surrogate_group(first, second, last, last)) {
        _mm512_storeu_si512(output + written, encode_surrogates(first));
        _mm512_storeu_si512(output + written + 64, encode_surrogates(second));
        _mm512_storeu_si512(output + written + 2 * (len - 32),
                            encode_surrogates(last));
        *outlen = written + 2 * len;
        return size_t(input - start) + len;
      }
    }
    if (len <= 96 && !(prefix & UINT64_C(0xff80ff80ff80ff80))) {
      const __m512i first = load<big_endian>(input);
      const __m512i second = load<big_endian>(input + 32);
      const __m512i last = load<big_endian>(input + len - 32);
      const __m512i combined =
          _mm512_ternarylogic_epi32(first, second, last, 0xfe);
      if (!_mm512_test_epi16_mask(combined, _mm512_set1_epi16(-128))) {
        _mm512_mask_cvtepi16_storeu_epi8(output + written, UINT32_MAX, first);
        _mm512_mask_cvtepi16_storeu_epi8(output + written + 32, UINT32_MAX,
                                         second);
        _mm512_mask_cvtepi16_storeu_epi8(output + written + len - 32,
                                         UINT32_MAX, last);
        *outlen = written + len;
        return size_t(input - start) + len;
      }
    }
    if (uint16_t(prefix) < 0x800 && (prefix & UINT64_C(0xff80ff80ff80ff80))) {
      __m512i words = load<big_endian>(input);
      __m512i next = load<big_endian>(input + 32);
      for (;;) {
        __m512i following = next;
        if (len >= 96) {
          following = load<big_endian>(input + 64);
        }
        size_t step = 32;
        const size_t count =
            convert_block(words, UINT32_MAX, len > 32, &step, output + written);
        if (count == SIZE_MAX) {
          *outlen = written;
          return size_t(input - start);
        }
        input += step;
        len -= step;
        written += count;
        if (len < 32) {
          break;
        }
        if (step == 32) {
          words = next;
          next = following;
        } else {
          words = load<big_endian>(input);
          if (len >= 64) {
            next = load<big_endian>(input + 32);
          }
        }
      }
    } else if (simdutf_unlikely(uint16_t(prefix) >= 0x800 &&
                                uint32_t(uint16_t(prefix)) - 0xd800 >= 0x800)) {
      for (; len >= 32; input += 32, len -= 32, written += 96) {
        const __m512i words = load<big_endian>(input);
        const __mmask32 narrow =
            _mm512_cmplt_epu16_mask(words, _mm512_set1_epi16(0x800));
        const __mmask32 surrogate = _mm512_cmplt_epu16_mask(
            _mm512_sub_epi16(words, _mm512_set1_epi16(int16_t(0xd800))),
            _mm512_set1_epi16(0x800));
        if ((narrow | surrogate) != 0) {
          break;
        }
        __m512i pairs =
            _mm512_ternarylogic_epi32(words, _mm512_slli_epi16(words, 2),
                                      _mm512_set1_epi16(0x00ff), 0xe4);
        pairs =
            _mm512_ternarylogic_epi32(pairs, _mm512_set1_epi16(0x3f3f),
                                      _mm512_set1_epi16(int16_t(0x8080)), 0xea);
        const __m512i leads = _mm512_or_si512(_mm512_srli_epi16(words, 12),
                                              _mm512_set1_epi16(0xe0));
        const __m512i index_first = _mm512_setr_epi64(
            0x0544020342000140, 0x4a08094806074604, 0x0e0f4e0c0d4c0a0b,
            0x1554121352101150, 0x5a18195816175614, 0x1e1f5e1c1d5c1a1b,
            0x2564222362202160, 0x6a28296826276624);
        const __m512i index_second = _mm512_setr_epi64(
            0x2e2f6e2c2d6c2a2b, 0x3574323372303170, 0x7a38397836377634,
            0x3e3f7e3c3d7c3a3b, 0, 0, 0, 0);
        _mm512_storeu_si512(output + written, _mm512_permutex2var_epi8(
                                                  pairs, index_first, leads));
        _mm256_storeu_si256((__m256i *)(output + written + 64),
                            _mm512_castsi512_si256(_mm512_permutex2var_epi8(
                                pairs, index_second, leads)));
      }
    }
  }
  if (simdutf_unlikely(len > 32 && len < 64)) {
    size_t tail = len - 32;
    const uint32_t active = (UINT32_C(1) << tail) - 1;
    const __m512i first = load<big_endian>(input);
    const __m512i last = maskz_load<big_endian>(active, input + 32);
    size_t step = 32;
    const size_t count =
        convert_block(first, UINT32_MAX, true, &step, output + written);
    if (count == SIZE_MAX) {
      *outlen = written;
      return size_t(input - start);
    }
    if (step == 32) {
      const size_t final_count =
          convert_block(last, active, false, &tail, output + written + count);
      if (final_count == SIZE_MAX) {
        *outlen = written + count;
        return size_t(input - start) + 32;
      }
      *outlen = written + count + final_count;
      return size_t(input - start) + len;
    }
    input += step;
    len -= step;
    written += count;
  }
  while (len >= 32) {
    size_t step = 32;
    const size_t count = convert_block(load<big_endian>(input), UINT32_MAX,
                                       len > 32, &step, output + written);
    if (count == SIZE_MAX) {
      *outlen = written;
      return size_t(input - start);
    }
    input += step;
    len -= step;
    written += count;
  }
  if (len) {
    const uint32_t active = (UINT32_C(1) << len) - 1;
    const __m512i words = maskz_load<big_endian>(active, input);
    size_t step = len;
    const size_t count =
        convert_block(words, active, false, &step, output + written);
    if (count == SIZE_MAX) {
      *outlen = written;
      return size_t(input - start);
    }
    written += count;
    input += len;
  }
  *outlen = written;
  return size_t(input - start);
}

} // namespace utf16_to_utf8_fast
