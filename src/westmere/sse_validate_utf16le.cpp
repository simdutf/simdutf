/*
    In UTF16 words in range 0xD800 to 0xDFFF have special meaning.

    In a vectorized algorithm we want to examine the most significant
    nibble in order to select a fast path. If none of highest nibbles
    are 0xD (13), than we are sure that UTF16 chunk in a vector
    register is valid.

    Let us analyze what we need to check if the nibble is 0xD. The
    value of the preceding nibble determines what we have:

    0xd000 .. 0xd7ff - a valid word
    0xd800 .. 0xdbff - low surrogate
    0xdc00 .. 0xdfff - high surrogate

    Other constraints we have to consider:
    - there must not be two consecutive low surrogates (0xd800 .. 0xdbff)
    - there must not be two consecutive high surrogates (0xdc00 .. 0xdfff)
    - there must not be sole low surrogate nor high surrogate

    We're going to build three bitmasks based on the 3rd nibble:
    - V = valid word,
    - L = low surrogate (0xd800 .. 0xdbff)
    - H = high surrogate (0xdc00 .. 0xdfff)

      0   1   2   3   4   5   6   7    <--- word index
    [ V | L | H | L | H | V | V | L ]
      1   0   0   0   0   1   1   0     - V = valid masks
      0   1   0   1   0   0   0   1     - L = low surrogate
      0   0   1   0   1   0   0   0     - H high surrogate


      1   0   0   0   0   1   1   0   V = valid masks
      0   1   0   1   0   0   0   0   a = L & (H >> 1)
      0   0   1   0   1   0   0   0   b = a << 1
      1   1   1   1   1   1   1   0   c = V | a | b
                                  ^
                                  the last bit can be zero, we just consume 7 words
                                  and recheck this word in the next iteration
*/

/* Returns:
   - pointer to the last unprocessed character (a scalar fallback should check the rest);
   - nullptr if an error was detected.
*/
const char16_t* sse_validate_utf16le(const char16_t* input, size_t size) {
    const char16_t* end = input + size;

    const __m128i v_d8 = _mm_set1_epi8(int8_t(0xd8));
    const __m128i v_f8 = _mm_set1_epi8(int8_t(0xf8));
    const __m128i v_fc = _mm_set1_epi8(int8_t(0xfc));
    const __m128i v_dc = _mm_set1_epi8(int8_t(0xdc));

    while (input + 16 < end) {
        // 0. Load data: since the validation takes into account only higher
        //    byte of each word, we compress the two vectors into one which
        //    consists only the higher bytes.
        const __m128i in0 = _mm_loadu_si128((__m128i*)(input + 0*8));
        const __m128i in1 = _mm_loadu_si128((__m128i*)(input + 1*8));

        const __m128i t0  = _mm_srli_epi16(in0, 8);
        const __m128i t1  = _mm_srli_epi16(in1, 8);

        const __m128i in  = _mm_packus_epi16(t0, t1);

        // 1. Check whether we have any 0xD800..DFFF word (0b1101'1xxx'yyyy'yyyy).
        const __m128i surrogates_wordmask = _mm_cmpeq_epi8(_mm_and_si128(in, v_f8), v_d8);
        const uint16_t surrogates_bitmask = static_cast<uint16_t>(_mm_movemask_epi8(surrogates_wordmask));
        if (surrogates_bitmask == 0x0000) {
            input += 16;
        } else {
            // 2. We have some surrogates that have to be distinguished:
            //    - low  surrogates: 0b1101'10xx'yyyy'yyyy (0xD800..0xDBFF)
            //    - high surrogates: 0b1101'11xx'yyyy'yyyy (0xDC00..0xDFFF)
            //
            //    Fact: high surrogate has 11th bit set (3rd bit in the higher word)

            // V - non-surrogate words
            //     V = not surrogates_wordmask
            const uint16_t V = ~surrogates_bitmask;

            // H - word-mask for high surrogates: the six highest bits are 0b1101'11
            const __m128i vH = _mm_cmpeq_epi8(_mm_and_si128(in, v_fc), v_dc);
            const uint16_t H = static_cast<uint16_t>(_mm_movemask_epi8(vH));

            // L - word mask for low surrogates
            //     L = not H and surrogates_wordmask
            const uint16_t L = static_cast<uint16_t>(~H & surrogates_bitmask);

            const uint16_t a = L & (H >> 1);  // A low surrogate must be followed by high one.
                                              // (A low surrogate placed in the 7th register's word
                                              // is an exception we handle.)
            const uint16_t b = a << 1;        // Just mark that the opposite fact is hold,
                                              // thanks to that we have only two masks for valid case.
            const uint16_t c = V | a | b;     // Combine all the masks into the final one.

            if (c == 0xffff) {
                // The whole input register contains valid UTF16, i.e.,
                // either single words or proper surrogate pairs.
                input += 16;
            } else if (c == 0x7fff) {
                // The 15 lower words of the input register contains valid UTF16.
                // The 15th word may be either a low or high surrogate. It the next
                // iteration we 1) check if the low surrogate is followed by a high
                // one, 2) reject sole high surrogate.
                input += 15;
            } else {
                return nullptr;
            }
        }
    }

    return input;
}
