// file included directly

// File contains conversion procedure from possibly invalid UTF-8 strings.


/* 
size_t utf8_to_latin1(const char* buf, size_t len, char* latin_output) {
 const uint8_t *data = (const uint8_t *)(buf);
  size_t pos = 0;
  char* start = latin_output;

  while (pos < len) {
    // try to convert the next block of 16 ASCII bytes
    if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that they are ascii
      uint64_t v1;
      memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v = v1 | v2; // We are only interested in these bits: 1000 1000 1000 1000 .... etc
      if ((v & 0x8080808080808080) == 0) { // if NONE of these are set, e.g. all of them are zero, then everything is ASCII
        size_t final_pos = pos + 16;
        while(pos < final_pos) {
          *latin_output++ = (char)(buf[pos]);
          pos++;
        }
        continue;
      }
    }

    // suppose it is not an all ASCII byte sequence
    uint8_t leading_byte = data[pos]; // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *latin_output++ = (char)(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) { // the first three bits indicate:
      // We have a two-byte UTF-8
      if(pos + 1 >= len) {
         return 0; } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; } // checks if the next byte is a valid continuation byte in UTF-8. A valid continuation byte starts with 10.
      // range check -
      uint32_t code_point = (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111); // assembles the Unicode code point from the two bytes. It does this by discarding the leading 110 and 10 bits from the two bytes, shifting the remaining bits of the first byte, and then combining the results with a bitwise OR operation.
      if (code_point < 0x80 || 0xFF < code_point) {
        return 0; // We only care about the range 129-255 which is Non-ASCII latin1 characters. A code_point beneath 0x80 is invalid as it's already covered by bytes whose leading bit is zero. 
      }
      *latin_output++ = (char)(code_point);
      pos += 2;
    } else {
      return 0;
    }
  }
  return (size_t)(latin_output - start);
} */
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

/* size_t utf8_to_latin1_avx512_orig(const char* buf, size_t len, char* latin_output) {
  char* start = latin_output;
  size_t pos = 0;
  __m512i mask_80808080 = _mm512_set1_epi32((int)0x80808080);

  while(pos + 64 <= len) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    __mmask64 ascii = _mm512_cmplt_epu8_mask(input, mask_80808080);
    if(ascii == 0) {
      _mm512_storeu_si512(latin_output, input);
      latin_output += 64;
      pos += 64;
      continue;
    }
    __mmask64 continuation = _mm512_cmplt_epi8_mask(input, _mm512_set1_epi8(-64));
    __mmask64 leading = _kxnor_mask64(ascii, continuation);
    //
    // All continuation bytes *must* be preceded by a leading byte
    if((continuation>>1)&~leading) { abort();return 0; } // some continuation byte is not preceeded by a leading byte
    // We cannot start with a continuation byte:
    if(continuation & 1) { abort();return 0; }
    // All leading bytes *must* be followed by a continuation byte... except maybe for the last one
    if((leading<<1)^continuation) { abort();return 0; } // some continuation byte is not preceeded by a leading byte
    bool end_with_leading = leading & 0x8000000000000000; // that's possible, if so we are missing the continuation byte
    // Only two possible leading byte value: 
    // 0b11000011 and 0b11000010. Or 194 and 195 as unsigned byte values.
    // Adding 62 to it, we get 0 or 1.
    __m512i highbits = _mm512_maskz_add_epi8(leading, input, _mm512_set1_epi8(62));
    __mmask64 invalid_leading_bytes = _mm512_mask_cmpgt_epu8_mask(leading, highbits, _mm512_set1_epi8(1));
    if(invalid_leading_bytes) { abort();return 0; } // We have an invalid leading byte.
    highbits = _mm512_slli_epi16(highbits, 6); // shift in position
    input = _mm512_mask_blend_epi8(leading, input, highbits);
    //
    // We are going to load the ascii + continuation
    __m512i ascii_continuation = _mm512_maskz_compress_epi8(ascii | continuation, input);
    __m512i ascii_leading = _mm512_maskz_compress_epi8(ascii | leading, input);
    __m512i output = _mm512_or_si512(ascii_continuation, ascii_leading);
    size_t written_out = (size_t)(64 - _popcnt64(continuation));
    if(end_with_leading) { written_out -= 1;}
    __mmask64 store_mask = (1ULL << written_out) - 1;
    _mm512_mask_storeu_epi8((__m512i*)latin_output, store_mask, output); 
    latin_output += written_out;
    pos += 64;
    if(end_with_leading) { pos -= 1; }
  }
  // We repeat the code, this could be reengineered to be nicer.
  if(pos < len) {
    __mmask64 load_mask = (1ULL<<(len-pos)) - 1;
    __m512i input = _mm512_mask_loadu_epi8(_mm512_setzero_si512(), load_mask, (__m512i *)(buf + pos));
    __mmask64 ascii = _mm512_cmplt_epu8_mask(input, mask_80808080);
    __mmask64 fake_ascii = ascii &~ load_mask;
    __mmask64 continuation = _mm512_cmplt_epi8_mask(input, _mm512_set1_epi8(-64));
    __mmask64 leading = _kxnor_mask64(ascii, continuation);
    //
    // All continuation bytes *must* be preceded by a leading byte
    if((continuation>>1)&~leading) { abort(); return 0; } // some continuation byte is not preceeded by a leading byte
    // We cannot start with a continuation byte:
    if(continuation & 1) { abort(); return 0; }
    // All leading bytes *must* be followed by a continuation byte... except maybe for the last one
    if((leading<<1)^continuation) { abort(); return 0; } // some continuation byte is not preceeded by a leading byte
    // Only two possible leading byte value: 
    // 0b11000011 and 0b11000010. Or 194 and 195 as unsigned byte values.
    // Adding 62 to it, we get 0 or 1.
    __m512i highbits = _mm512_maskz_add_epi8(leading, input, _mm512_set1_epi8(62));
    __mmask64 invalid_leading_bytes = _mm512_mask_cmpgt_epu8_mask(leading, highbits, _mm512_set1_epi8(1));
    if(invalid_leading_bytes) { abort(); return 0; } // We have an invalid leading byte.
    highbits = _mm512_slli_epi16(highbits, 6); // shift in position
    input = _mm512_mask_blend_epi8(leading, input, highbits);
    //
    // We are going to load the ascii + continuation
    __m512i ascii_continuation = _mm512_maskz_compress_epi8(ascii | continuation, input);
    __m512i ascii_leading = _mm512_maskz_compress_epi8(ascii | leading, input);
    __m512i output = _mm512_or_si512(ascii_continuation, ascii_leading);
    size_t written_out = (size_t)(64 - _popcnt64(continuation|fake_ascii));
    __mmask64 store_mask = (1ULL << written_out) - 1;
    _mm512_mask_storeu_epi8((__m512i*)latin_output, store_mask, output); 
    latin_output += written_out;
  }
  return (size_t)(latin_output - start);
} */





