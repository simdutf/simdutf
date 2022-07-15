// Common procedures for both validating and non-validating conversions from UTF-8.

using utf8_to_utf16_result = std::pair<const char*, char16_t*>;
using utf8_to_utf32_result = std::pair<const char*, uint32_t*>;



/*
    utf32_to_utf16_masked converts `count` lower UTF-32 words
    from input `utf32` into UTF-16. It differs from utf32_to_utf16
    in that it 'masks' the writes.

    Returns how many 16-bit words were stored.
*/
simdutf_really_inline size_t utf32_to_utf16_masked(__m512i utf32, unsigned int count, char16_t* output) {
    const __mmask16 valid = uint16_t((1 << count) - 1);
    // 1. check if we have any surrogate pairs
    const __m512i v_0000_ffff = _mm512_set1_epi32(0x0000ffff);
    const __mmask16 sp_mask = _mm512_mask_cmpgt_epu32_mask(valid, utf32, v_0000_ffff);

    if (sp_mask == 0) {
        _mm256_mask_storeu_epi16((__m256i*)output, valid, _mm512_cvtepi32_epi16(utf32));
        return count;
    }

    {
        // build surrogate pair words in 32-bit lanes

        //    t0 = 8 x [000000000000aaaa|aaaaaabbbbbbbbbb]
        const __m512i v_0001_0000 = _mm512_set1_epi32(0x00010000);
        const __m512i t0 = _mm512_sub_epi32(utf32, v_0001_0000);

        //    t1 = 8 x [000000aaaaaaaaaa|bbbbbbbbbb000000]
        const __m512i t1 = _mm512_slli_epi32(t0, 6);

        //    t2 = 8 x [000000aaaaaaaaaa|aaaaaabbbbbbbbbb] -- copy hi word from t1 to t0
        //         0xe4 = (t1 and v_ffff_0000) or (t0 and not v_ffff_0000)
        const __m512i v_ffff_0000 = _mm512_set1_epi32(0xffff0000);
        const __m512i t2 = _mm512_ternarylogic_epi32(t1, t0, v_ffff_0000, 0xe4);

        //    t2 = 8 x [110110aaaaaaaaaa|110111bbbbbbbbbb] -- copy hi word from t1 to t0
        //         0xba = (t2 and not v_fc00_fc000) or v_d800_dc00
        const __m512i v_fc00_fc00 = _mm512_set1_epi32(0xfc00fc00);
        const __m512i v_d800_dc00 = _mm512_set1_epi32(0xd800dc00);
        const __m512i t3 = _mm512_ternarylogic_epi32(t2, v_fc00_fc00, v_d800_dc00, 0xba);
        const __m512i t4 = _mm512_mask_blend_epi32(sp_mask, utf32, t3);
        const __m512i t5 = _mm512_ror_epi32(t4, 16);
        const  __mmask32 nonzero = _mm512_cmpneq_epi16_mask(t5, _mm512_setzero_si512());
         _mm512_mask_compressstoreu_epi16(output, nonzero, t5);
    }

    return count + static_cast<unsigned int>(count_ones(sp_mask));
}




/*
    utf32_to_utf16 converts `count` lower UTF-32 words
    from input `utf32` into UTF-16. It may overflow.

    Returns how many 16-bit words were stored.
*/
simdutf_really_inline size_t utf32_to_utf16(__m512i utf32, unsigned int count, char16_t* output) {
    // 1. check if we have any surrogate pairs
    const __m512i v_0000_ffff = _mm512_set1_epi32(0x0000ffff);
    const __mmask16 sp_mask = _mm512_cmpgt_epu32_mask(utf32, v_0000_ffff);

    if (sp_mask == 0) {
        _mm256_storeu_epi16((__m256i*)output, _mm512_cvtepi32_epi16(utf32));
        return count;
    }

    {
        // build surrogate pair words in 32-bit lanes

        //    t0 = 8 x [000000000000aaaa|aaaaaabbbbbbbbbb]
        const __m512i v_0001_0000 = _mm512_set1_epi32(0x00010000);
        const __m512i t0 = _mm512_sub_epi32(utf32, v_0001_0000);

        //    t1 = 8 x [000000aaaaaaaaaa|bbbbbbbbbb000000]
        const __m512i t1 = _mm512_slli_epi32(t0, 6);

        //    t2 = 8 x [000000aaaaaaaaaa|aaaaaabbbbbbbbbb] -- copy hi word from t1 to t0
        //         0xe4 = (t1 and v_ffff_0000) or (t0 and not v_ffff_0000)
        const __m512i v_ffff_0000 = _mm512_set1_epi32(0xffff0000);
        const __m512i t2 = _mm512_ternarylogic_epi32(t1, t0, v_ffff_0000, 0xe4);

        //    t2 = 8 x [110110aaaaaaaaaa|110111bbbbbbbbbb] -- copy hi word from t1 to t0
        //         0xba = (t2 and not v_fc00_fc000) or v_d800_dc00
        const __m512i v_fc00_fc00 = _mm512_set1_epi32(0xfc00fc00);
        const __m512i v_d800_dc00 = _mm512_set1_epi32(0xd800dc00);
        const __m512i t3 = _mm512_ternarylogic_epi32(t2, v_fc00_fc00, v_d800_dc00, 0xba);
        const __m512i t4 = _mm512_mask_blend_epi32(sp_mask, utf32, t3);
        const __m512i t5 = _mm512_ror_epi32(t4, 16);
        const  __mmask32 nonzero = _mm512_cmpneq_epi16_mask(t5, _mm512_setzero_si512());
         _mm512_mask_compressstoreu_epi16(output, nonzero, t5);
    }

    return count + static_cast<unsigned int>(count_ones(sp_mask));
}

