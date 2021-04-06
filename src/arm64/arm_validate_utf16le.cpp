// Surrogate pairs:
// 0xD800–0xDBFF.
// 0xDC00–0xDFFF.

//  port of sse_validate_utf16le with NEON adaptations
const char16_t* arm_validate_utf16le(const char16_t* input, size_t size) {
    const char16_t* end = input + size;

    const uint8x16_t v_d8 = vmovq_n_u8(uint8_t(0xd8));
    const uint8x16_t v_f8 = vmovq_n_u8(uint8_t(0xf8));
    const uint8x16_t v_fc = vmovq_n_u8(uint8_t(0xfc));
    const uint8x16_t v_dc = vmovq_n_u8(uint8_t(0xdc));

    while (input + 16 < end) {
        // 0. Load data: since the validation takes into account only higher
        //    byte of each word, we compress the two vectors into one which
        //    consists only the higher bytes.
        static_assert(sizeof(char16_t) == sizeof(uint16_t), "unexpected char16_t sizeof");
        const uint16x8_t in0 = vld1q_u16(reinterpret_cast<const uint16_t*>(input) + 0*8);
        const uint16x8_t in1 = vld1q_u16(reinterpret_cast<const uint16_t*>(input) + 1*8);
        const uint16x8_t t0  = vshrq_n_u16(in0, 8);
        const uint16x8_t t1  = vshrq_n_u16(in1, 8);
        const uint8x16_t in = vqmovn_high_u16(vqmovn_u16(t0), t1);
        // 1. Check whether we have any 0xD800..DFFF word (0b1101'1xxx'yyyy'yyyy).
        uint8x16_t surrogates_wordmask = vceqq_u8(vandq_u8(in, v_f8),v_d8);
        // NEON implementation note. We do not have a fast movmask so we have
        // to do a bit of reengineering.
        if (vmaxvq_u8(surrogates_wordmask) == 0) {
            input += 16;
        } else {
            uint8x16_t vH = vceqq_u8(vandq_u8(in, v_fc),v_dc);
            uint8x16_t vL = vbicq_u8(surrogates_wordmask, vH);
            // We are going to need these later:
            uint8_t low_vh = vgetq_lane_u8(vH,0);
            uint8_t high_vl = vgetq_lane_u8(vL,15);

            // We shift vH down, possibly killing low_vh
            const uint8x16_t sh = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0xFF};
            uint8x16_t vHshifteddown = vqtbl1q_u8(vH, sh);
            uint8x16_t match = vceqq_u8(vHshifteddown, vL);
            // We need to handle the fact that high_vl is unmatched.
            // We could use this...
            // const uint8x16_t allbutlast = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xFF};
            //             match = vorrq_u8(match, allbutlast);
            // but sh will do:
            match = vorrq_u8(match, sh);
            // We deliberately take these two lines out of the following branchy code
            // so that they are always s
            if (vminvq_u8(match) == 0xFF && low_vh == 0) {
                input += (high_vl == 0) ? 16 : 15;
            } else {
                return nullptr;
            }
        }
    }

    return input;
}
