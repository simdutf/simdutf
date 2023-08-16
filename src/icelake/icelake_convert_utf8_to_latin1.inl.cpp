// file included directly

// File contains conversion procedure from possibly invalid UTF-8 strings.


void printbinary(uint64_t n) {
  for (size_t i = 0; i < 64; i++) {
    if (n & 1)
      printf("1");
    else
      printf("0");

    n >>= 1;
  }
  printf("\n");
}
void print8(const char *name, __m512i x) {
  printf("%.32s : ", name);
  uint8_t buffer[64];
  _mm512_storeu_si512((__m512i *)buffer, x);
  for (size_t i = 0; i < 64; i++) {
    printf("%02x ", buffer[i]);
  }
  printf("\n");
}


template <bool is_remaining>
simdutf_really_inline size_t process_block(const char *buf, size_t len, char *latin_output,
                     __m512i minus64, __m512i one,
                     __mmask64 *next_leading_ptr, __mmask64 *next_bit6_ptr,__mmask64 load_mask = ~0ULL) {
    // __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)buf);
    __m512i input = is_remaining ? _mm512_maskz_loadu_epi8(load_mask, (__m512i *)buf) : _mm512_loadu_si512((__m512i *)buf);
    __mmask64 nonascii = _mm512_movepi8_mask(input);

    if (nonascii == 0) {
        // _mm512_mask_storeu_epi8((__m512i *)latin_output, load_mask, input);
        is_remaining ? _mm512_mask_storeu_epi8((__m512i *)latin_output, load_mask, input) : _mm512_storeu_si512((__m512i *)latin_output, input);
        return len;
    }

    __mmask64 leading = _mm512_cmpge_epu8_mask(input, minus64);

    __m512i highbits = _mm512_xor_si512(input, _mm512_set1_epi8(-62));
    __mmask64 invalid_leading_bytes = _mm512_mask_cmpgt_epu8_mask(leading, highbits, one);

    if (invalid_leading_bytes) {
        return 0; // Indicates error
    }

    __mmask64 leading_shift = (leading << 1) | *next_leading_ptr;
    *next_leading_ptr = leading >> 63;

    if ((nonascii ^ leading) != leading_shift) {
        return 0; // Indicates error
    }

    __mmask64 bit6 = _mm512_cmpeq_epi8_mask(highbits, one);
    input = _mm512_mask_sub_epi8(input, (bit6 << 1) | *next_bit6_ptr, input, minus64);
    *next_bit6_ptr = bit6 >> 63;

    __mmask64 retain = ~leading & load_mask;
    __m512i output = _mm512_maskz_compress_epi8(retain, input);
    int64_t written_out = _popcnt64(retain);
    __mmask64 store_mask = (1ULL << written_out) - 1;

    // _mm512_mask_storeu_epi8((__m512i *)latin_output, store_mask, output);
    is_remaining ? _mm512_mask_storeu_epi8((__m512i *)latin_output, store_mask, output) : _mm512_storeu_si512((__m512i *)latin_output, output);
    return written_out;
}

size_t utf8_to_latin1_avx512(const char *buf, size_t len, char *latin_output) {
    char *start = latin_output;
    size_t pos = 0;
    __m512i minus64 = _mm512_set1_epi8(-64); // 11111111111 ... 1100 0000
    __m512i one = _mm512_set1_epi8(1);
    __mmask64 next_leading = 0;
    __mmask64 next_bit6 = 0;
    __mmask64 load_mask = ~0ULL;

    while (pos + 64 <= len) {
        size_t written = process_block<false>(buf + pos, 64, latin_output, minus64, one, &next_leading, &next_bit6 , load_mask);
        if (written == 0) {
            return 0; // Indicates error
        }
        latin_output += written;
        pos += 64;
    }

    load_mask = _bzhi_u64(~0ULL, len);
    if (pos < len) {
        size_t remaining = len - pos;
        size_t written = process_block<true>(buf + pos, remaining, latin_output, minus64, one, &next_leading, &next_bit6 , load_mask);
        if (written == 0) {
            return 0; // Indicates error
        }
        latin_output += written;
    }

    return (size_t)(latin_output - start);
}


