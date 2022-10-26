// file included directly

// File contains conversion procedure from possibly invalid UTF-8 strings.

/**
 * Attempts to convert up to len 1-byte words from in (in UTF-8 format) to
 * out.
 * Returns the position of the input and output after the processing is
 * completed. Upon error, the output is set to null.
 */
template <endianness big_endian>
utf8_to_utf16_result fast_avx512_convert_utf8_to_utf16(const char *in, size_t len, char16_t *out) {
  const char *const final_in = in + len;
  bool result = true;
  while (result) {
    if (in + 64 <= final_in) {
        result = process_block_utf8_to_utf16<SIMDUTF_FULL, big_endian>(in, out, final_in - in);
    } else if(in < final_in) {
        result = process_block_utf8_to_utf16<SIMDUTF_TAIL, big_endian>(in, out, final_in - in);
    } else { break; }
  }
  if(!result) { out = nullptr; }
  return std::make_pair(in, out);
}

template <endianness big_endian>
simdutf::result fast_avx512_convert_utf8_to_utf16_with_errors(const char *in, size_t len, char16_t *out) {
  const char *const init_in = in;
  const char16_t *const init_out = out;
  const char *const final_in = in + len;
  bool  result = true;
  while (result) {
    if (in + 64 <= final_in) {
        result = process_block_utf8_to_utf16<SIMDUTF_FULL, big_endian>(in, out, final_in - in);
    } else if(in < final_in) {
        result = process_block_utf8_to_utf16<SIMDUTF_TAIL, big_endian>(in, out, final_in - in);
    } else { break; }
  }
  if(!result) {
    simdutf::result res = scalar::utf8_to_utf16::rewind_and_convert_with_errors<big_endian>(in, final_in - in, out);
    res.count += (in - init_in);
    return res;
  } else {
    return simdutf::result(error_code::SUCCESS,out - init_out);
  }
}


