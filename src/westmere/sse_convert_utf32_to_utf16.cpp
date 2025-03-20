struct expansion_result_t {
  size_t u16count;
  __m128i compressed;
};

// Function sse_expand_surrogate takes four **valid** UTF-32 characters
// having at least one code-point producing a surrogate pair.
template <endianness byte_order>
expansion_result_t sse_expand_surrogate(const __m128i x) {
  using vector_u32 = simd32<uint32_t>;
  using vector_u8 = simd8<uint8_t>;

  const auto in = vector_u32(x);

  const auto non_surrogate_mask = (in & uint32_t(0xffff0000)) == uint32_t(0);
  const auto mask = (~non_surrogate_mask.to_4bit_bitmask()) & 0xf;

  const auto t0 = in - uint32_t(0x00010000);
  const auto hi = t0.shr<10>() & uint32_t(0x000003ff);
  const auto lo = t0.shl<16>() & uint32_t(0x03ff0000);
  const auto surrogates = (lo | hi) | uint32_t(0xdc00d800);

  const auto merged = as_vector_u8(select(non_surrogate_mask, in, surrogates));

  const auto shuffle = vector_u8::load(
      (byte_order == endianness::LITTLE)
          ? tables::utf32_to_utf16::pack_utf32_to_utf16le[mask]
          : tables::utf32_to_utf16::pack_utf32_to_utf16be[mask]);

  const size_t u16count = (4 + count_ones(mask));
  const auto compressed = shuffle.lookup_16(merged);

  return {u16count, compressed};
}

// Function `validate_utf32` checks 2 x 4 UTF-32 characters for their validity.
simdutf_really_inline bool validate_utf32(const __m128i a, const __m128i b) {
  using vector_u32 = simd32<uint32_t>;

  const auto in0 = vector_u32(a);
  const auto in1 = vector_u32(b);

  const auto standardmax = vector_u32::splat(0x10ffff);
  const auto offset = vector_u32::splat(0xffff2000);
  const auto standardoffsetmax = vector_u32::splat(0xfffff7ff);

  const auto too_large = max(in0, in1) > standardmax;
  const auto surrogate0 = (in0 + offset) > standardoffsetmax;
  const auto surrogate1 = (in1 + offset) > standardoffsetmax;

  const auto combined = too_large | surrogate0 | surrogate1;
  return !combined.any();
}

template <endianness big_endian>
std::pair<const char32_t *, char16_t *>
sse_convert_utf32_to_utf16(const char32_t *buf, size_t len,
                           char16_t *utf16_output) {

  const char32_t *end = buf + len;

  const __m128i v_ffff0000 = _mm_set1_epi32((int32_t)0xffff0000);
  __m128i forbidden_bytemask = _mm_setzero_si128();

  while (end - buf >= 16 + 8) {
    const __m128i *ptr = reinterpret_cast<const __m128i *>(buf);
    const __m128i in0 = _mm_loadu_si128(ptr + 0);
    const __m128i in1 = _mm_loadu_si128(ptr + 1);
    const __m128i in2 = _mm_loadu_si128(ptr + 2);
    const __m128i in3 = _mm_loadu_si128(ptr + 3);

    const __m128i combined =
        _mm_or_si128(_mm_or_si128(in2, in3), _mm_or_si128(in0, in1));
    if (simdutf_likely(_mm_testz_si128(combined, v_ffff0000))) {
      // No bits set above 16th, directly pack UTF-32 to UTF-16
      __m128i utf16_packed0 = _mm_packus_epi32(in0, in1);
      __m128i utf16_packed1 = _mm_packus_epi32(in2, in3);

      const __m128i v_f800 = _mm_set1_epi16((uint16_t)0xf800);
      const __m128i v_d800 = _mm_set1_epi16((uint16_t)0xd800);
      forbidden_bytemask = _mm_or_si128(
          forbidden_bytemask,
          _mm_or_si128(
              _mm_cmpeq_epi16(_mm_and_si128(utf16_packed0, v_f800), v_d800),
              _mm_cmpeq_epi16(_mm_and_si128(utf16_packed1, v_f800), v_d800)));

      if (big_endian) {
        const __m128i swap =
            _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
        utf16_packed0 = _mm_shuffle_epi8(utf16_packed0, swap);
        utf16_packed1 = _mm_shuffle_epi8(utf16_packed1, swap);
      }

      _mm_storeu_si128((__m128i *)utf16_output + 0, utf16_packed0);
      _mm_storeu_si128((__m128i *)utf16_output + 1, utf16_packed1);
      utf16_output += 16;
      buf += 16;
    } else {
      if (!validate_utf32(in0, in1) || !validate_utf32(in2, in3)) {
        return std::make_pair(nullptr, utf16_output);
      }

      const auto ret0 = sse_expand_surrogate<big_endian>(in0);
      _mm_storeu_si128((__m128i *)utf16_output, ret0.compressed);
      utf16_output += ret0.u16count;

      const auto ret1 = sse_expand_surrogate<big_endian>(in1);
      _mm_storeu_si128((__m128i *)utf16_output, ret1.compressed);
      utf16_output += ret1.u16count;

      const auto ret2 = sse_expand_surrogate<big_endian>(in2);
      _mm_storeu_si128((__m128i *)utf16_output, ret2.compressed);
      utf16_output += ret2.u16count;

      const auto ret3 = sse_expand_surrogate<big_endian>(in3);
      _mm_storeu_si128((__m128i *)utf16_output, ret3.compressed);
      utf16_output += ret3.u16count;

      buf += 16;
    }
  }

  // check for invalid input
  if (static_cast<uint32_t>(_mm_movemask_epi8(forbidden_bytemask)) != 0) {
    return std::make_pair(nullptr, utf16_output);
  }

  return std::make_pair(buf, utf16_output);
}

template <endianness big_endian>
std::pair<result, char16_t *>
sse_convert_utf32_to_utf16_with_errors(const char32_t *buf, size_t len,
                                       char16_t *utf16_output) {
  const char32_t *start = buf;
  const char32_t *end = buf + len;

  const __m128i v_ffff0000 = _mm_set1_epi32((int32_t)0xffff0000);

  while (end - buf >= 8) {
    const __m128i in = _mm_loadu_si128((__m128i *)buf);
    const __m128i nextin = _mm_loadu_si128((__m128i *)buf + 1);

    const __m128i combined = _mm_or_si128(in, nextin);
    if (simdutf_likely(_mm_testz_si128(combined, v_ffff0000))) {
      // No bits set above 16th, directly pack UTF-32 to UTF-16
      __m128i utf16_packed = _mm_packus_epi32(in, nextin);

      const __m128i v_f800 = _mm_set1_epi16((uint16_t)0xf800);
      const __m128i v_d800 = _mm_set1_epi16((uint16_t)0xd800);
      const __m128i forbidden_bytemask =
          _mm_cmpeq_epi16(_mm_and_si128(utf16_packed, v_f800), v_d800);
      if (static_cast<uint32_t>(_mm_movemask_epi8(forbidden_bytemask)) != 0) {
        return std::make_pair(result(error_code::SURROGATE, buf - start),
                              utf16_output);
      }

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
