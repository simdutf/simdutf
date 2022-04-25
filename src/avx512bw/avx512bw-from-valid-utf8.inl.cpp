// file included directly

// File contains conversion procedure from VALID UTF-8 strings.

/*
    expand_bytes takes 16 + 4 bytes (of a UTF-8 string)
    and loads all possible 4-byte substring into an AVX512 register.

    For example if we have bytes abcdefgh... we create following 32-bit lanes

    [abcd|bcde|cdef|defg|efgh|...]
     ^                          ^
     byte 0 of reg              byte 63 of reg
*/
__m512i expand_bytes(const char* ptr) {
    // load bytes 0..15 (16) and broadcast the 128-bit lane
    const __m128i lane    = _mm_loadu_si128((const __m128i*)ptr);
    const __m512i lane512 = _mm512_castsi128_si512(lane);
    const __m512i t0 = _mm512_permutexvar_epi32(broadcast_0th_lane, lane512);

    // load bytes 16..19 (4)
    uint32_t tmp1;
    memcpy(&tmp1, ptr + 16, sizeof(tmp1));
    const __m512i t1 = _mm512_set1_epi32(tmp1); // latency 3

    // In the last lane we need bytes 13..19, so we're placing
    // 32-bit word from t1 at 0th position of the lane
    const __m512i t2 = _mm512_mask_mov_epi32(t0, 0x1000, t1); // latency 1

    // We could also do the above using a single load/mask instruction.
    // We want to load the 32-bit at ptr+16 in 12th position within the register.
    // ptr + 4*(-8) + 12*4 = ptr + 4*4.
    // const __m512i t2 = _mm512_mask_loadu_epi32(t0, 0x1000, ptr + 4*(-8)); // latency: 8
    // However, _mm512_mask_loadu_epi32 might be remarkably expensive.
    /** pshufb
        # lane{0,1,2} have got bytes: [  0,  1,  2,  3,  4,  5,  6,  8,  9, 10, 11, 12, 13, 14, 15]
        # lane3 has got bytes:        [ 16, 17, 18, 19,  4,  5,  6,  8,  9, 10, 11, 12, 13, 14, 15]

        expand_ver2 = [
            # lane 0:
            0, 1, 2, 3,
            1, 2, 3, 4,
            2, 3, 4, 5,
            3, 4, 5, 6,

            # lane 1:
            4, 5, 6, 7,
            5, 6, 7, 8,
            6, 7, 8, 9,
            7, 8, 9, 10,

            # lane 2:
             8,  9, 10, 11,
             9, 10, 11, 12,
            10, 11, 12, 13,
            11, 12, 13, 14,

            # lane 3 order: 13, 14, 15, 16 14, 15, 16, 17, 15, 16, 17, 18, 16, 17, 18, 19
            12, 13, 14, 15,
            13, 14, 15,  0,
            14, 15,  0,  1,
            15,  0,  1,  2,
        ]
    */
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

    return _mm512_shuffle_epi8(t2, expand_ver2);
}

/*
    expanded_utf8_to_utf32 converts expanded UTF-8 characters (`utf8`)
    stored at separate 32-bit lanes.

    For each lane we have also a character class (`char_class), given in form
    0x8080800N, where N is 4 higest bits from the leading byte; 0x80 resets
    corresponding bytes during pshufb.
*/
__m512i expanded_utf8_to_utf32(__m512i char_class, __m512i utf8) {
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
    values = _mm512_and_si512(utf8, v_3f3f_3f7f);

    /* 2. Swap and join fields A-B and C-D

        |0000.cccc|ccdd.dddd|0001.110a|aabb.bbbb| 4-byte char
        |0000.cccc|cc??.????|0001.10aa|aabb.bbbb| 3-byte char
        |0000.????|????.????|0001.0aaa|aabb.bbbb| 2-byte char
        |0000.????|????.????|000a.aaaa|aa??.????| ASCII char */
    values = _mm512_maddubs_epi16(values, v_0140_0140);

    /* 3. Swap and join fields AB & CD

        |0000.0001|110a.aabb|bbbb.cccc|ccdd.dddd| 4-byte char
        |0000.0001|10aa.aabb|bbbb.cccc|cc??.????| 3-byte char
        |0000.0001|0aaa.aabb|bbbb.????|????.????| 2-byte char
        |0000.000a|aaaa.aa??|????.????|????.????| ASCII char */
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
std::pair<const char*, OUTPUT*> valid_utf8_to_fixed_length(const char* str, size_t len, OUTPUT* out_buffer) {
    constexpr bool UTF32 = std::is_same<OUTPUT, uint32_t>::value;
    constexpr bool UTF16 = std::is_same<OUTPUT, char16_t>::value;
    static_assert(UTF32 or UTF16, "output type has to be uint32_t (for UTF-32) or char16_t (for UTF-16)");

    const char* ptr = str;
    const char* end = ptr + len;

    OUTPUT* output = out_buffer;

    while (ptr + 16 + 4 < end) {
        // 1. Load all possible 4-byte substring into an AVX512 register.
        const __m512i input = expand_bytes(ptr);

        /* 2. Classify which words contain valid UTF-8 characters.
              We test if the 0th byte is not a continuation byte (0b10xxxxxx) */
        __mmask16 leading_byte;
        {
            const __m512i t0 = _mm512_and_si512(input, v_0000_00c0);
            leading_byte = _mm512_cmpneq_epu32_mask(t0, v_0000_0080);
        }
        const int valid_count = __builtin_popcount(_cvtmask16_u32(leading_byte));

        // 3. Find out character classes
        __m512i char_class;
        {
            const __m512i t0 = _mm512_srli_epi32(input, 4);
            // char_class = ((input >> 4) & 0x0f) | 0x80808000
            char_class = _mm512_ternarylogic_epi32(t0, v_0000_000f, v_8080_8000, 0xea);
        }

        // 3. Convert words into UTF-32
        const __m512i utf32 = expanded_utf8_to_utf32(char_class, input);

        // 4. Pack only the valid words
        const __m512i out = _mm512_mask_compress_epi32(_mm512_setzero_si512(), leading_byte, utf32);

        // 5. Store them
        if (UTF32) {
            _mm512_storeu_si512((__m512i*)output, out);
            output += valid_count;
        } else {
            output += utf32_to_utf16(out, valid_count, output);
        }

        ptr += 16;
    }

    return {ptr, output};
}

using utf8_to_utf16_result = std::pair<const char*, char16_t*>;
