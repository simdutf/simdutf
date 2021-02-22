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

    const __m128i v_d000 = _mm_set1_epi32(0xd000'd000);
    const __m128i v_f000 = _mm_set1_epi32(0xf000'f000);
    const __m128i v_0f00 = _mm_set1_epi32(0x0f00'0f00);
    const __m128i v_0080 = _mm_set1_epi32(0x0080'0080);

    while (input + 8 < end) {
        const __m128i in = _mm_loadu_si128((__m128i*)input);

        // 1. Check whether we have any 0xD??? word.
        const __m128i eq = _mm_cmpeq_epi16(_mm_and_si128(in, v_f000), v_d000);
        if (_mm_movemask_epi8(eq) == 0x0000) {
            input += 8;
        } else {
            // 2. We have some 0xD??? words, classify each word by 3rd nibble 'x'
            // in = [ 1101 | xxxx | ???? | ???? ] x 8
            //          3      2      1      0    <-- nibble index

            // t0 = [ 0000 | xxxx | 0000 | 0000 ] x 8
            const __m128i t0 = _mm_and_si128(in, v_0f00);

            // Reset 3rd nibble when 4rd nibble is not 0xd.
            // Thanks to that it will be marked as valid during classification.
            const __m128i t1 = _mm_and_si128(t0, eq);

            const __m128i classify_nibble = _mm_setr_epi8(
                /* bits: 0bVLH0'0000 */
                /* 0 */  char(0b1000'0000),  // valid word (V)
                /* 1 */  char(0b1000'0000),  // valid word
                /* 2 */  char(0b1000'0000),  // valid word
                /* 3 */  char(0b1000'0000),  // valid word
                /* 4 */  char(0b1000'0000),  // valid word
                /* 5 */  char(0b1000'0000),  // valid word
                /* 6 */  char(0b1000'0000),  // valid word
                /* 7 */  char(0b1000'0000),  // valid word
                /* 8 */  char(0b0100'0000),  // low surrogate (L)
                /* 9 */  char(0b0100'0000),  // low surrogate
                /* a */  char(0b0100'0000),  // low surrogate
                /* b */  char(0b0100'0000),  // low surrogate
                /* c */  char(0b0010'0000),  // high surrogate (H)
                /* d */  char(0b0010'0000),  // high surrogate
                /* e */  char(0b0010'0000),  // high surrogate
                /* f */  char(0b0010'0000)   // high surrogate
            );
            // t2 = [ VHL0 | 0000 | 1000 | 0000 ] x 8
            const __m128i t2 = _mm_shuffle_epi8(classify_nibble, t1);

            // Note: We'are getting MSB for each byte, not word. Each word
            //       yields two bits, but only the 1st one of the pair is important.
            const int V = _mm_movemask_epi8(t2); // TODO: check if V can be reused from the first _mm_movemask_epi8
            const int L = _mm_movemask_epi8(_mm_slli_epi16(t2, 1));
            const int H = _mm_movemask_epi8(_mm_slli_epi16(t2, 2));

            const int a = L & (H >> 2);  // A low surrogate must be followed by high one.
                                         // (A low surrogate placed in the 7th register's word
                                         // is an exception we handle.)
            const int b = a << 2;        // Just mark that the opposite fact is hold,
                                         // thanks to that we have only two masks for valid case.
            const int c = V | a | b;     // Combine all the masks into the final one.

            if (c == 0xffff)
                // The whole input register contains valid UTF16, i.e.,
                // either single words or proper surrogates.
                input += 8;
            else if (c == 0x7fff)
                // The 7 lower words of the input register contains valid UTF16.
                // The 7th word may be either a low or high surrogate. It the next
                // iteration we 1) check if a low surrogate is followed by high
                // one, 2) reject sole hight surrogate.
                input += 7;
            else
                return nullptr;
        }
    }

    return input;
}