/* par:
convert_utf8_to_latin1+icelake, input size: 440052, iterations: 3000, dataset: french.utflatin8.txt
   0.434 ins/byte,    0.171 cycle/byte,   18.167 GB/s (3.9 %),     3.099 GHz,    2.544 ins/cycle 
   0.442 ins/char,    0.174 cycle/char,   17.848 Gc/s (3.9 %)     1.02 byte/char   */
/* 
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
 */

/* -=--------------------------------------- */
/* 
template <bool is_residual>
size_t process_chunk(const char *buf, size_t len, char *&latin_output,
                     __m512i minus64, __m512i one,
                     __mmask64 &next_leading, __mmask64 &next_bit6) {

    __mmask64 load_mask = is_residual ? _bzhi_u64(~0ULL, len) : ~0ULL;
    __m512i input = is_residual ? _mm512_maskz_loadu_epi8(load_mask, (__m512i *)buf) 
                                : _mm512_loadu_si512((__m512i *)buf);
    
    __mmask64 nonascii = _mm512_movepi8_mask(input);
    if (nonascii == 0) {
        if (is_residual) {
            _mm512_mask_storeu_epi8(latin_output, load_mask, input);
            latin_output += len;
        } else {
            _mm512_storeu_si512(latin_output, input);
            latin_output += 64;
        }
        return len;
    }

    // ... [rest of the conversion code remains mostly unchanged] ...


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
    // pos += 64;
    return len;
  }

// 

    if(is_residual) {
        __mmask64 retain = ~leading & load_mask;
        __m512i output = _mm512_maskz_compress_epi8(retain, input);
        int64_t written_out = _popcnt64(retain);
        __mmask64 store_mask = (1ULL << written_out) - 1;
        _mm512_mask_storeu_epi8((__m512i *)latin_output, store_mask, output);
        latin_output += written_out;
    }
    return (size_t)(latin_output - start);
}

size_t utf8_to_latin1_avx512(const char *buf, size_t len, char *latin_output) {
    char *start = latin_output;
    size_t pos = 0;
    __m512i minus64 = _mm512_set1_epi8(-64);
    __m512i one = _mm512_set1_epi8(1);
    __mmask64 next_leading = 0;
    __mmask64 next_bit6 = 0;

    while (pos + 64 <= len) {
        process_chunk<false>(buf + pos, 64, latin_output, minus64, one, next_leading, next_bit6);
        pos += 64;
    }

    if (pos < len) {
        size_t remaining = len - pos;
        process_chunk<true>(buf + pos, remaining, latin_output, minus64, one, next_leading, next_bit6);
    }

    return (size_t)(latin_output - start);
} */


/* 
convert_utf8_to_latin1+icelake, input size: 440052, iterations: 3000, dataset: /home/leorio/simdutf/unicode_lipsum/wikipedia_mars/french.utflatin8.txt
   0.478 ins/byte,    0.175 cycle/byte,   17.720 GB/s (3.7 %),     3.100 GHz,    2.735 ins/cycle 
   0.487 ins/char,    0.178 cycle/char,   17.408 Gc/s (3.7 %)     1.02 byte/char   */

simdutf_really_inline size_t process_block(const char *buf, size_t len, char *latin_output,
                     __m512i minus64, __m512i one,
                     __mmask64 *next_leading_ptr, __mmask64 *next_bit6_ptr,__mmask64 load_mask) {
    __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)buf);
    __mmask64 nonascii = _mm512_movepi8_mask(input);

    if (nonascii == 0) {
        _mm512_mask_storeu_epi8((__m512i *)latin_output, load_mask, input);
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
    __mmask64 load_mask = ~0ULL;

    while (pos + 64 <= len) {
        size_t written = process_block(buf + pos, 64, latin_output, minus64, one, &next_leading, &next_bit6 , load_mask);
        if (written == 0) {
            return 0; // Indicates error
        }
        latin_output += written;
        pos += 64;
    }

    load_mask = _bzhi_u64(~0ULL, len);
    if (pos < len) {
        size_t remaining = len - pos;
        size_t written = process_block(buf + pos, remaining, latin_output, minus64, one, &next_leading, &next_bit6 , load_mask);
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