/**
 * Store the last N bytes of previous followed by 512-N bytes from input.
 */
template <int N>
__m512i prev(__m512i input, __m512i previous) {
    static_assert(N<=32, "N must be no larger than 32");
    const __m512i movemask = _mm512_setr_epi32(28,29,30,31,0,1,2,3,4,5,6,7,8,9,10,11);
    const __m512i rotated = _mm512_permutex2var_epi32(input, movemask, previous);
#if SIMDUTF_GCC8
    constexpr int shift = 16-N; // workaround for GCC8
    return _mm512_alignr_epi8(input, rotated, shift);
#else
    return _mm512_alignr_epi8(input, rotated, 16-N);
#endif // SIMDUTF_GCC8
}

template <unsigned idx0, unsigned idx1, unsigned idx2, unsigned idx3>
__m512i shuffle_epi128(__m512i v) {
    static_assert((idx0 >= 0 && idx0 <= 3), "idx0 must be in range 0..3");
    static_assert((idx1 >= 0 && idx1 <= 3), "idx1 must be in range 0..3");
    static_assert((idx2 >= 0 && idx2 <= 3), "idx2 must be in range 0..3");
    static_assert((idx3 >= 0 && idx3 <= 3), "idx3 must be in range 0..3");

    constexpr unsigned shuffle = idx0 | (idx1 << 2) | (idx2 << 4) | (idx3 << 6);
    return _mm512_shuffle_i32x4(v, v, shuffle);
}

template <unsigned idx>
constexpr __m512i broadcast_epi128(__m512i v) {
    return shuffle_epi128<idx, idx, idx, idx>(v);
}

/**
 * Current unused.
 */
template <int N>
__m512i rotate_by_N_epi8(const __m512i input) {

    // lanes order: 1, 2, 3, 0 => 0b00_11_10_01
    const __m512i permuted = _mm512_shuffle_i32x4(input, input, 0x39);

    return _mm512_alignr_epi8(permuted, input, N);
}