/* size_t utf8_to_latin1_avx512(const char *buf, size_t len, char *latin_output) {
    char *start = latin_output;
    size_t pos = 0;
    __m512i minus64 = _mm512_set1_epi8(-64); // 11111111111 ... 1100 0000
    __m512i one = _mm512_set1_epi8(1);
    __mmask64 next_leading = 0;
    __mmask64 next_bit6 = 0;

    while (pos < len) {
        size_t block_len = (pos + 64 <= len) ? 64 : len - pos;
        __mmask64 load_mask = (block_len < 64) ? _bzhi_u64(~0ULL, block_len) : ~0ULL;
        __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)(buf + pos));
        __mmask64 nonascii = _mm512_movepi8_mask(input);

        if (nonascii == 0) {
            _mm512_mask_storeu_epi8((__m512i *)latin_output, load_mask, input);
            latin_output += block_len;
            pos += block_len;
            continue;
        }

        __mmask64 leading = _mm512_cmpge_epu8_mask(input, minus64);

        __m512i highbits = _mm512_xor_si512(input, _mm512_set1_epi8(-62));
        __mmask64 invalid_leading_bytes = _mm512_mask_cmpgt_epu8_mask(leading, highbits, one);

        if (invalid_leading_bytes) {
            return 0; // Indicates error
        }

        __mmask64 leading_shift = (leading << 1) | next_leading;
        next_leading = leading >> 63;

        if ((nonascii ^ leading) != leading_shift) {
            return 0; // Indicates error
        }

        __mmask64 bit6 = _mm512_cmpeq_epi8_mask(highbits, one);
        input = _mm512_mask_sub_epi8(input, (bit6 << 1) | next_bit6, input, minus64);
        next_bit6 = bit6 >> 63;

        __mmask64 retain = ~leading & load_mask;
        __m512i output = _mm512_maskz_compress_epi8(retain, input);
        int64_t written_out = _popcnt64(retain);
        __mmask64 store_mask = (1ULL << written_out) - 1;

        _mm512_mask_storeu_epi8((__m512i *)latin_output, store_mask, output);
        latin_output += written_out;
        pos += block_len;
    }

    return (size_t)(latin_output - start);
}
 */


/* convert_utf8_to_latin1+icelake, input size: 440052, iterations: 10000, dataset: /home/leorio/simdutf/unicode_lipsum/wikipedia_mars/french.utflatin8.txt
   0.488 ins/byte,    0.179 cycle/byte,   17.345 GB/s (3.0 %),     3.100 GHz,    2.730 ins/cycle 
   0.497 ins/char,    0.182 cycle/char,   17.040 Gc/s (3.0 %)     1.02 byte/char  */

/* simdutf_really_inline size_t process_block(const char *buf, __mmask64 load_mask, char *latin_output,
                                           __m512i minus64, __m512i one,
                                           __mmask64 *next_leading_ptr, __mmask64 *next_bit6_ptr) {

    __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)buf);
    __mmask64 nonascii = _mm512_movepi8_mask(input);

    if (nonascii == 0) {
        _mm512_mask_storeu_epi8((__m512i *)latin_output, load_mask, input);
        return _popcnt64(load_mask);
    }


    __mmask64 leading = _mm512_cmpge_epu8_mask(input, minus64);

    __m512i highbits = _mm512_xor_si512(input, _mm512_set1_epi8(-62));
    __mmask64 invalid_leading_bytes = _mm512_mask_cmpgt_epu8_mask(leading, highbits, one);

    if (invalid_leading_bytes) {
        return 0; // Indicates error
    }

    __mmask64 leading_shift = (leading << 1) | *next_leading_ptr;
    *next_leading_ptr = leading >> 63;

    if ((nonascii ^ leading) != leading_shift) {
        return 0; // Indicates error
    }

    __mmask64 bit6 = _mm512_cmpeq_epi8_mask(highbits, one);
    input = _mm512_mask_sub_epi8(input, (bit6 << 1) | *next_bit6_ptr, input, minus64);
    *next_bit6_ptr = bit6 >> 63;

    __mmask64 retain = ~leading & load_mask;
    __m512i output = _mm512_maskz_compress_epi8(retain, input);
    int64_t written_out = _popcnt64(retain);
    __mmask64 store_mask = (1ULL << written_out) - 1;

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
        size_t written = process_block(buf + pos, ~0ULL, latin_output, minus64, one, &next_leading, &next_bit6);
        if (written == 0) {
            return 0; // Indicates error
        }
        latin_output += written;
        pos += 64;
    }

    if (pos < len) {
        __mmask64 tail_mask = _bzhi_u64(~0ULL, len - pos);
        size_t written = process_block(buf + pos, tail_mask, latin_output, minus64, one, &next_leading, &next_bit6);
        if (written == 0) {
            return 0; // Indicates error
        }
        latin_output += written;
    }

    return (size_t)(latin_output - start);
}
 */