// file included directly

// File contains conversion procedure from possibly invalid UTF-8 strings.

// template <bool is_remaining, bool use_masked_store>
template <bool is_remaining>
simdutf_really_inline size_t process_block_from_utf8_to_latin1(const char *buf, size_t len,
                                           char *latin_output, __m512i minus64,
                                           __m512i one,
                                           __mmask64 *next_leading_ptr,
                                           __mmask64 *next_bit6_ptr) {
  __mmask64 load_mask =
      is_remaining ? _bzhi_u64(~0ULL, (unsigned int)len) : ~0ULL;
  __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)buf);
  __mmask64 nonascii = _mm512_movepi8_mask(input);

  if (nonascii == 0) {
    is_remaining
        ? _mm512_mask_storeu_epi8((__m512i *)latin_output, load_mask, input)
        : _mm512_storeu_si512((__m512i *)latin_output, input);
    return len;
  }

  __mmask64 leading = _mm512_cmpge_epu8_mask(input, minus64);

  __m512i highbits = _mm512_xor_si512(input, _mm512_set1_epi8(-62));
  __mmask64 invalid_leading_bytes =
      _mm512_mask_cmpgt_epu8_mask(leading, highbits, one);

  if (invalid_leading_bytes) {
    return 0; // Indicates error
  }

  __mmask64 leading_shift = (leading << 1) | *next_leading_ptr;
  *next_leading_ptr = leading >> 63;

  if ((nonascii ^ leading) != leading_shift) {
    return 0; // Indicates error
  }

  __mmask64 bit6 = _mm512_cmpeq_epi8_mask(highbits, one);
  input =
      _mm512_mask_sub_epi8(input, (bit6 << 1) | *next_bit6_ptr, input, minus64);
  *next_bit6_ptr = bit6 >> 63;

  __mmask64 retain = ~leading & load_mask;
  __m512i output = _mm512_maskz_compress_epi8(retain, input);
  int64_t written_out = count_ones(retain);
  __mmask64 store_mask = (1ULL << written_out) - 1;

  // ***************************
  //  Possible optimization? (Nick Nuon)
  //  This commented out line is 5% faster but sadly it'll also write past
  //  memory bounds for latin1_output: is_remaining ?
  //  _mm512_mask_storeu_epi8((__m512i *)latin_output, store_mask, output) :
  //  _mm512_storeu_si512((__m512i *)latin_output, output); I tried using
  //  _mm512_storeu_si512 and have the next process_block start from the
  //  "written_out" point but the compiler shuffles memory in such a way that it
  //  is significantly slower...
  // ****************************
  _mm512_mask_storeu_epi8((__m512i *)latin_output, store_mask, output);

  return written_out;
}

size_t utf8_to_latin1_avx512(const char *buf, size_t len, char *latin_output) {
  char *start = latin_output;
  size_t pos = 0;
  __m512i minus64 = _mm512_set1_epi8(-64); // 11111111111 ... 1100 0000
  __m512i one = _mm512_set1_epi8(1);
  __mmask64 next_leading = 0;
  __mmask64 next_bit6 = 0;

  while (pos + 64 <= len) {
    size_t written = process_block_from_utf8_to_latin1<false>(buf + pos, 64, latin_output, minus64,
                                          one, &next_leading, &next_bit6);
    if (written == 0) {
      return 0; // Indicates error
    }
    latin_output += written;
    pos += 64;
  }

  if (pos < len) {
    size_t remaining = len - pos;
    size_t written =
        process_block_from_utf8_to_latin1<true>(buf + pos, remaining, latin_output, minus64, one,
                            &next_leading, &next_bit6);
    if (written == 0) {
      return 0; // Indicates error
    }
    latin_output += written;
  }

  return (size_t)(latin_output - start);
}
