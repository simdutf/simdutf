// file included directly

// File contains conversion procedure from VALID UTF-8 strings.


/*
    valid_utf8_to_fixed_length converts a valid UTF-8 string into UTF-32.

    The `OUTPUT` template type decides what to do with UTF-32: store
    it directly or convert into UTF-16 (with AVX512).

    Input:
    - str           - valid UTF-8 string
    - len           - string length
    - out_buffer    - output buffer

    Result:
    - pair.first    - the first unprocessed input byte
    - pair.second   - the first unprocessed output word
*/
template <typename OUTPUT>
std::pair<const char*, OUTPUT*> valid_utf8_to_fixed_length(const char* str, size_t len, OUTPUT* dwords) {
    constexpr bool UTF32 = std::is_same<OUTPUT, uint32_t>::value;
    constexpr bool UTF16 = std::is_same<OUTPUT, char16_t>::value;
    static_assert(UTF32 or UTF16, "output type has to be uint32_t (for UTF-32) or char16_t (for UTF-16)");

    const char* ptr = str;
    const char* end = ptr + len;

    OUTPUT* output = dwords;
    /**
     * In the main loop, we consume 64 bytes per iteration,
     * but we access 64 + 4 bytes.
     */
    while (ptr + 64 + 4 <= end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        const __m512i v_80 = _mm512_set1_epi8(char(0x80));
        const __mmask64 ascii = _mm512_test_epi8_mask(utf8, v_80);
        if(ascii == 0) {
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
            if (UTF32) {
                const __mmask16 valid = uint16_t((1 << valid_count0) - 1);
                _mm512_mask_storeu_epi32((__m512i*)output, valid, vec0);
                output += valid_count0;
            } else {
                output += utf32_to_utf16(vec0, valid_count0, reinterpret_cast<char16_t *>(output));
            } 
        } else {
            vec0 = expand_utf8_to_utf32(vec0);
            vec1 = expand_utf8_to_utf32(vec1);
            if (UTF32) {
                const __mmask16 valid0 = uint16_t((1 << valid_count0) - 1);
                _mm512_mask_storeu_epi32((__m512i*)output, valid0, vec0);
                output += valid_count0;
                const __mmask16 valid1 = uint16_t((1 << valid_count1) - 1);
                _mm512_mask_storeu_epi32((__m512i*)output, valid1, vec1);
                output += valid_count1;
            } else {
                output += utf32_to_utf16(vec0, valid_count0, reinterpret_cast<char16_t *>(output));
                output += utf32_to_utf16(vec1, valid_count1, reinterpret_cast<char16_t *>(output));
            } 
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
            if (UTF32) {
                const __mmask16 valid = uint16_t((1 << valid_count2) - 1);
                _mm512_mask_storeu_epi32((__m512i*)output, valid, vec2);
                output += valid_count2;
            } else {
                output += utf32_to_utf16(vec2, valid_count2, reinterpret_cast<char16_t *>(output));
            } 
        } else {
            vec2 = expand_utf8_to_utf32(vec2);
            vec3 = expand_utf8_to_utf32(vec3);
            if (UTF32) {
                const __mmask16 valid2 = uint16_t((1 << valid_count2) - 1);
                _mm512_mask_storeu_epi32((__m512i*)output, valid2, vec2);
                output += valid_count2;
                const __mmask16 valid3 = uint16_t((1 << valid_count3) - 1);
                _mm512_mask_storeu_epi32((__m512i*)output, valid3, vec3);
                output += valid_count3;
            } else {
                output += utf32_to_utf16(vec2, valid_count2, reinterpret_cast<char16_t *>(output));
                output += utf32_to_utf16(vec3, valid_count3, reinterpret_cast<char16_t *>(output));
            } 
        }
        ptr += 4*16;
    }

    // For the final pass, we validate 64 bytes, but we only transcode
    // 3*16 bytes, so we may end up double-validating 16 bytes.
    if (ptr + 64 <= end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        const __m512i v_80 = _mm512_set1_epi8(char(0x80));
        const __mmask64 ascii = _mm512_test_epi8_mask(utf8, v_80);
        if(ascii == 0) {
            SIMDUTF_ICELAKE_STORE_ASCII(UTF32, utf8, output)
            output += 64;
            ptr += 64;
        } else {
            const __m512i lane0 = broadcast_epi128<0>(utf8);
            const __m512i lane1 = broadcast_epi128<1>(utf8);

            SIMDUTF_ICELAKE_TRANSCODE16(lane0, lane1)

            const __m512i lane2 = broadcast_epi128<2>(utf8);
            SIMDUTF_ICELAKE_TRANSCODE16(lane1, lane2)

            const __m512i lane3 = broadcast_epi128<3>(utf8);
            SIMDUTF_ICELAKE_TRANSCODE16(lane2, lane3)

            ptr += 3*16;
        }
    }
    return {ptr, output};
}


using utf8_to_utf16_result = std::pair<const char*, char16_t*>;
