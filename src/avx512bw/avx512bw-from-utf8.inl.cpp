// file included directly

// File contains conversion procedure from possibly invalid UTF-8 strings.

/*
validate_utf8_structure only checks if leading bytes
are followed by proper number of continuation bytes
and then there's another leading byte.

We don't handle end-of-stream here.
*/
bool validate_utf8_structure(__m512i input) {
    /* 1. bitmask for various character byte classes.

        leading: 111010011100011010001
                 abccdddefgggghiijjjjk   a-k -- 11 UTF-8 chars characters

        ascii:   110000011000010000001
                 ab     ef    h      k

        2 bytes: 001000000000001000000
                   cc          ii

        3 bytes: 000010000000000000000
                     ddd

        4 bytes: 000000000100010010000
                          gggg   jjjj
    */
    __mmask64 leading;
    __mmask64 ascii;
    __mmask64 _2bytes;
    __mmask64 _3bytes;
    __mmask64 _4bytes;

    // we can validate 60 - 4 leading bytes
    constexpr __mmask64 mask  = 0x0ffffffffffffffflu;

    {
        const __m512i t0 = _mm512_and_si512(input, v_c0);
        leading = _mm512_cmpneq_epu8_mask(t0, v_80);
    }
    {
        ascii = _mm512_testn_epi8_mask(input, v_80) & mask;
    }
    {
        const __m512i t0 = _mm512_and_si512(input, v_e0);
        _2bytes = _mm512_cmpeq_epi8_mask(t0, v_c0) & mask;
    }
    {
        const __m512i t0 = _mm512_and_si512(input, v_f0);
        _3bytes = _mm512_cmpeq_epi8_mask(t0, v_e0) & mask;
    }
    {
        const __m512i t0 = _mm512_and_si512(input, v_f8);
        _4bytes = _mm512_cmpeq_epi8_mask(t0, v_f0) & mask;
    }

    const __mmask64 next1 = _kshiftri_mask64(leading, 1);
    const __mmask64 next2 = _kshiftri_mask64(leading, 2);
    const __mmask64 next3 = _kshiftri_mask64(leading, 3);
    const __mmask64 next4 = _kshiftri_mask64(leading, 4);

    /* 1. validate ASCII
        ascii =             110000011000010000001
                            ^^^^^^^^^^^^^^^^^
                            60 considered bytes
        next1 =             110100111000110100010
        next & ascii =      100000011000010000000
    */
    const __mmask64 valid_ascii = _kand_mask64(ascii, next1);


    /* 2. validate 2-byte chars

        2-byte   001000000000001000000
        ~next1   001011000111001011101 -- expect a continuation byte
        next2    101001110001101000100 -- and a leading byte
        ------------------------------
        and-all  001000000000001000000
    */
    const __mmask64 valid_2bytes = _kandn_mask64(next1, _kand_mask64(_2bytes, next2));

    /* 3. validate 3-byte chars

        3-byte   000010000000000000000
        ~next1   001011000111001011100 -- expect a continuation byte
        ~next2   010110001110010111000 -- another continuation byte
        next3    010011100011010001000 -- and a leading byte
        ------------------------------
        and-all  000010000000000000000

    */
    const __mmask64 valid_3bytes = _kandn_mask64(_kor_mask64(next1, next2), _kand_mask64(_3bytes, next3));

    /* 4. validate 4-byte chars

        4-byte   000000000100010010000
        ~next1   001011000111001011100 -- expect a continuation byte
        ~next2   010110001110010111000 -- another continuation
        ~next3   101100011100101110000 -- another continuation
        next4    100111000110100010000 -- and a leading byte
        ------------------------------
        and-all  000000000100000010000
    */

	const __mmask64 t0 = _kor_mask64(_kor_mask64(next1, next2), next3);
    const __mmask64 t1 = _kand_mask64(_4bytes, next4);

    // valid_3bytes = (_3bytes & next3) & ~(next1 | next2);
    const __mmask64 valid_4bytes = _kandn_mask64(t0, t1);

    // eqX == 0 iff the arumets are equal
    const __mmask64 eq0 = _kxor_mask64(valid_ascii, ascii);
    const __mmask64 eq1 = _kxor_mask64(valid_2bytes, _2bytes);
    const __mmask64 eq2 = _kxor_mask64(valid_3bytes, _3bytes);
    const __mmask64 eq3 = _kxor_mask64(valid_4bytes, _4bytes);

    unsigned char unused1;
    unsigned char unused2;
    // (eq0 | eq1) == 0 and (eq2 | eq3) == 0
    return _kortest_mask64_u8(eq0, eq1, &unused1)
        && _kortest_mask64_u8(eq2, eq3, &unused2);
}

