
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
        const auto surrogates_wordmask = ((in & v_f8) == v_d8);
        if(surrogates_wordmask.none()) {
            input += 16;
        } else {
            const auto vH = simd8<uint8_t>((in & v_fc) ==  v_dc);
            const auto vL = simd8<uint8_t>(surrogates_wordmask).bit_andnot(vH);
            // We are going to need these later:
            const uint8_t low_vh = vH.first();
            const uint8_t high_vl = vL.last();
            // We shift vH down, possibly killing low_vh
            const auto sh = simd8<uint8_t>({1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0xFF});
            const auto vHshifteddown = vH.apply_lookup_16_to(sh);
            const auto match = vHshifteddown == vL;
            // We need to handle the fact that high_vl is unmatched.
            // We could use this...
            // const uint8x16_t allbutlast = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xFF};
            //             match = vorrq_u8(match, allbutlast);
            // but sh will do:
            const auto fmatch = simd8<uint8_t>(match) | sh;
            // We deliberately take these two lines out of the following branchy code
            // so that they are always s
            if (fmatch.min_val() == 0xFF && low_vh == 0) {
                input += (high_vl == 0) ? 16 : 15;
            } else {
                return nullptr;
            }
        }
    }
    return input;
}