template <endianness big_endian, typename OUTPUT>
std::pair<const char*, OUTPUT*> validating_utf8_to_fixed_length(const char* str, size_t len, OUTPUT* dwords) {
    constexpr bool UTF32 = std::is_same<OUTPUT, uint32_t>::value;
    constexpr bool UTF16 = std::is_same<OUTPUT, char16_t>::value;
    static_assert(UTF32 or UTF16, "output type has to be uint32_t (for UTF-32) or char16_t (for UTF-16)");
    static_assert(!(UTF32 and big_endian), "we do not currently support big-endian UTF-32");

    const char* ptr = str;
    const char* end = ptr + len;
    __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
    OUTPUT* output = dwords;
    avx512_utf8_checker checker{};
    /**
     * In the main loop, we consume 64 bytes per iteration,
     * but we access 64 + 4 bytes.
     * We check for ptr + 64 + 64 <= end because
     * we want to be do maskless writes without overruns.
     */
    while (ptr + 64 + 64 <= end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        if(checker.check_next_input(utf8)) {
            SIMDUTF_ICELAKE_STORE_ASCII(UTF32, utf8, output)
            output += 64;
            ptr += 64;
            continue;
        }
        const __m512i lane0 = broadcast_epi128<0>(utf8);
        const __m512i lane1 = broadcast_epi128<1>(utf8);
        int valid_count0;
        __m512i vec0 = expand_and_identify(lane0, lane1, valid_count0);
        const __m512i lane2 = broadcast_epi128<2>(utf8);
        int valid_count1;
        __m512i vec1 = expand_and_identify(lane1, lane2, valid_count1);
        if(valid_count0 + valid_count1 <= 16) {
            vec0 = _mm512_mask_expand_epi32(vec0, __mmask16(((1<<valid_count1)-1)<<valid_count0), vec1);
            valid_count0 += valid_count1;
            vec0 = expand_utf8_to_utf32(vec0);
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec0, valid_count0, false)
        } else {
            vec0 = expand_utf8_to_utf32(vec0);
            vec1 = expand_utf8_to_utf32(vec1);
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec0, valid_count0, false)
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec1, valid_count1, false)
        }
        const __m512i lane3 = broadcast_epi128<3>(utf8);
        int valid_count2;
        __m512i vec2 = expand_and_identify(lane2, lane3, valid_count2);
        uint32_t tmp1;
        ::memcpy(&tmp1, ptr + 64, sizeof(tmp1));
        const __m512i lane4 = _mm512_set1_epi32(tmp1);
        int valid_count3;
        __m512i vec3 = expand_and_identify(lane3, lane4, valid_count3);
        if(valid_count2 + valid_count3 <= 16) {
            vec2 = _mm512_mask_expand_epi32(vec2, __mmask16(((1<<valid_count3)-1)<<valid_count2), vec3);
            valid_count2 += valid_count3;
            vec2 = expand_utf8_to_utf32(vec2);
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec2, valid_count2, false)
        } else {
            vec2 = expand_utf8_to_utf32(vec2);
            vec3 = expand_utf8_to_utf32(vec3);
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec2, valid_count2, false)
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec3, valid_count3, false)
        }
        ptr += 4*16;
    }
    const char* validatedptr = ptr; // validated up to ptr

    // For the final pass, we validate 64 bytes, but we only transcode
    // 3*16 bytes, so we may end up double-validating 16 bytes.
    if (ptr + 64 <= end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        if(checker.check_next_input(utf8)) {
            SIMDUTF_ICELAKE_STORE_ASCII(UTF32, utf8, output)
            output += 64;
            ptr += 64;
        } else {
            const __m512i lane0 = broadcast_epi128<0>(utf8);
            const __m512i lane1 = broadcast_epi128<1>(utf8);
            int valid_count0;
            __m512i vec0 = expand_and_identify(lane0, lane1, valid_count0);
            const __m512i lane2 = broadcast_epi128<2>(utf8);
            int valid_count1;
            __m512i vec1 = expand_and_identify(lane1, lane2, valid_count1);
            if(valid_count0 + valid_count1 <= 16) {
                vec0 = _mm512_mask_expand_epi32(vec0, __mmask16(((1<<valid_count1)-1)<<valid_count0), vec1);
                valid_count0 += valid_count1;
                vec0 = expand_utf8_to_utf32(vec0);
                SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec0, valid_count0, true)
            } else {
                vec0 = expand_utf8_to_utf32(vec0);
                vec1 = expand_utf8_to_utf32(vec1);
                SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec0, valid_count0, true)
                SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec1, valid_count1, true)
            }

            const __m512i lane3 = broadcast_epi128<3>(utf8);
            SIMDUTF_ICELAKE_TRANSCODE16(lane2, lane3, true)

            ptr += 3*16;
        }
        validatedptr += 4*16;
    }
    {
       const __m512i utf8 = _mm512_maskz_loadu_epi8((1ULL<<(end - validatedptr))-1, (const __m512i*)validatedptr);
       checker.check_next_input(utf8);
    }
    checker.check_eof();
    if(checker.errors()) {
        return {ptr, nullptr}; // We found an error.
    }
    return {ptr, output};
}

