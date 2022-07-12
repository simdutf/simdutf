
const char16_t* arm_validate_utf16le(const char16_t* input, size_t size) {
    const char16_t* end = input + size;
    const auto v_d8 = simd8<uint8_t>::splat(0xd8);
    const auto v_f8 = simd8<uint8_t>::splat(0xf8);
    const auto v_fc = simd8<uint8_t>::splat(0xfc);
    const auto v_dc = simd8<uint8_t>::splat(0xdc);
    while (input + 16 < end) {
        // 0. Load data: since the validation takes into account only higher
        //    byte of each word, we compress the two vectors into one which
        //    consists only the higher bytes.
        const auto in0 = simd16<uint16_t>(input);
        const auto in1 = simd16<uint16_t>(input + simd16<uint16_t>::SIZE / sizeof(char16_t));
        const auto t0 = in0.shr<8>();
        const auto t1 = in1.shr<8>();
        const simd8<uint8_t> in = simd16<uint16_t>::pack(t0, t1);
        // 1. Check whether we have any 0xD800..DFFF word (0b1101'1xxx'yyyy'yyyy).
        const uint64_t surrogates_wordmask = ((in & v_f8) == v_d8).to_bitmask64();
        if(surrogates_wordmask == 0) {
            input += 16;
        } else {
            // 2. We have some surrogates that have to be distinguished:
            //    - low  surrogates: 0b1101'10xx'yyyy'yyyy (0xD800..0xDBFF)
            //    - high surrogates: 0b1101'11xx'yyyy'yyyy (0xDC00..0xDFFF)
            //
            //    Fact: high surrogate has 11th bit set (3rd bit in the higher word)

            // V - non-surrogate words
            //     V = not surrogates_wordmask
            const uint64_t V = ~surrogates_wordmask;

            // H - word-mask for high surrogates: the six highest bits are 0b1101'11
            const auto vH = ((in & v_fc) ==  v_dc);
            const uint64_t H = vH.to_bitmask64();

            // L - word mask for low surrogates
            //     L = not H and surrogates_wordmask
            const uint64_t L = ~H & surrogates_wordmask;

            const uint64_t a = L & (H >> 4); // A low surrogate must be followed by high one.
                              // (A low surrogate placed in the 7th register's word
                              // is an exception we handle.)
            const uint64_t b = a << 4; // Just mark that the opposite fact is hold,
                          // thanks to that we have only two masks for valid case.
            const uint64_t c = V | a | b;      // Combine all the masks into the final one.
            if (c == ~0ull) {
                // The whole input register contains valid UTF-16, i.e.,
                // either single words or proper surrogate pairs.
                input += 16;
            } else if (c == 0xfffffffffffffffull) {
                // The 15 lower words of the input register contains valid UTF-16.
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
