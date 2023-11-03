// file included directly

static inline size_t latin1_to_utf8_avx512_vec(__m512i input, size_t input_len, char *utf8_output, int mask_output) {
  __mmask64 nonascii = _mm512_movepi8_mask(input);
  size_t output_size = input_len + (size_t)count_ones(nonascii);
  
  // Mask to denote whether the byte is a leading byte that is not ascii
  __mmask64 sixth =
      _mm512_cmpge_epu8_mask(input, _mm512_set1_epi8(-64)); //binary representation of -64: 1100 0000
  
  const uint64_t alternate_bits = UINT64_C(0x5555555555555555);
  uint64_t ascii = ~nonascii;
  // the bits in ascii are inverted and zeros are interspersed in between them
  uint64_t maskA = ~_pdep_u64(ascii, alternate_bits);
  uint64_t maskB = ~_pdep_u64(ascii>>32, alternate_bits);
  
  // interleave bytes from top and bottom halves (abcd...ABCD -> aAbBcCdD)
  __m512i input_interleaved = _mm512_permutexvar_epi8(_mm512_set_epi32(
    0x3f1f3e1e, 0x3d1d3c1c, 0x3b1b3a1a, 0x39193818,
    0x37173616, 0x35153414, 0x33133212, 0x31113010,
    0x2f0f2e0e, 0x2d0d2c0c, 0x2b0b2a0a, 0x29092808,
    0x27072606, 0x25052404, 0x23032202, 0x21012000
  ), input);
  
  // double size of each byte, and insert the leading byte 1100 0010

/* 
upscale the bytes to 16-bit value, adding the 0b11000000 leading byte in the process.
We adjust for the bytes that have their two most significant bits. This takes care of the first 32 bytes, assuming we interleaved the bytes. */
  __m512i outputA = _mm512_shldi_epi16(input_interleaved, _mm512_set1_epi8(-62), 8); 
  outputA = _mm512_mask_add_epi16(
                                  outputA, 
                                 (__mmask32)sixth, 
                                  outputA, 
                                  _mm512_set1_epi16(1 - 0x4000)); // 1- 0x4000 = 1100 0000 0000 0001????
  
  // in the second 32-bit half, set first or second option based on whether original input is leading byte (second case) or not (first case)
  __m512i leadingB = _mm512_mask_blend_epi16(
                                              (__mmask32)(sixth>>32), 
                                              _mm512_set1_epi16(0x00c2), // 0000 0000 1101 0010
                                              _mm512_set1_epi16(0x40c3));// 0100 0000 1100 0011
  __m512i outputB = _mm512_ternarylogic_epi32(
                                              input_interleaved, 
                                              leadingB, 
                                              _mm512_set1_epi16((short)0xff00), 
                                              (240 & 170) ^ 204); // (input_interleaved & 0xff00) ^ leadingB
  
  // prune redundant bytes
  outputA = _mm512_maskz_compress_epi8(maskA, outputA);
  outputB = _mm512_maskz_compress_epi8(maskB, outputB);
  
  
  size_t output_sizeA = (size_t)count_ones((uint32_t)nonascii) + 32;

  if(mask_output) {
    if(input_len > 32) { // is the second half of the input vector used?
      __mmask64 write_mask = _bzhi_u64(~0ULL, (unsigned int)output_sizeA);
      _mm512_mask_storeu_epi8(utf8_output, write_mask, outputA);
      utf8_output += output_sizeA;
      write_mask = _bzhi_u64(~0ULL, (unsigned int)(output_size - output_sizeA));
      _mm512_mask_storeu_epi8(utf8_output, write_mask, outputB);
    } else {
      __mmask64 write_mask = _bzhi_u64(~0ULL, (unsigned int)output_size);
      _mm512_mask_storeu_epi8(utf8_output, write_mask, outputA);
    }
  } else {
    _mm512_storeu_si512(utf8_output, outputA);
    utf8_output += output_sizeA;
    _mm512_storeu_si512(utf8_output, outputB);
  }
  return output_size;
}


// We take 32 zero-extended Latin1 characters and we write between 32 and 64 UTF-8 bytes.
// Returns the number of bytes written.
static simdutf_really_inline size_t latin1_to_utf8_avx512_vec2(__m512i in, char *utf8_output, __mmask32 is2byte) {
  const __m512i twobytes = _mm512_ternarylogic_epi32(
        _mm512_slli_epi16(in, 8), _mm512_srli_epi16(in, 6),
        _mm512_set1_epi16(0x3f3f), 0xa8); // (A|B)&C
  in = _mm512_mask_add_epi16(in, is2byte, twobytes,
                                _mm512_set1_epi16(int16_t(0x80c0)));
  const __m512i cmpmask = _mm512_set1_epi16(0x0800);
  // in >= cmpmask, always true for low bytes in 2-byte word.
  //              , always false for high bytes in ASCII 2-byte word.
  //              , always true for high bytes in UTF-8 2-byte word.
  const __mmask64 smoosh = _mm512_cmp_epu8_mask(in, cmpmask, _MM_CMPINT_NLT);
  const __m512i out = _mm512_maskz_compress_epi8(smoosh, in);
  _mm512_storeu_epi8(utf8_output, out);
  return 32 + _mm_popcnt_u32(_cvtmask32_u32(is2byte));
}

static simdutf_really_inline size_t latin1_to_utf8_avx512_branch( __m512i input, char *utf8_output) {
  __mmask64 nonascii = _mm512_movepi8_mask(input);
  if(nonascii) {
    __mmask32 nonascii0 = (__mmask32)nonascii;
    __mmask32 nonascii1 = (__mmask32)_kshiftri_mask64(nonascii,32);
    const __m256i h0 = _mm512_castsi512_si256(input);
    const __m256i h1 = _mm512_extracti64x4_epi64(input, 1);
    const __m512i input0 = _mm512_cvtepu8_epi16(h0);
    const __m512i input1 = _mm512_cvtepu8_epi16(h1);
    size_t written0 = latin1_to_utf8_avx512_vec2(input0, utf8_output, nonascii0);
    utf8_output += written0;
    size_t written1 = latin1_to_utf8_avx512_vec2(input1, utf8_output, nonascii1);
    return written0 + written1;
  } else {
    _mm512_storeu_si512(utf8_output, input);
    return 64;
  }
}
 
size_t latin1_to_utf8_avx512_start(const char *buf, size_t len, char *utf8_output) {
  char *start = utf8_output;
  size_t pos = 0;
  // if there's at least 128 bytes remaining, we don't need to mask the output
  for (; pos + 128 <= len; pos += 64) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_branch(input, utf8_output);
  }
  // in the last 128 bytes, the first 64 may require masking the output
  if (pos + 64 <= len) {
    __m512i input = _mm512_loadu_si512((__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, 64, utf8_output, 1);
    pos += 64;
  }
  // with the last 64 bytes, the input also needs to be masked
  if (pos < len) {
    __mmask64 load_mask = _bzhi_u64(~0ULL, (unsigned int)(len - pos));
    __m512i input = _mm512_maskz_loadu_epi8(load_mask, (__m512i *)(buf + pos));
    utf8_output += latin1_to_utf8_avx512_vec(input, len - pos, utf8_output, 1);
  }
  return (size_t)(utf8_output - start);
}
