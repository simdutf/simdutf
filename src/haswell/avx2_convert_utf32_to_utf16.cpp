struct expansion_result_t {
  size_t u16count_lo;
  __m128i compressed_lo;

  size_t u16count_hi;
  __m128i compressed_hi;
};

// Function avx2_expand_surrogate takes eight **valid** UTF-32 characters
// having at least one code-point producing a surrogate pair.
template <endianness byte_order>
expansion_result_t avx2_expand_surrogate(const __m256i x) {
  using vector_u32 = simd32<uint32_t>;

  const auto in = vector_u32(x);

  const auto non_surrogate_mask = (in & uint32_t(0xffff0000)) == uint32_t(0);
  const uint8_t mask_2x = uint8_t(~non_surrogate_mask.to_8bit_bitmask());
  const uint8_t mask_lo = uint8_t(mask_2x & 0xf);
  const uint8_t mask_hi = uint8_t(mask_2x >> 4);

  const auto t0 = in - uint32_t(0x00010000);
  const auto hi = t0.shr<10>() & uint32_t(0x000003ff);
  const auto lo = t0.shl<16>() & uint32_t(0x03ff0000);
  const auto surrogates = (lo | hi) | uint32_t(0xdc00d800);

  const auto merged = as_vector_u8(select(non_surrogate_mask, in, surrogates));

  // Note: we're getting back to the intrinsics world, as there's
  //       no way to refer to the westmere implementation (we enclose
  //       implementations in anonymous namespaces).
  const auto lo_half = _mm256_extractf128_si256(merged.value, 0);
  const auto hi_half = _mm256_extractf128_si256(merged.value, 1);

  const auto shuffle_lo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(
      (byte_order == endianness::LITTLE)
          ? tables::utf32_to_utf16::pack_utf32_to_utf16le[mask_lo]
          : tables::utf32_to_utf16::pack_utf32_to_utf16be[mask_lo]));

  const auto shuffle_hi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(
      (byte_order == endianness::LITTLE)
          ? tables::utf32_to_utf16::pack_utf32_to_utf16le[mask_hi]
          : tables::utf32_to_utf16::pack_utf32_to_utf16be[mask_hi]));

  const size_t u16count_lo = (4 + count_ones(mask_lo));
  const size_t u16count_hi = (4 + count_ones(mask_hi));

  const auto compressed_lo = _mm_shuffle_epi8(lo_half, shuffle_lo);
  const auto compressed_hi = _mm_shuffle_epi8(hi_half, shuffle_hi);

  return {u16count_lo, compressed_lo, u16count_hi, compressed_hi};
}

// Function `invalid_utf32` returns a bytemask pointing invalid UTF-32
// characters.
simdutf_really_inline simd32<bool> invalid_utf32(const __m256i x) {
  using vector_u32 = simd32<uint32_t>;

  const auto in = vector_u32(x);

  const auto standardmax = vector_u32::splat(0x10ffff);

  const auto too_large = in > standardmax;
  const auto surrogate = (in & uint32_t(0xfffff800)) == uint32_t(0xd800);

  return too_large | surrogate;
}

