template<class checker>
// len is known to be a multiple of 2 when this is called
int arm_detect_encodings(const char * buf, size_t len) {
    const char* start = buf;
    const char* end = buf + len;

    bool is_utf8 = true;
    bool is_utf16 = true;
    bool is_utf32 = true;

    int out = 0;

    const auto v_d8 = simd8<uint8_t>::splat(0xd8);
    const auto v_f8 = simd8<uint8_t>::splat(0xf8);

    uint32x4_t currentmax = vmovq_n_u32(0x0);

    checker check{};

    while(buf + 64 <= end) {
        uint16x8_t in = vld1q_u16(reinterpret_cast<const uint16_t*>(buf));
        uint16x8_t secondin = vld1q_u16(reinterpret_cast<const uint16_t*>(buf) + simd16<uint16_t>::SIZE / sizeof(char16_t));
        uint16x8_t thirdin = vld1q_u16(reinterpret_cast<const uint16_t*>(buf) + 2*simd16<uint16_t>::SIZE / sizeof(char16_t));
        uint16x8_t fourthin = vld1q_u16(reinterpret_cast<const uint16_t*>(buf) + 3*simd16<uint16_t>::SIZE / sizeof(char16_t));

        const auto u0 = simd16<uint16_t>(in);
        const auto u1 = simd16<uint16_t>(secondin);
        const auto u2 = simd16<uint16_t>(thirdin);
        const auto u3 = simd16<uint16_t>(fourthin);

        const auto v0 = u0.shr<8>();
        const auto v1 = u1.shr<8>();
        const auto v2 = u2.shr<8>();
        const auto v3 = u3.shr<8>();

        const auto in16 = simd16<uint16_t>::pack(v0, v1);
        const auto nextin16 = simd16<uint16_t>::pack(v2, v3);

        const auto surrogates_wordmask0 = (in16 & v_f8) == v_d8;
        const auto surrogates_wordmask1 = (nextin16 & v_f8) == v_d8;

        // Check for surrogates
        if (!surrogates_wordmask0.none() || !surrogates_wordmask1.none()) {
            // Cannot be UTF8
            is_utf8 = false;
            // Can still be either UTF-16LE or UTF-32LE depending on the positions of the surrogates
            // To be valid UTF-32LE, a surrogate cannot be in the two most significant bytes of any 32-bit word.
            // On the other hand, to be valid UTF-16LE, at least one surrogate must be in the two most significant
            // bytes of a 32-bit word since they always come in pairs in UTF-16LE.
            // Note that we always proceed in multiple of 4 before this point so there is no offset in 32-bit words.

            if (((surrogates_wordmask0.to_bitmask() | surrogates_wordmask1.to_bitmask()) & 0xaaaa) != 0) {
                is_utf32 = false;
                // Code from arm_validate_utf16le.cpp
                // Not efficient, we do not process surrogates_wordmask1
                const char16_t * input = reinterpret_cast<const char16_t*>(buf);
                const char16_t* end16 = reinterpret_cast<const char16_t*>(start) + len/2;

                const auto v_fc = simd8<uint8_t>::splat(0xfc);
                const auto v_dc = simd8<uint8_t>::splat(0xdc);

                const auto vH0 = simd8<uint8_t>((in16 & v_fc) ==  v_dc);
                const auto vL0 = simd8<uint8_t>(surrogates_wordmask0).bit_andnot(vH0);

                const uint8_t low_vh0 = vH0.first();
                const uint8_t high_vl0 = vL0.last();

                const auto sh0 = simd8<uint8_t>({1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0xFF});
                const auto vHshifteddown0 = vH0.apply_lookup_16_to(sh0);
                const auto match0 = vHshifteddown0 == vL0;

                const auto fmatch0 = simd8<bool>(simd8<uint8_t>(match0) | sh0);

                if (fmatch0.all() && low_vh0 == 0) {
                    input += (high_vl0 == 0) ? 16 : 15;
                } else {
                    is_utf16 = false;
                    break;
                }

                 while (input + 16 < end16) {
                    const auto in0 = simd16<uint16_t>(input);
                    const auto in1 = simd16<uint16_t>(input + simd16<uint16_t>::SIZE / sizeof(char16_t));
                    const auto t0 = in0.shr<8>();
                    const auto t1 = in1.shr<8>();
                    const simd8<uint8_t> in_16 = simd16<uint16_t>::pack(t0, t1);

                    const auto surrogates_wordmask = ((in_16 & v_f8) == v_d8);
                    if(surrogates_wordmask.none()) {
                        input += 16;
                    } else {
                        const auto vH = simd8<uint8_t>((in_16 & v_fc) ==  v_dc);
                        const auto vL = simd8<uint8_t>(surrogates_wordmask).bit_andnot(vH);

                        const uint8_t low_vh = vH.first();
                        const uint8_t high_vl = vL.last();

                        const auto sh = simd8<uint8_t>({1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0xFF});
                        const auto vHshifteddown = vH.apply_lookup_16_to(sh);
                        const auto match = vHshifteddown == vL;

                        const auto fmatch = simd8<bool>(simd8<uint8_t>(match) | sh);

                        if (fmatch.all() && low_vh == 0) {
                            input += (high_vl == 0) ? 16 : 15;
                        } else {
                            is_utf16 = false;
                            break;
                        }
                    }
                }
            } else {
                is_utf16 = false;
                // Check for UTF-32LE
                if (len % 4 == 0) {
                    const char32_t * input = reinterpret_cast<const char32_t*>(buf);
                    const char32_t* end32 = reinterpret_cast<const char32_t*>(start) + len/4;

                    // Must start checking for surrogates
                    uint32x4_t currentoffsetmax = vmovq_n_u32(0x0);
                    const uint32x4_t offset = vmovq_n_u32(0xffff2000);
                    const uint32x4_t standardoffsetmax = vmovq_n_u32(0xfffff7ff);

                    const uint32x4_t in32 =  vreinterpretq_u32_u16(in);
                    const uint32x4_t secondin32 =  vreinterpretq_u32_u16(secondin);
                    const uint32x4_t thirdin32 =  vreinterpretq_u32_u16(thirdin);
                    const uint32x4_t fourthin32 =  vreinterpretq_u32_u16(fourthin);

                    currentmax = vmaxq_u32(in32,currentmax);
                    currentmax = vmaxq_u32(secondin32,currentmax);
                    currentmax = vmaxq_u32(thirdin32,currentmax);
                    currentmax = vmaxq_u32(fourthin32,currentmax);

                    currentoffsetmax = vmaxq_u32(vaddq_u32(in32, offset), currentoffsetmax);
                    currentoffsetmax = vmaxq_u32(vaddq_u32(secondin32, offset), currentoffsetmax);
                    currentoffsetmax = vmaxq_u32(vaddq_u32(thirdin32, offset), currentoffsetmax);
                    currentoffsetmax = vmaxq_u32(vaddq_u32(fourthin32, offset), currentoffsetmax);

                    while (input + 4 < end32) {
                        const uint32x4_t in_32 = vld1q_u32(reinterpret_cast<const uint32_t*>(input));
                        currentmax = vmaxq_u32(in_32,currentmax);
                        currentoffsetmax = vmaxq_u32(vaddq_u32(in_32, offset), currentoffsetmax);
                        input += 4;
                    }

                    uint32x4_t forbidden_words = veorq_u32(vmaxq_u32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
                    if(vmaxvq_u32(forbidden_words) != 0) {
                        is_utf32 = false;
                    }
                } else {
                    is_utf32 = false;
                }
            }
            break;
        }
        // If no surrogate, validate under other encodings as well

        // UTF-32LE validation
        currentmax = vmaxq_u32(vreinterpretq_u32_u16(in),currentmax);
        currentmax = vmaxq_u32(vreinterpretq_u32_u16(secondin),currentmax);
        currentmax = vmaxq_u32(vreinterpretq_u32_u16(thirdin),currentmax);
        currentmax = vmaxq_u32(vreinterpretq_u32_u16(fourthin),currentmax);

        // UTF-8 validation
        // Relies on ../generic/utf8_validation/utf8_lookup4_algorithm.h
        simd::simd8x64<uint8_t> in8(vreinterpretq_u8_u16(in), vreinterpretq_u8_u16(secondin), vreinterpretq_u8_u16(thirdin), vreinterpretq_u8_u16(fourthin));
        check.check_next_input(in8);

        buf += 64;
    }

    // Check which encodings are possible

    if (is_utf8) {
        if (static_cast<size_t>(buf - start) != len) {
            uint8_t block[64]{};
            std::memset(block, 0x20, 64);
            std::memcpy(block, buf, len - (buf - start));
            simd::simd8x64<uint8_t> in(block);
            check.check_next_input(in);
        }
        if (!check.errors()) {
            out |= simdutf::encoding_type::UTF8;
        }
    }

    if (is_utf16 && scalar::utf16::validate(reinterpret_cast<const char16_t*>(buf), (len - (buf - start))/2)) {
        out |= simdutf::encoding_type::UTF16_LE;
    }

    if (is_utf32 && (len % 4 == 0)) {
        const uint32x4_t standardmax = vmovq_n_u32(0x10ffff);
        uint32x4_t is_zero = veorq_u32(vmaxq_u32(currentmax, standardmax), standardmax);
        if (vmaxvq_u32(is_zero) == 0 && scalar::utf32::validate(reinterpret_cast<const char32_t*>(buf), (len - (buf - start))/4)) {
            out |= simdutf::encoding_type::UTF32_LE;
        }
    }

    return out;
}