template<class checker>
// len is known to be a multiple of 2 when this is called
int avx2_detect_encodings(const char * buf, size_t len) {
    const char* start = buf;
    const char* end = buf + len;

    bool is_utf8 = true;
    bool is_utf16 = true;
    bool is_utf32 = true;

    int out = 0;

    const auto v_d8 = simd8<uint8_t>::splat(0xd8);
    const auto v_f8 = simd8<uint8_t>::splat(0xf8);

    __m256i currentmax = _mm256_setzero_si256();

    checker check{};

    while(buf + 64 <= end) {
        __m256i in = _mm256_loadu_si256((__m256i*)buf);
        __m256i nextin = _mm256_loadu_si256((__m256i*)buf+1);

        const auto u0 = simd16<uint16_t>(in);
        const auto u1 = simd16<uint16_t>(nextin);

        const auto v0 = u0.shr<8>();
        const auto v1 = u1.shr<8>();

        const auto in16 = simd16<uint16_t>::pack(v0, v1);

        const auto surrogates_wordmask0 = (in16 & v_f8) == v_d8;
        uint32_t surrogates_bitmask0 = surrogates_wordmask0.to_bitmask();

        // Check for surrogates
        if (surrogates_bitmask0 != 0x0) {
            // Cannot be UTF8
            is_utf8 = false;
            // Can still be either UTF-16LE or UTF-32LE depending on the positions of the surrogates
            // To be valid UTF-32LE, a surrogate cannot be in the two most significant bytes of any 32-bit word.
            // On the other hand, to be valid UTF-16LE, at least one surrogate must be in the two most significant
            // bytes of a 32-bit word since they always come in pairs in UTF-16LE.
            // Note that we always proceed in multiple of 4 before this point so there is no offset in 32-bit words.

            if ((surrogates_bitmask0 & 0xaaaaaaaa) != 0) {
                is_utf32 = false;
                // Code from avx2_validate_utf16le.cpp
                const char16_t * input = reinterpret_cast<const char16_t*>(buf);
                const char16_t* end16 = reinterpret_cast<const char16_t*>(start) + len/2;

                const auto v_fc = simd8<uint8_t>::splat(0xfc);
                const auto v_dc = simd8<uint8_t>::splat(0xdc);

                const uint32_t V0 = ~surrogates_bitmask0;

                const auto    vH0 = (in16 & v_fc) == v_dc;
                const uint32_t H0 = vH0.to_bitmask();

                const uint32_t L0 = ~H0 & surrogates_bitmask0;

                const uint32_t a0 = L0 & (H0 >> 1);
                const uint32_t b0 = a0 << 1;
                const uint32_t c0 = V0 | a0 | b0;

                if (c0 == 0xffffffff) {
                    input += simd16<uint16_t>::ELEMENTS * 2;
                } else if (c0 == 0x7fffffff) {
                    input += simd16<uint16_t>::ELEMENTS * 2 - 1;
                } else {
                    return simdutf::encoding_type::unspecified;
                }

                while (input + simd16<uint16_t>::ELEMENTS * 2 < end16) {
                    const auto in0 = simd16<uint16_t>(input);
                    const auto in1 = simd16<uint16_t>(input + simd16<uint16_t>::ELEMENTS);

                    const auto t0 = in0.shr<8>();
                    const auto t1 = in1.shr<8>();

                    const auto in_16 = simd16<uint16_t>::pack(t0, t1);

                    const auto surrogates_wordmask = (in_16 & v_f8) == v_d8;
                    const uint32_t surrogates_bitmask = surrogates_wordmask.to_bitmask();
                    if (surrogates_bitmask == 0x0) {
                        input += simd16<uint16_t>::ELEMENTS * 2;
                    } else {
                        const uint32_t V = ~surrogates_bitmask;

                        const auto    vH = (in_16 & v_fc) == v_dc;
                        const uint32_t H = vH.to_bitmask();

                        const uint32_t L = ~H & surrogates_bitmask;

                        const uint32_t a = L & (H >> 1);

                        const uint32_t b = a << 1;

                        const uint32_t c = V | a | b;

                        if (c == 0xffffffff) {
                            input += simd16<uint16_t>::ELEMENTS * 2;
                        } else if (c == 0x7fffffff) {
                            input += simd16<uint16_t>::ELEMENTS * 2 - 1;
                        } else {
                            return simdutf::encoding_type::unspecified;
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
                    __m256i currentoffsetmax = _mm256_setzero_si256();
                    const __m256i offset = _mm256_set1_epi32(0xffff2000);
                    const __m256i standardoffsetmax = _mm256_set1_epi32(0xfffff7ff);

                    currentmax = _mm256_max_epu32(in, currentmax);
                    currentmax = _mm256_max_epu32(nextin, currentmax);

                    currentoffsetmax = _mm256_max_epu32(_mm256_add_epi32(in, offset), currentoffsetmax);
                    currentoffsetmax = _mm256_max_epu32(_mm256_add_epi32(nextin, offset), currentoffsetmax);

                    while (input + 8 < end32) {
                        const __m256i in32 = _mm256_loadu_si256((__m256i *)input);
                        currentmax = _mm256_max_epu32(in32,currentmax);
                        currentoffsetmax = _mm256_max_epu32(_mm256_add_epi32(in32, offset), currentoffsetmax);
                        input += 8;
                    }

                    __m256i forbidden_words = _mm256_xor_si256(_mm256_max_epu32(currentoffsetmax, standardoffsetmax), standardoffsetmax);
                    if(_mm256_testz_si256(forbidden_words, forbidden_words) == 0) {
                        return simdutf::encoding_type::unspecified;
                    }
                } else {
                    return simdutf::encoding_type::unspecified;
                }
            }
            break;
        }
        // If no surrogate, validate under other encodings as well

        // UTF-32LE validation
        currentmax = _mm256_max_epu32(in, currentmax);
        currentmax = _mm256_max_epu32(nextin, currentmax);

        // UTF-8 validation
        // Relies on ../generic/utf8_validation/utf8_lookup4_algorithm.h
        simd::simd8x64<uint8_t> in8(in, nextin);
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
        const __m256i standardmax = _mm256_set1_epi32(0x10ffff);
        __m256i is_zero = _mm256_xor_si256(_mm256_max_epu32(currentmax, standardmax), standardmax);
        if (_mm256_testz_si256(is_zero, is_zero) == 1 && scalar::utf32::validate(reinterpret_cast<const char32_t*>(buf), (len - (buf - start))/4)) {
            out |= simdutf::encoding_type::UTF32_LE;
        }
    }

    return out;
}