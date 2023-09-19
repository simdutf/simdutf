inline void _sse_convert_latin1_to_utf8(
  const char*& latin_input,
  char*& utf8_output,
  const __m128i v_latin,
  const __m128i latin_1_half_into_u16_byte_mask,
  const __m128i latin_2_half_into_u16_byte_mask,
  const __m128i v_80,
  const __m128i v_0000,
  const __m128i v_ff80
) {
  if (_mm_testz_si128(v_latin, v_80)) {// ASCII fast path!!!!
    _mm_storeu_si128((__m128i*)utf8_output, v_latin);
    latin_input += 16;
    utf8_output += 16;
    return;
  }


  // assuming a/b are bytes and A/B are uint16 of the same value
  // aaaa_aaaa_bbbb_bbbb -> AAAA_AAAA
  const __m128i v_u16_latin_1_half = _mm_shuffle_epi8(v_latin, latin_1_half_into_u16_byte_mask);
  // aaaa_aaaa_bbbb_bbbb -> BBBB_BBBB
  const __m128i v_u16_latin_2_half = _mm_shuffle_epi8(v_latin, latin_2_half_into_u16_byte_mask);


  internal::westmere::write_v_u16_11bits_to_utf8(v_u16_latin_1_half, utf8_output, v_0000, v_ff80);
  internal::westmere::write_v_u16_11bits_to_utf8(v_u16_latin_2_half, utf8_output, v_0000, v_ff80);
  latin_input += 16;
};


std::pair<const char* const, char* const> sse_convert_latin1_to_utf8(
  const char* latin_input,
  const size_t latin_input_length,
  char* utf8_output) {
  const char* end = latin_input + latin_input_length;

  const __m128i v_0000 = _mm_setzero_si128();
  // 0b1000_0000
  const __m128i v_80 = _mm_set1_epi8((uint8_t)0x80);
  // 0b1111_1111_1000_0000
  const __m128i v_ff80 = _mm_set1_epi16((uint16_t)0xff80);

  const __m128i latin_1_half_into_u16_byte_mask = _mm_setr_epi8(
    0, '\x80',
    1, '\x80',
    2, '\x80',
    3, '\x80',
    4, '\x80',
    5, '\x80',
    6, '\x80',
    7, '\x80'
  );

  const __m128i latin_2_half_into_u16_byte_mask = _mm_setr_epi8(
    8, '\x80',
    9, '\x80',
    10, '\x80',
    11, '\x80',
    12, '\x80',
    13, '\x80',
    14, '\x80',
    15, '\x80'
  );

  // each latin1 takes 1-2 utf8 bytes
  // slow path writes useful 8-15 bytes twice (eagerly writes 16 bytes and then adjust the pointer)
  // so the last write can exceed the utf8_output size by 8-1 bytes 
  // by reserving 8 extra input bytes, we expect the output to have 8-16 bytes free

  // loop unroll depth 4
  while (latin_input + 16 * 4 + 8 <= end) {
    // Load 16 Latin1 characters (16 bytes) into a 128-bit register
    const __m128i v_latin_1 = _mm_loadu_si128((__m128i*)latin_input);
    const __m128i v_latin_2 = _mm_loadu_si128((__m128i*)latin_input + 1);
    const __m128i v_latin_3 = _mm_loadu_si128((__m128i*)latin_input + 2);
    const __m128i v_latin_4 = _mm_loadu_si128((__m128i*)latin_input + 3);

    _sse_convert_latin1_to_utf8(latin_input, utf8_output, v_latin_1, latin_1_half_into_u16_byte_mask, latin_2_half_into_u16_byte_mask, v_80, v_0000, v_ff80);
    _sse_convert_latin1_to_utf8(latin_input, utf8_output, v_latin_2, latin_1_half_into_u16_byte_mask, latin_2_half_into_u16_byte_mask, v_80, v_0000, v_ff80);
    _sse_convert_latin1_to_utf8(latin_input, utf8_output, v_latin_3, latin_1_half_into_u16_byte_mask, latin_2_half_into_u16_byte_mask, v_80, v_0000, v_ff80);
    _sse_convert_latin1_to_utf8(latin_input, utf8_output, v_latin_4, latin_1_half_into_u16_byte_mask, latin_2_half_into_u16_byte_mask, v_80, v_0000, v_ff80);
  }

  // loop unroll depth 2
  while (latin_input + 16 * 2 + 8 <= end) {
    // Load 16 Latin1 characters (16 bytes) into a 128-bit register
    const __m128i v_latin_1 = _mm_loadu_si128((__m128i*)latin_input);
    const __m128i v_latin_2 = _mm_loadu_si128((__m128i*)latin_input + 1);

    _sse_convert_latin1_to_utf8(latin_input, utf8_output, v_latin_1, latin_1_half_into_u16_byte_mask, latin_2_half_into_u16_byte_mask, v_80, v_0000, v_ff80);
    _sse_convert_latin1_to_utf8(latin_input, utf8_output, v_latin_2, latin_1_half_into_u16_byte_mask, latin_2_half_into_u16_byte_mask, v_80, v_0000, v_ff80);
  }


  // this loop always run just 1 iteration
  // nevertheless it shouldn't be replaced with if statement
  // it would significantly drop performance at least with MSVC 17.5.5
  while (latin_input + 16 + 8 <= end) {
    // Load 16 Latin1 characters (16 bytes) into a 128-bit register
    const __m128i v_latin = _mm_loadu_si128((__m128i*)latin_input);

    _sse_convert_latin1_to_utf8(latin_input, utf8_output, v_latin, latin_1_half_into_u16_byte_mask, latin_2_half_into_u16_byte_mask, v_80, v_0000, v_ff80);
  }

  return std::make_pair(latin_input, utf8_output);
};