/*
validate_leading_bytes is a direct translation of branchy algorithm from
"Validating UTF-8 In Less Than One Instruction Per Byte".

There are following observations:

1. ASCII bytes do not need handling
2. Continuation bytes are rejected instantly
3. For 2-/3-/4-byte chars it's sufficient to check the values
   of leading byte and the first continuation byte. The
   subsequent continuation bytes may have any values

a) For 2-byte chars we have one rule:

- The leading byte must not be 0xc0 nor 0xc1.

b) For 3-byte chars we decide based on the lower nibble of the leading byte:

- 0: valid if (continuation1 & 0x3f) > 0x1f
- 14: valid if (continuation1 & 0x3f) <= 0x1f [negation of the above case]
- 0..13, 14, 15: always valid

We calculate the following expression:

    c = (continuation1 & 0x3f)
    x = (c ^ fixup) > 0x1f

    where `fixup` is a function of nibble value:

        fixup[nibble] = 0  => 0x00
                        14 => 0x40
                        _  => 0xc0

    case 1 fixup = 0x00:
                                                          c ^ fixup > 0x1f
    c <= 0x1f: 0b000x_xxxx ^ 0xb0000_0000 = 0xb000x_xxxx  false
    c >  0x1f: 0b001x_xxxx ^ 0xb0000_0000 = 0xb001x_xxxx  true

    case 2 fixup = 0x40:
                                                          c ^ fixup > 0x1f
    c <= 0x1f: 0b000x_xxxx ^ 0xb0010_0000 = 0xb001x_xxxx  true
    c >  0x1f: 0b001x_xxxx ^ 0xb0010_0000 = 0xb000x_xxxx  false

    case 3 fixup = 0x40:
                                                          c ^ fixup > 0x1f
    c <= 0x1f: 0b000x_xxxx ^ 0xb1100_0000 = 0xb110x_xxxx  true
    c >  0x1f: 0b001x_xxxx ^ 0xb1100_0000 = 0xb111x_xxxx  true


c) For 4-byte chars we decide based on the lower nibble of the leading byte:

- 0: valid if (continuation1 & 0x3f) > 0x0f
- 1, 2, 3: always valid
- 4: valid if (continuation1 & 0x3f) <= 0x0f [negation of case #0]
- 5, 15: always invalid

In this case we build a 2-bit values where one bit stores result of
(continuation1 & 0x3f) > 0x0f and another is negation of that bit.  Then case
#0 is encoded as mask 0b01, case #4 is encoded as 0b10, cases #{1,2,3} as 0b11
and the remaining cases as 0b00. The validity of chars is checked with the
following expression:

    value & mask != 0

To create a 2-bit value we subtract from c = (continuation1 & 0x3f) the number
0x10. If c = 0b0000_xxxx (<= 0x0f) then the result of the subtraction is always
in form 0b1111_xxxx. The two most significant bits encodes condition (c <= 0x0f).
Now we negate 6th bit, and then the these two most significant bits represents
the required combination which is later tested.
*/
__mmask64 validate_leading_bytes(__m512i leading_bytes, __m512i continuation1, __mmask64 tested_chars) {

    const __m512i nibble0 = _mm512_and_si512(leading_bytes, v_0f);

    // 1. Assume all non-continuation bytes are valid leading bytes
    //    We unset all continuation bytes (0b10xx_xxxx) and ASCII chars
    //    (0b0xxx_xxxx) --- so we looking for 0b11xx_xxxx chars.
    //
    //    (We may mask-out some leading bytes via `tested_chars`).
    __mmask64 valid;
    {
        const __m512i t0 = _mm512_and_si512(leading_bytes, v_c0);
        valid = _mm512_cmpeq_epi8_mask(t0, v_c0);
    }

    __mmask64 _2bytes;
    {
        const __m512i t0 = _mm512_and_si512(leading_bytes, v_e0);
        _2bytes = _mm512_cmpeq_epi8_mask(t0, v_c0);
    }

    __mmask64 _3bytes;
    {
        const __m512i t0 = _mm512_and_si512(leading_bytes, v_f0);
        _3bytes = _mm512_cmpeq_epi8_mask(t0, v_e0);
    }

    __mmask64 _4bytes;
    {
        const __m512i t0 = _mm512_and_si512(leading_bytes, v_f8);
        _4bytes = _mm512_cmpeq_epi8_mask(t0, v_f0);
    }

    // 1. Handle 2-byte chars
    //    Valid if leading byte is not 0xc0 nor 0xc1
    __mmask64 valid_2bytes = _mm512_mask_cmpge_epu8_mask(_2bytes, leading_bytes, v_c2);

    // 4. Handle 3-byte chars
    //    let M = (continuation1 & 03f) > 0x1f
    continuation1 = _mm512_and_si512(continuation1, v_3f);
    __mmask64 valid_3bytes;
    {
        /** pshufb
            M     = 0b0000_0000 # we test 5th bit
            notM  = 0b0010_0000
            true  = 0b1100_0000

            fixup_lookup = 4 * [
                M,      # 0b0000
                true,   # 0b0001
                true,   # 0b0010
                true,   # 0b0011
                true,   # 0b0100
                true,   # 0b0101
                true,   # 0b0110
                true,   # 0b0111
                true,   # 0b1000
                true,   # 0b1001
                true,   # 0b1010
                true,   # 0b1011
                true,   # 0b1100
                notM,   # 0b1101
                true,   # 0b1110
                true,   # 0b1111
        ] */
        const __m512i fixup_lookup = _mm512_setr_epi64(
            0xc0c0c0c0c0c0c000,
            0xc0c020c0c0c0c0c0,
            0xc0c0c0c0c0c0c000,
            0xc0c020c0c0c0c0c0,
            0xc0c0c0c0c0c0c000,
            0xc0c020c0c0c0c0c0,
            0xc0c0c0c0c0c0c000,
            0xc0c020c0c0c0c0c0
        );

        const __m512i fixup = _mm512_shuffle_epi8(fixup_lookup, nibble0);
        const __m512i t0 = _mm512_xor_si512(fixup, continuation1);
        valid_3bytes = _mm512_mask_cmpgt_epu8_mask(_3bytes, t0, v_1f);
    }

    // 5. Handle 4-byte chars
    __mmask64 valid_4bytes;
    {
        // continuation1 in range [0..63] (0b0000_0000 .. 0b0011_1111)
        //
        // case 1: c1 <= 0xf:  0b0000_xxxx - 0x10 = 0b1111_yyyy
        // case 2: c1  > 0xf:  0b00xx_xxxx - 0x10 = 0b00yy_yyyy
        __m512i t0;
        t0 = _mm512_sub_epi8(continuation1, v_10);
        t0 = _mm512_xor_si512(t0, v_40);
        // bit 7th of c: continuation1 <= 0x0f
        // bit 6th of c: continuation1 >  0x0f

        /** pshufb
            le_0f   = 0x80  # c1[7] = continuation1 <= 0xf0
            gt_0f   = 0x40  # c1[6] = not c1[7]
            true    = gt_0f | le_0f
            false   = 0x00

            mask_lookup = 4 * [
                gt_0f,  # 0b0000
                true,   # 0b0001
                true,   # 0b0010
                true,   # 0b0011
                le_0f,  # 0b0100
                false,  # 0b0101
                false,  # 0b0110
                false,  # 0b0111
                false,  # 0b1000
                false,  # 0b1001
                false,  # 0b1010
                false,  # 0b1011
                false,  # 0b1100
                false,  # 0b1101
                false,  # 0b1110
                false,  # 0b1111
        ] */
        const __m512i mask_lookup = _mm512_setr_epi64(
            0x00000080c0c0c040,
            0x0000000000000000,
            0x00000080c0c0c040,
            0x0000000000000000,
            0x00000080c0c0c040,
            0x0000000000000000,
            0x00000080c0c0c040,
            0x0000000000000000
        );
        const __m512i mask = _mm512_shuffle_epi8(mask_lookup, nibble0);

        valid_4bytes = _mm512_mask_test_epi8_mask(_4bytes, mask, t0);
    }

    {
        // all: marks all valid leading bytes
        __mmask64 all = _kor_mask64(valid_2bytes, _kor_mask64(valid_3bytes, valid_4bytes));

        // reset leading byte marks from input
        valid = _kxor_mask64(valid, all);
        valid = _kand_mask64(valid, tested_chars);
    }

    return valid == 0;
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

__m512i rotate_by1_epi8(const __m512i input) {

    // lanes order: 1, 2, 3, 0 => 0b00_11_10_01
    const __m512i permuted = _mm512_shuffle_i32x4(input, input, 0x39);

    return _mm512_alignr_epi8(permuted, input, 1);
}

template <typename OUTPUT>
std::pair<const char*, OUTPUT*> validating_utf8_to_fixed_length(const char* str, size_t len, OUTPUT* dwords) {
    constexpr bool UTF32 = std::is_same<OUTPUT, uint32_t>::value;
    constexpr bool UTF16 = std::is_same<OUTPUT, char16_t>::value;
    static_assert(UTF32 or UTF16, "output type has to be uint32_t (for UTF-32) or char16_t (for UTF-16)");

    const char* ptr = str;
    const char* end = ptr + len;

    OUTPUT* output = dwords;

    while (ptr + 64 < end) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        const __mmask64 ascii = _mm512_test_epi8_mask(utf8, v_80);
        if (ascii == 0) {
            const __m256i h0 = _mm512_castsi512_si256(utf8);
            const __m256i h1 = _mm512_extracti32x8_epi32(utf8, 1);

            const __m128i t0 = _mm256_castsi256_si128(h0);
            const __m128i t1 = _mm256_extracti32x4_epi32(h0, 1);
            const __m128i t2 = _mm256_castsi256_si128(h1);
            const __m128i t3 = _mm256_extracti32x4_epi32(h1, 1);

            if (UTF32) {
                _mm512_storeu_si512((__m512i*)(output + 0*16), _mm512_cvtepu8_epi32(t0));
                _mm512_storeu_si512((__m512i*)(output + 1*16), _mm512_cvtepu8_epi32(t1));
                _mm512_storeu_si512((__m512i*)(output + 2*16), _mm512_cvtepu8_epi32(t2));
                _mm512_storeu_si512((__m512i*)(output + 3*16), _mm512_cvtepu8_epi32(t3));
            }
            else {
                _mm256_storeu_si256((__m256i*)(output + 0*16), _mm256_cvtepu8_epi16(t0));
                _mm256_storeu_si256((__m256i*)(output + 1*16), _mm256_cvtepu8_epi16(t1));
                _mm256_storeu_si256((__m256i*)(output + 2*16), _mm256_cvtepu8_epi16(t2));
                _mm256_storeu_si256((__m256i*)(output + 3*16), _mm256_cvtepu8_epi16(t3));
            }

            output += 64;
            ptr += 64;
            continue;
        }

        // 1. Validate the structure of UTF-8 sequence.
        //    Note: procedure validates chars that starts in range [0..60]
        //    of input.
        if (not validate_utf8_structure(utf8)) {
            return {ptr, nullptr};
        }

        // 2. Precise validate: once we know that the bytes structure is correct,
        //    we have to check for some forbidden input values.
        //    Note: this procedure validates chars in range [0..63], due to
        //    method of obtaining continuation1 vector.
        const __m512i continuation1 = rotate_by1_epi8(utf8);
        if (not validate_leading_bytes(utf8, continuation1, 0x0ffffffffffffffflu)) {
            return {ptr, nullptr};
        }

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

        // 3. Convert 3*16 input bytes
        // We waste the last 16 bytes, which are not fully validated...
        // this is another topic to check in the future.
#define TRANSCODE16(LANE0, LANE1)                                                                            \
        {                                                                                                    \
            const __m512i merged = _mm512_mask_mov_epi32(LANE0, 0x1000, LANE1);                              \
            const __m512i input = _mm512_shuffle_epi8(merged, expand_ver2);                                  \
                                                                                                             \
            __mmask16 leading_bytes;                                                                         \
            const __m512i t0 = _mm512_and_si512(input, v_0000_00c0);                                         \
            leading_bytes = _mm512_cmpneq_epu32_mask(t0, v_0000_0080);                                       \
                                                                                                             \
            __m512i char_class;                                                                              \
            char_class = _mm512_srli_epi32(input, 4);                                                        \
            /*  char_class = ((input >> 4) & 0x0f) | 0x80808000 */                                           \
            char_class = _mm512_ternarylogic_epi32(char_class, v_0000_000f, v_8080_8000, 0xea);              \
                                                                                                             \
            const int valid_count = __builtin_popcount(leading_bytes);                                       \
            const __m512i utf32 = expanded_utf8_to_utf32(char_class, input);                                 \
                                                                                                             \
            const __m512i out = _mm512_mask_compress_epi32(_mm512_setzero_si512(), leading_bytes, utf32);    \
                                                                                                             \
            if (UTF32) {                                                                                     \
                _mm512_storeu_si512((__m512i*)output, out);                                                  \
                output += valid_count;                                                                       \
            } else {                                                                                         \
                output += utf32_to_utf16(out, valid_count, output);                                          \
            }                                                                                                \
        }

        const __m512i lane0 = broadcast_epi128<0>(utf8);
        const __m512i lane1 = broadcast_epi128<1>(utf8);
        TRANSCODE16(lane0, lane1)

        const __m512i lane2 = broadcast_epi128<2>(utf8);
        TRANSCODE16(lane1, lane2)

        const __m512i lane3 = broadcast_epi128<3>(utf8);
        TRANSCODE16(lane2, lane3)

        ptr += 3*16;
    }

    return {ptr, output};
}
