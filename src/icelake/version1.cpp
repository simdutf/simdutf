// file included directly

// File contains conversion procedure from possibly invalid UTF-8 strings.

/*
par:
convert_utf8_to_latin1+icelake, input size: 440052, iterations: 3000, dataset: french.utflatin8.txt
   0.434 ins/byte,    0.171 cycle/byte,   18.167 GB/s (3.9 %),     3.099 GHz,    2.544 ins/cycle 
   0.442 ins/char,    0.174 cycle/char,   17.848 Gc/s (3.9 %)     1.02 byte/char   */

size_t utf8_to_latin1_avx512(const char *buf, size_t len,
                                  char *latin_output) {
  char *start = latin_output;
  size_t pos = 0;
  __m512i minus64 = _mm512_set1_epi8(-64); // 11111111111 ... 1100 0000
  __m512i one = _mm512_set1_epi8(1);
  __mmask64 next_leading = 0;
  __mmask64 next_bit6 = 0;

  while (pos + 64 <= len) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    __mmask64 nonascii = _mm512_movepi8_mask(input);
    if (nonascii == 0) {
      _mm512_storeu_si512(latin_output, input);
      latin_output += 64;
      pos += 64;
      continue;
    }

    __mmask64 leading = _mm512_cmpge_epu8_mask(input, minus64);

    // disallow non-latin1 chars
    
    // highbits = 1 if 1100 0011
    //            0 if 1100 0010
    __m512i highbits = _mm512_xor_si512(
                                        input, 
                                        _mm512_set1_epi8(-62)); //111111.... 1100 0010
    __mmask64 invalid_leading_bytes =
        _mm512_mask_cmpgt_epu8_mask(leading, highbits, one);
    if (invalid_leading_bytes) {
      return 0;
    } // We have an invalid leading byte.

    __mmask64 leading_shift = (leading << 1) | next_leading;
    next_leading = leading >> 63;
    // leading bytes must be paired with a continuation
    if ((nonascii ^ leading) != leading_shift) {
      return 0;
    }

    // mask for 1100 0011 byte
    __mmask64 bit6 = _mm512_cmpeq_epi8_mask(highbits, one);
    // We remove the leading two bits from the 1100 0011 bytes
    input =
        _mm512_mask_sub_epi8(input, (bit6 << 1) | next_bit6, input, minus64);
    next_bit6 = bit6 >> 63;

    // We retain everything that's not a leading byte
    __mmask64 retain = ~leading;
    __m512i output = _mm512_maskz_compress_epi8(retain, input);
    int64_t written_out = _popcnt64(retain);
    __mmask64 store_mask = (1ULL << written_out) - 1;
    _mm512_mask_storeu_epi8((__m512i *)latin_output, store_mask, output);
    //_mm512_mask_compressstoreu_epi8((__m512i*)latin_output, retain, input); //
    // WARNING: bad on Zen4
    latin_output += written_out;
    pos += 64;
  }
  // We repeat the code, this could be reengineered to be nicer.
  if (pos < len) {
    __mmask64 load_mask = _bzhi_u64(~0ULL, len - pos);
    __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)(buf + pos));
    __mmask64 nonascii = _mm512_movepi8_mask(input);
    if (nonascii == 0) {
      _mm512_mask_storeu_epi8(latin_output, load_mask, input);
      latin_output += len - pos;
    } else {
      __mmask64 leading = _mm512_cmpge_epu8_mask(input, minus64);

      // disallow non-latin1 chars
      __m512i highbits = _mm512_xor_si512(input, _mm512_set1_epi8(-62));
      __mmask64 invalid_leading_bytes =
          _mm512_mask_cmpgt_epu8_mask(leading, highbits, one);
      if (invalid_leading_bytes) {
        return 0;
      } // We have an invalid leading byte.

      __mmask64 leading_shift = (leading << 1) | next_leading;
      // leading bytes must be paired with a continuation
      if ((nonascii ^ leading) != leading_shift) {
        return 0;
      }

      __mmask64 bit6 = _mm512_cmpeq_epi8_mask(highbits, one);
      input =
          _mm512_mask_sub_epi8(input, (bit6 << 1) | next_bit6, input, minus64);

      __mmask64 retain = ~leading & load_mask;
      //_mm512_mask_compressstoreu_epi8((__m512i*)latin_output, retain, input);
      //// WARNING: bad on Zen4
      __m512i output = _mm512_maskz_compress_epi8(retain, input);
      int64_t written_out = _popcnt64(retain);
      __mmask64 store_mask = (1ULL << written_out) - 1;
      _mm512_mask_storeu_epi8((__m512i *)latin_output, store_mask, output);
      latin_output += written_out;
    }
  }
  return (size_t)(latin_output - start);
}
