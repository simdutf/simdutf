// file included directly

// File contains conversion procedure from possibly invalid UTF-8 strings.


template <typename OUTPUT>
std::pair<const char*, OUTPUT*> validating_utf8_to_fixed_length(const char* str, size_t len, OUTPUT* dwords) {
    constexpr bool UTF32 = std::is_same<OUTPUT, uint32_t>::value;
    constexpr bool UTF16 = std::is_same<OUTPUT, char16_t>::value;
    static_assert(UTF32 or UTF16, "output type has to be uint32_t (for UTF-32) or char16_t (for UTF-16)");

    const char* ptr = str;
    const char* end = ptr + len;

    OUTPUT* output = dwords;
    avx512_utf8_checker checker{};
    /**
     * In the main loop, we consume 64 bytes per iteration,
     * but we access 64 + 4 bytes.
     */
    while (ptr + 64 + 4 <= end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        if(checker.check_next_input(utf8)) {
            STORE_ASCII(UTF32, utf8, output)
            output += 64;
            ptr += 64;
            continue;
        }


        const __m512i lane0 = broadcast_epi128<0>(utf8);
        const __m512i lane1 = broadcast_epi128<1>(utf8);
        TRANSCODE16(lane0, lane1)

        const __m512i lane2 = broadcast_epi128<2>(utf8);
        TRANSCODE16(lane1, lane2)

        const __m512i lane3 = broadcast_epi128<3>(utf8);
        TRANSCODE16(lane2, lane3)

        uint32_t tmp1;
        memcpy(&tmp1, ptr + 64, sizeof(tmp1));
        const __m512i lane4 = _mm512_set1_epi32(tmp1);
        TRANSCODE16(lane3, lane4)
        ptr += 4*16;
    }
    const char* validatedptr = ptr; // validated up to ptr

    // For the final pass, we validate 64 bytes, but we only transcode
    // 3*16 bytes, so we may end up double-validating 16 bytes.
    if (ptr + 64 <= end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        if(checker.check_next_input(utf8)) {
            STORE_ASCII(UTF32, utf8, output)
            output += 64;
            ptr += 64;
        } else {
            const __m512i lane0 = broadcast_epi128<0>(utf8);
            const __m512i lane1 = broadcast_epi128<1>(utf8);
            TRANSCODE16(lane0, lane1)

            const __m512i lane2 = broadcast_epi128<2>(utf8);
            TRANSCODE16(lane1, lane2)

            const __m512i lane3 = broadcast_epi128<3>(utf8);
            TRANSCODE16(lane2, lane3)

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