/*
    expanded_utf8_to_utf32 converts expanded UTF-8 characters (`utf8`)
    stored at separate 32-bit lanes.

    For each lane we have also a character class (`char_class), given in form
    0x8080800N, where N is 4 higest bits from the leading byte; 0x80 resets
    corresponding bytes during pshufb.
*/
simdutf_really_inline __m512i expanded_utf8_to_utf32(__m512i char_class, __m512i utf8) {
    /*
        Input:
        - utf8: bytes stored at separate 32-bit words
        - valid: which words have valid UTF-8 characters

        Bit layout of single word. We show 4 cases for each possible
        UTF-8 character encoding. The `?` denotes bits we must not
        assume their value.

        |10dd.dddd|10cc.cccc|10bb.bbbb|1111.0aaa| 4-byte char
        |????.????|10cc.cccc|10bb.bbbb|1110.aaaa| 3-byte char
        |????.????|????.????|10bb.bbbb|110a.aaaa| 2-byte char
        |????.????|????.????|????.????|0aaa.aaaa| ASCII char
          byte 3    byte 2    byte 1     byte 0
    */

    /* 1. Reset control bits of continuation bytes and the MSB
          of the leading byte; this makes all bytes unsigned (and
          does not alter ASCII char).

        |00dd.dddd|00cc.cccc|00bb.bbbb|0111.0aaa| 4-byte char
        |00??.????|00cc.cccc|00bb.bbbb|0110.aaaa| 3-byte char
        |00??.????|00??.????|00bb.bbbb|010a.aaaa| 2-byte char
        |00??.????|00??.????|00??.????|0aaa.aaaa| ASCII char
         ^^        ^^        ^^        ^
    */
    __m512i values;
    const __m512i v_3f3f_3f7f = _mm512_set1_epi32(0x3f3f3f7f);
    values = _mm512_and_si512(utf8, v_3f3f_3f7f);

    /* 2. Swap and join fields A-B and C-D

        |0000.cccc|ccdd.dddd|0001.110a|aabb.bbbb| 4-byte char
        |0000.cccc|cc??.????|0001.10aa|aabb.bbbb| 3-byte char
        |0000.????|????.????|0001.0aaa|aabb.bbbb| 2-byte char
        |0000.????|????.????|000a.aaaa|aa??.????| ASCII char */
    const __m512i v_0140_0140 = _mm512_set1_epi32(0x01400140);
    values = _mm512_maddubs_epi16(values, v_0140_0140);

    /* 3. Swap and join fields AB & CD

        |0000.0001|110a.aabb|bbbb.cccc|ccdd.dddd| 4-byte char
        |0000.0001|10aa.aabb|bbbb.cccc|cc??.????| 3-byte char
        |0000.0001|0aaa.aabb|bbbb.????|????.????| 2-byte char
        |0000.000a|aaaa.aa??|????.????|????.????| ASCII char */
    const __m512i v_0001_1000 = _mm512_set1_epi32(0x00011000);
    values = _mm512_madd_epi16(values, v_0001_1000);

    /* 4. Shift left the values by variable amounts to reset highest UTF-8 bits
        |aaab.bbbb|bccc.cccd|dddd.d000|0000.0000| 4-byte char -- by 11
        |aaaa.bbbb|bbcc.cccc|????.??00|0000.0000| 3-byte char -- by 10
        |aaaa.abbb|bbb?.????|????.???0|0000.0000| 2-byte char -- by 9
        |aaaa.aaa?|????.????|????.????|?000.0000| ASCII char -- by 7 */
    {
        /** pshufb

        continuation = 0
        ascii    = 7
        _2_bytes = 9
        _3_bytes = 10
        _4_bytes = 11

        shift_left_v3 = 4 * [
            ascii, # 0000
            ascii, # 0001
            ascii, # 0010
            ascii, # 0011
            ascii, # 0100
            ascii, # 0101
            ascii, # 0110
            ascii, # 0111
            continuation, # 1000
            continuation, # 1001
            continuation, # 1010
            continuation, # 1011
            _2_bytes, # 1100
            _2_bytes, # 1101
            _3_bytes, # 1110
            _4_bytes, # 1111
        ] */
        const __m512i shift_left_v3 = _mm512_setr_epi64(
            0x0707070707070707,
            0x0b0a090900000000,
            0x0707070707070707,
            0x0b0a090900000000,
            0x0707070707070707,
            0x0b0a090900000000,
            0x0707070707070707,
            0x0b0a090900000000
        );

        const __m512i shift = _mm512_shuffle_epi8(shift_left_v3, char_class);
        values = _mm512_sllv_epi32(values, shift);
    }

    /* 5. Shift right the values by variable amounts to reset lowest bits
        |0000.0000|000a.aabb|bbbb.cccc|ccdd.dddd| 4-byte char -- by 11
        |0000.0000|0000.0000|aaaa.bbbb|bbcc.cccc| 3-byte char -- by 16
        |0000.0000|0000.0000|0000.0aaa|aabb.bbbb| 2-byte char -- by 21
        |0000.0000|0000.0000|0000.0000|0aaa.aaaa| ASCII char -- by 25 */
    {
        // 4 * [25, 25, 25, 25, 25, 25, 25, 25, 0, 0, 0, 0, 21, 21, 16, 11]
        const __m512i shift_right = _mm512_setr_epi64(
            0x1919191919191919,
            0x0b10151500000000,
            0x1919191919191919,
            0x0b10151500000000,
            0x1919191919191919,
            0x0b10151500000000,
            0x1919191919191919,
            0x0b10151500000000
        );

        const __m512i shift = _mm512_shuffle_epi8(shift_right, char_class);
        values = _mm512_srlv_epi32(values, shift);
    }

    return values;
}


simdutf_really_inline __m512i expand_and_identify(__m512i lane0, __m512i lane1, int &count) {
    const __m512i merged = _mm512_mask_mov_epi32(lane0, 0x1000, lane1);
    const __m512i expand_ver2 = _mm512_setr_epi64(
                0x0403020103020100,
                0x0605040305040302,
                0x0807060507060504,
                0x0a09080709080706,
                0x0c0b0a090b0a0908,
                0x0e0d0c0b0d0c0b0a,
                0x000f0e0d0f0e0d0c,
                0x0201000f01000f0e
    );
    const __m512i input = _mm512_shuffle_epi8(merged, expand_ver2);
    const __m512i v_0000_00c0 = _mm512_set1_epi32(0xc0);
    const __m512i t0 = _mm512_and_si512(input, v_0000_00c0);
    const __m512i v_0000_0080 = _mm512_set1_epi32(0x80);
    const __mmask16 leading_bytes = _mm512_cmpneq_epu32_mask(t0, v_0000_0080);
    count = static_cast<int>(count_ones(leading_bytes));
    return  _mm512_mask_compress_epi32(_mm512_setzero_si512(), leading_bytes, input);
}

simdutf_really_inline __m512i expand_utf8_to_utf32(__m512i input) {
    __m512i char_class = _mm512_srli_epi32(input, 4);
    /*  char_class = ((input >> 4) & 0x0f) | 0x80808000 */
    const __m512i v_0000_000f = _mm512_set1_epi32(0x0f);
    const __m512i v_8080_8000 = _mm512_set1_epi32(0x80808000);
    char_class = _mm512_ternarylogic_epi32(char_class, v_0000_000f, v_8080_8000, 0xea);
    return expanded_utf8_to_utf32(char_class, input);
}