// Like validating_utf8_to_fixed_length but returns as soon as an error is identified
template <endianness big_endian, typename OUTPUT>
std::tuple<const char*, OUTPUT*, bool> validating_utf8_to_fixed_length_with_constant_checks(const char* str, size_t len, OUTPUT* dwords) {
    constexpr bool UTF32 = std::is_same<OUTPUT, uint32_t>::value;
    constexpr bool UTF16 = std::is_same<OUTPUT, char16_t>::value;
    static_assert(UTF32 or UTF16, "output type has to be uint32_t (for UTF-32) or char16_t (for UTF-16)");
    static_assert(!(UTF32 and big_endian), "we do not currently support big-endian UTF-32");

    const char* ptr = str;
    const char* end = ptr + len;
    __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
    OUTPUT* output = dwords;
    avx512_utf8_checker checker{};
    /**
     * In the main loop, we consume 64 bytes per iteration,
     * but we access 64 + 4 bytes.
     * We check for ptr + 64 + 64 <= end because
     * we want to be do maskless writes without overruns.
     */
    while (ptr + 64 + 64 <= end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        if(checker.check_next_input(utf8)) {
            SIMDUTF_ICELAKE_STORE_ASCII(UTF32, utf8, output)
            output += 64;
            ptr += 64;
            continue;
        }
        if(checker.errors()) {
            return {ptr, output, false}; // We found an error.
        }
        const __m512i lane0 = broadcast_epi128<0>(utf8);
        const __m512i lane1 = broadcast_epi128<1>(utf8);
        int valid_count0;
        __m512i vec0 = expand_and_identify(lane0, lane1, valid_count0);
        const __m512i lane2 = broadcast_epi128<2>(utf8);
        int valid_count1;
        __m512i vec1 = expand_and_identify(lane1, lane2, valid_count1);
        if(valid_count0 + valid_count1 <= 16) {
            vec0 = _mm512_mask_expand_epi32(vec0, __mmask16(((1<<valid_count1)-1)<<valid_count0), vec1);
            valid_count0 += valid_count1;
            vec0 = expand_utf8_to_utf32(vec0);
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec0, valid_count0, false)
        } else {
            vec0 = expand_utf8_to_utf32(vec0);
            vec1 = expand_utf8_to_utf32(vec1);
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec0, valid_count0, false)
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec1, valid_count1, false)
        }
        const __m512i lane3 = broadcast_epi128<3>(utf8);
        int valid_count2;
        __m512i vec2 = expand_and_identify(lane2, lane3, valid_count2);
        uint32_t tmp1;
        ::memcpy(&tmp1, ptr + 64, sizeof(tmp1));
        const __m512i lane4 = _mm512_set1_epi32(tmp1);
        int valid_count3;
        __m512i vec3 = expand_and_identify(lane3, lane4, valid_count3);
        if(valid_count2 + valid_count3 <= 16) {
            vec2 = _mm512_mask_expand_epi32(vec2, __mmask16(((1<<valid_count3)-1)<<valid_count2), vec3);
            valid_count2 += valid_count3;
            vec2 = expand_utf8_to_utf32(vec2);
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec2, valid_count2, false)
        } else {
            vec2 = expand_utf8_to_utf32(vec2);
            vec3 = expand_utf8_to_utf32(vec3);
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec2, valid_count2, false)
            SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec3, valid_count3, false)
        }
        ptr += 4*16;
    }
    const char* validatedptr = ptr; // validated up to ptr

    // For the final pass, we validate 64 bytes, but we only transcode
    // 3*16 bytes, so we may end up double-validating 16 bytes.
    if (ptr + 64 <= end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        if(checker.check_next_input(utf8)) {
            SIMDUTF_ICELAKE_STORE_ASCII(UTF32, utf8, output)
            output += 64;
            ptr += 64;
        } else if(checker.errors()) {
            return {ptr, output, false}; // We found an error.
        } else {
            const __m512i lane0 = broadcast_epi128<0>(utf8);
            const __m512i lane1 = broadcast_epi128<1>(utf8);
            int valid_count0;
            __m512i vec0 = expand_and_identify(lane0, lane1, valid_count0);
            const __m512i lane2 = broadcast_epi128<2>(utf8);
            int valid_count1;
            __m512i vec1 = expand_and_identify(lane1, lane2, valid_count1);
            if(valid_count0 + valid_count1 <= 16) {
                vec0 = _mm512_mask_expand_epi32(vec0, __mmask16(((1<<valid_count1)-1)<<valid_count0), vec1);
                valid_count0 += valid_count1;
                vec0 = expand_utf8_to_utf32(vec0);
                SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec0, valid_count0, true)
            } else {
                vec0 = expand_utf8_to_utf32(vec0);
                vec1 = expand_utf8_to_utf32(vec1);
                SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec0, valid_count0, true)
                SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(vec1, valid_count1, true)
            }

            const __m512i lane3 = broadcast_epi128<3>(utf8);
            SIMDUTF_ICELAKE_TRANSCODE16(lane2, lane3, true)

            ptr += 3*16;
        }
        validatedptr += 4*16;
    }
    {
       const __m512i utf8 = _mm512_maskz_loadu_epi8((1ULL<<(end - validatedptr))-1, (const __m512i*)validatedptr);
       checker.check_next_input(utf8);
    }
    checker.check_eof();
    if(checker.errors()) {
        return {ptr, output, false}; // We found an error.
    }
    return {ptr, output, true};
}
