// Common procedures for both validating and non-validating conversions from UTF-8.

using utf8_to_utf16_result = std::pair<const char*, char16_t*>;

// See: http://0x80.pl/notesen/2021-12-22-test-and-clear-bit.html
bool test_and_clear_bit(uint32_t& val, int bitpos) {
    const uint32_t bitmask = uint32_t(1) << bitpos;
    const uint32_t old = val;

    val &= ~bitmask;

    return val < old;
}

/*
    utf32_to_utf16 converts `count` lower UTF-32 words
    from input `utf32` into UTF-16.

    Returns how many 16-bit words were stored.
*/
size_t utf32_to_utf16(__m512i utf32, unsigned int count, char16_t* output) {
    const __mmask16 valid = uint16_t((1 << count) - 1);
    // 1. check if we have any surrogate pairs
    const __mmask16 sp_mask = _mm512_mask_cmpgt_epu32_mask(valid, utf32, v_0000_ffff);
    if (sp_mask == 0) {
        // XXX: Masked vmovdqa is slow;
        //      Check: If we processed larger blocks, we can
        //      assume that the unmasked store won't overflow.
        _mm256_mask_storeu_epi16((__m256i*)output, valid, _mm512_cvtepi32_epi16(utf32));
        return count;
    }

    uint16_t words[32];

    {
        // 2. build surrogate pair words in 32-bit lanes

        //    t0 = 8 x [000000000000aaaa|aaaaaabbbbbbbbbb]
        const __m512i t0 = _mm512_sub_epi32(utf32, v_0001_0000);

        //    t1 = 8 x [000000aaaaaaaaaa|bbbbbbbbbb000000]
        const __m512i t1 = _mm512_slli_epi32(t0, 6);

        //    t2 = 8 x [000000aaaaaaaaaa|aaaaaabbbbbbbbbb] -- copy hi word from t1 to t0
        //         0xe4 = (t1 and v_ffff_0000) or (t0 and not v_ffff_0000)
        const __m512i t2 = _mm512_ternarylogic_epi32(t1, t0, v_ffff_0000, 0xe4);

        //    t2 = 8 x [110110aaaaaaaaaa|110111bbbbbbbbbb] -- copy hi word from t1 to t0
        //         0xba = (t2 and not v_fc00_fc000) or v_d800_dc00
        const __m512i t3 = _mm512_ternarylogic_epi32(t2, v_fc00_fc00, v_d800_dc00, 0xba);

        _mm512_storeu_si512((__m512i*)words, t3);
    }

    // 3. store valid 16-bit values
    _mm256_mask_storeu_epi16((__m256i*)output, valid, _mm512_cvtepi32_epi16(utf32));

    int sp = __builtin_popcount(sp_mask);

    // 4. copy surrogate pairs
    uint32_t mask = sp_mask;
    for (int i=count; i >= 0 && mask != 0; i--) {
        if (test_and_clear_bit(mask, i)) {
            output[i + sp] = words[2*i + 0];
            sp -= 1;
            output[i + sp] = words[2*i + 1];
        } else {
            output[i + sp] = output[i];
        }
    }

    return count + __builtin_popcount(sp_mask);
}