template <endianness big_endian>
std::pair<const char32_t *, char16_t *>
avx2_convert_utf32_to_utf16(const char32_t *buf, size_t len,
                            char16_t *utf16_output) {
  const char32_t *end = buf + len;

  // to avoid overruns, see issue https://github.com/simdutf/simdutf/issues/92
  const size_t safety_margin = 12;

  __m256i forbidden_bytemask = _mm256_setzero_si256();

  const __m256i v_ffff0000 = _mm256_set1_epi32((int32_t)0xffff0000);
  const __m256i v_f800 = _mm256_set1_epi32((uint32_t)0xf800);
  const __m256i v_d800 = _mm256_set1_epi32((uint32_t)0xd800);

  while (end - buf >= std::ptrdiff_t(8 + safety_margin)) {
    bool check = false;
    __m256i in;
    while (end - buf >= std::ptrdiff_t(8 + safety_margin)) {
      in = _mm256_loadu_si256((const __m256i *)buf);

      if (simdutf_likely(_mm256_testz_si256(in, v_ffff0000))) {
        // no bits set above 16th bit <=> can pack to UTF16
        // without surrogate pairs
        forbidden_bytemask = _mm256_or_si256(
            forbidden_bytemask,
            _mm256_cmpeq_epi32(_mm256_and_si256(in, v_f800), v_d800));

        __m128i utf16_packed = _mm_packus_epi32(
            _mm256_castsi256_si128(in), _mm256_extractf128_si256(in, 1));
        if (big_endian) {
          const __m128i swap = _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11,
                                             10, 13, 12, 15, 14);
          utf16_packed = _mm_shuffle_epi8(utf16_packed, swap);
        }
        _mm_storeu_si128((__m128i *)utf16_output, utf16_packed);
        utf16_output += 8;
        buf += 8;
      } else {
        check = true;
        break;
      }
    } // inner while

    if (check) {
      const auto err = invalid_utf32(in);
      if (simdutf_unlikely(err.any())) {
        return std::make_pair(nullptr, utf16_output);
      }

      const auto ret = avx2_expand_surrogate<big_endian>(in);
      _mm_storeu_si128((__m128i *)utf16_output, ret.compressed_lo);
      utf16_output += ret.u16count_lo;
      _mm_storeu_si128((__m128i *)utf16_output, ret.compressed_hi);
      utf16_output += ret.u16count_hi;

      buf += 8;
    }
  } // outer while

  // check for invalid input
  if (static_cast<uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0) {
    return std::make_pair(nullptr, utf16_output);
  }

  return std::make_pair(buf, utf16_output);
}

template <endianness big_endian>
std::pair<result, char16_t *>
avx2_convert_utf32_to_utf16_with_errors(const char32_t *buf, size_t len,
                                        char16_t *utf16_output) {
  const char32_t *start = buf;
  const char32_t *end = buf + len;

  const size_t safety_margin =
      12; // to avoid overruns, see issue
          // https://github.com/simdutf/simdutf/issues/92

  const __m256i v_ffff0000 = _mm256_set1_epi32((int32_t)0xffff0000);
  const __m256i v_f800 = _mm256_set1_epi32((uint32_t)0xf800);
  const __m256i v_d800 = _mm256_set1_epi32((uint32_t)0xd800);

  while (end - buf >= std::ptrdiff_t(8 + safety_margin)) {
    const __m256i in = _mm256_loadu_si256((__m256i *)buf);

    if (simdutf_likely(_mm256_testz_si256(in, v_ffff0000))) {
      // no bits set above 16th bit <=> can pack to UTF16 without surrogate
      // pairs
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
      const auto err = invalid_utf32(in);
      if (simdutf_unlikely(err.any())) {
        const size_t pos = err.first_set();
        // write chars prior the error
        for (size_t i = 0; i < pos; i++) {
          const uint32_t word = buf[i];
          if ((word & 0xFFFF0000) == 0) {
            *utf16_output++ =
                scalar::u16_force_endianness<endianness::LITTLE, big_endian>(
                    uint16_t(word));
          } else {
            const uint32_t w = word - 0x10000;
            const uint16_t high_surrogate = uint16_t(0xD800 + (w >> 10));
            const uint16_t low_surrogate = uint16_t(0xDC00 + (w & 0x3FF));
            *utf16_output++ =
                scalar::u16_force_endianness<endianness::LITTLE, big_endian>(
                    high_surrogate);
            *utf16_output++ =
                scalar::u16_force_endianness<endianness::LITTLE, big_endian>(
                    low_surrogate);
          }
        }

        // determine the exact error
        const uint32_t word = buf[pos];
        const size_t error_pos = buf - start + pos;
        if (word > 0x10FFFF) {
          return {result(error_code::TOO_LARGE, error_pos), utf16_output};
        }
        if (word >= 0xD800 && word <= 0xDFFF) {
          return {result(error_code::SURROGATE, error_pos), utf16_output};
        }
        return {result(error_code::OTHER, error_pos), utf16_output};
      }

      const auto ret = avx2_expand_surrogate<big_endian>(in);
      _mm_storeu_si128((__m128i *)utf16_output, ret.compressed_lo);
      utf16_output += ret.u16count_lo;
      _mm_storeu_si128((__m128i *)utf16_output, ret.compressed_hi);
      utf16_output += ret.u16count_hi;

      buf += 8;
    }
  }

  return {result(error_code::SUCCESS, buf - start), utf16_output};
}
