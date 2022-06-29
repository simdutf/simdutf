template<class checker>
// len is known to be a multiple of 2 when this is called
int sse_detect_encodings(const char * buf, size_t len) {
    const char* start = buf;
    const char* end = buf + len;

    bool is_utf8 = true;
    bool is_utf16 = true;
    bool is_utf32 = true;

    int out = 0;

    const auto v_d8 = simd8<uint8_t>::splat(0xd8);
    const auto v_f8 = simd8<uint8_t>::splat(0xf8);

    __m128i currentmax = _mm_setzero_si128();

    checker check{};

    while(buf + 64 <= end) {
        __m128i in = _mm_loadu_si128((__m128i*)buf);
        __m128i secondin = _mm_loadu_si128((__m128i*)buf+1);
        __m128i thirdin = _mm_loadu_si128((__m128i*)buf+2);
        __m128i fourthin = _mm_loadu_si128((__m128i*)buf+3);

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
        uint16_t surrogates_bitmask0 = static_cast<uint16_t>(surrogates_wordmask0.to_bitmask());
        uint16_t surrogates_bitmask1 = static_cast<uint16_t>(surrogates_wordmask1.to_bitmask());

        // Check for surrogates
        if (surrogates_bitmask0 != 0x0 || surrogates_bitmask1 != 0x0) {
            // Cannot be UTF8
            is_utf8 = false;
            // Can still be either UTF-16LE or UTF-32LE depending on the positions of the surrogates
            // To be valid UTF-32LE, a surrogate cannot be in the two most significant bytes of any 32-bit word.
            // On the other hand, to be valid UTF-16LE, at least one surrogate must be in the two most significant
            // bytes of a 32-bit word since they always come in pairs in UTF-16LE.
            // Note that we always proceed in multiple of 4 before this point so there is no offset in 32-bit words.

            if (((surrogates_bitmask0 | surrogates_bitmask1) & 0xaaaa) != 0) {
                is_utf32 = false;
                // Code from sse_validate_utf16le.cpp
                // Not efficient, we do not process surrogates_bitmask1
                const char16_t * input = reinterpret_cast<const char16_t*>(buf);
                const char16_t* end16 = reinterpret_cast<const char16_t*>(start) + len/2;

                const auto v_fc = simd8<uint8_t>::splat(0xfc);
                const auto v_dc = simd8<uint8_t>::splat(0xdc);

                const uint16_t V0 = static_cast<uint16_t>(~surrogates_bitmask0);

                const auto    vH0 = (in16 & v_fc) == v_dc;
                const uint16_t H0 = static_cast<uint16_t>(vH0.to_bitmask());

                const uint16_t L0 = static_cast<uint16_t>(~H0 & surrogates_bitmask0);

                const uint16_t a0 = static_cast<uint16_t>(L0 & (H0 >> 1));

                const uint16_t b0 = static_cast<uint16_t>(a0 << 1);

                const uint16_t c0 = static_cast<uint16_t>(V0 | a0 | b0);

                if (c0 == 0xffff) {
                    input += 16;
                } else if (c0 == 0x7fff) {
                    input += 15;
                } else {
                    is_utf16 = false;
                    break;
                }

                while (input + simd16<uint16_t>::SIZE * 2 < end16) {
                    const auto in0 = simd16<uint16_t>(input);
                    const auto in1 = simd16<uint16_t>(input + simd16<uint16_t>::SIZE / sizeof(char16_t));

                    const auto t0 = in0.shr<8>();
                    const auto t1 = in1.shr<8>();

                    const auto in_16 = simd16<uint16_t>::pack(t0, t1);

                    const auto surrogates_wordmask = (in_16 & v_f8) == v_d8;
                    const uint16_t surrogates_bitmask = static_cast<uint16_t>(surrogates_wordmask.to_bitmask());
                    if (surrogates_bitmask == 0x0) {
                        input += 16;
                    } else {
                        const uint16_t V = static_cast<uint16_t>(~surrogates_bitmask);

                        const auto    vH = (in_16 & v_fc) == v_dc;
                        const uint16_t H = static_cast<uint16_t>(vH.to_bitmask());

                        const uint16_t L = static_cast<uint16_t>(~H & surrogates_bitmask);

                        const uint16_t a = static_cast<uint16_t>(L & (H >> 1));

                        const uint16_t b = static_cast<uint16_t>(a << 1);

                        const uint16_t c = static_cast<uint16_t>(V | a | b);

                        if (c == 0xffff) {
                            input += 16;
                        } else if (c == 0x7fff) {
                            input += 15;
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
                    __m128i currentoffsetmax = _mm_setzero_si128();
                    const __m128i offset = _mm_set1_epi32(0xffff2000);
                    const __m128i standardoffsetmax = _mm_set1_epi32(0xfffff7ff);

                    currentmax = _mm_max_epu32(in, currentmax);
                    currentmax = _mm_max_epu32(secondin, currentmax);
                    currentmax = _mm_max_epu32(thirdin, currentmax);
                    currentmax = _mm_max_epu32(fourthin, currentmax);

                    currentoffsetmax = _mm_max_epu32(_mm_add_epi32(in, offset), currentoffsetmax);
                    currentoffsetmax = _mm_max_epu32(_mm_add_epi32(secondin, offset), currentoffsetmax);
                    currentoffsetmax = _mm_max_epu32(_mm_add_epi32(thirdin, offset), currentoffsetmax);
                    currentoffsetmax = _mm_max_epu32(_mm_add_epi32(fourthin, offset), currentoffsetmax);

                    while (input + 4 < end32) {
                        const __m128i in32 = _mm_loadu_si128((__m128i *)input);
                        currentmax = _mm_max_epu32(in32,currentmax);
                        currentoffsetmax = _mm_max_epu32(_mm_add_epi32(in32, offset), currentoffsetmax);
                        input += 4;
                    }

                    __m128i forbidden_words = _mm_xor_si128(_mm_max_epu32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
                    if(_mm_testz_si128(forbidden_words, forbidden_words) == 0) {
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
        currentmax = _mm_max_epu32(in, currentmax);
        currentmax = _mm_max_epu32(secondin, currentmax);
        currentmax = _mm_max_epu32(thirdin, currentmax);
        currentmax = _mm_max_epu32(fourthin, currentmax);

        // UTF-8 validation
        // Relies on ../generic/utf8_validation/utf8_lookup4_algorithm.h
        simd::simd8x64<uint8_t> in8(in, secondin, thirdin, fourthin);
        check.check_next_input(in8);

        buf += 64;
    }

    // Check which encodings are possible

    if (is_utf8 && !check.errors()) {
        if (scalar::utf8::validate(buf, len - (buf - start))) {
            out |= simdutf::encoding_type::UTF8;
        }
    }

    if (is_utf16 && scalar::utf16::validate(reinterpret_cast<const char16_t*>(buf), (len - (buf - start))/2)) {
        out |= simdutf::encoding_type::UTF16_LE;
    }

    if (is_utf32) {
        const __m128i standardmax = _mm_set1_epi32(0x10ffff);
        __m128i is_zero = _mm_xor_si128(_mm_max_epu32(currentmax, standardmax), standardmax);
        if (_mm_testz_si128(is_zero, is_zero) == 1 && scalar::utf32::validate(reinterpret_cast<const char32_t*>(buf), (len - (buf - start))/4)) {
            out |= simdutf::encoding_type::UTF32_LE;
        }
    }

    return out;
}