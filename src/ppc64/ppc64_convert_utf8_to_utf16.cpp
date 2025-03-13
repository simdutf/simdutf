// depends on "tables/utf8_to_utf16_tables.h"

// Convert up to 12 bytes from utf8 to utf16 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
template <endianness big_endian>
size_t convert_masked_utf8_to_utf16(const char *input,
                                    uint64_t utf8_end_of_code_point_mask,
                                    char16_t *&utf16_output) {
  // we use an approach where we try to process up to 12 input bytes.
  // Why 12 input bytes and not 16? Because we are concerned with the size of
  // the lookup tables. Also 12 is nicely divisible by two and three.
  //
  //
  // Optimization note: our main path below is load-latency dependent. Thus it
  // is maybe beneficial to have fast paths that depend on branch prediction but
  // have less latency. This results in more instructions but, potentially, also
  // higher speeds.
  //
  // We first try a few fast paths.
  const auto in = vector_u8::load(input);
  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask & 0xfff;
  if (utf8_end_of_code_point_mask == 0xfff) {
    // We process the data in chunks of 12 bytes.
    // Note: using 16 bytes is unsafe, see issue_ossfuzz_71218
    in.store_bytes_as_utf16<big_endian>(utf16_output);
    utf16_output += 12; // We wrote 12 16-bit characters.
    return 12;          // We consumed 12 bytes.
  }
  if (((utf8_end_of_code_point_mask & 0xFFFF) == 0xaaaa)) {
    // We want to take 8 2-byte UTF-8 code units and turn them into 8 2-byte
    // UTF-16 code units.
#if SIMDUTF_IS_BIG_ENDIAN
    const auto in16 = as_vector_u16(in);
#else
    const auto in16 = as_vector_u16(in).swap_bytes();
#endif // SIMDUTF_IS_BIG_ENDIAN
    const auto lo = in16 & uint16_t(0x007f);
    const auto hi = in16.shr<2>();

    auto composed = select(uint16_t(0x1f00 >> 2), hi, lo);
    if (!match_system(big_endian)) {
      composed = composed.swap_bytes();
    }

    composed.store(utf16_output);
    utf16_output += 8; // We wrote 16 bytes, 8 code points.
    return 16;
  }
  if (input_utf8_end_of_code_point_mask == 0x924) {
    // We want to take 4 3-byte UTF-8 code units and turn them into 4 2-byte
    // UTF-16 code units. There is probably a more efficient sequence, but the
    // following might do.

    // AltiVec: it might be done better, for now SSE translation

    const auto sh =
        vector_u8(2, 1, 0, 16, 5, 4, 3, 16, 8, 7, 6, 16, 11, 10, 9, 16);
#if SIMDUTF_IS_BIG_ENDIAN
    const auto perm =
        as_vector_u32(sh.lookup_32(in, vector_u8::zero())).swap_bytes();
#else
    const auto perm = as_vector_u32(sh.lookup_32(in, vector_u8::zero()));
#endif // SIMDUTF_IS_BIG_ENDIAN
    const auto b0 = perm & uint32_t(0x0000007f);
    const auto b1 = select(uint32_t(0x00003f00 >> 2), perm.shr<2>(), b0);
    const auto b2 = select(uint32_t(0x000f0000 >> 4), perm.shr<4>(), b1);
    const auto composed = b2;
    auto packed = vector_u32::pack(composed, composed);

    if (!match_system(big_endian)) {
      packed = packed.swap_bytes();
    }

    packed.store(utf16_output);
    utf16_output += 4;
    return 12;
  }
  /// We do not have a fast path available, so we fallback.

  const uint8_t idx =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][0];
  const uint8_t consumed =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][1];

  if (idx < 64) {
    // SIX (6) input code-code units
    // this is a relatively easy scenario
    // we process SIX (6) input code-code units. The max length in bytes of six
    // code code units spanning between 1 and 2 bytes each is 12 bytes. On
    // processors where pdep/pext is fast, we might be able to use a small
    // lookup table.
    const auto sh = vector_u8::load(&tables::utf8_to_utf16::shufutf8[idx]);
#if SIMDUTF_IS_BIG_ENDIAN
    const auto perm =
        as_vector_u16(sh.lookup_32(in, vector_u8::zero())).swap_bytes();
#else
    const auto perm = as_vector_u16(sh.lookup_32(in, vector_u8::zero()));
#endif // SIMDUTF_IS_BIG_ENDIAN
    const auto b0 = perm & uint16_t(0x007f);
    const auto b1 = perm & uint16_t(0x1f00);

    auto composed = b0 | b1.shr<2>();

    if (!match_system(big_endian)) {
      composed = composed.swap_bytes();
    }

    composed.store(utf16_output);
    utf16_output += 6; // We wrote 12 bytes, 6 code points.
  } else if (idx < 145) {
    // FOUR (4) input code-code units
    const auto sh = vector_u8::load(&tables::utf8_to_utf16::shufutf8[idx]);
#if SIMDUTF_IS_BIG_ENDIAN
    const auto perm =
        as_vector_u32(sh.lookup_32(in, vector_u8::zero())).swap_bytes();
#else
    const auto perm = as_vector_u32(sh.lookup_32(in, vector_u8::zero()));
#endif // SIMDUTF_IS_BIG_ENDIAN
    const auto b0 = perm & uint32_t(0x0000007f);
    const auto b1 = perm & uint32_t(0x00003f00);
    const auto b2 = perm & uint32_t(0x000f0000);

    const auto composed = b0 | b1.shr<2>() | b2.shr<4>();

    auto packed = vector_u32::pack(composed, composed);

    if (!match_system(big_endian)) {
      packed = packed.swap_bytes();
    }

    packed.store(utf16_output);
    utf16_output += 4;
  } else if (idx < 209) {
    // TWO (2) input code-code units
    //////////////
    // There might be garbage inputs where a leading byte mascarades as a
    // four-byte leading byte (by being followed by 3 continuation byte), but is
    // not greater than 0xf0. This could trigger a buffer overflow if we only
    // counted leading bytes of the form 0xf0 as generating surrogate pairs,
    // without further UTF-8 validation. Thus we must be careful to ensure that
    // only leading bytes at least as large as 0xf0 generate surrogate pairs. We
    // do as at the cost of an extra mask.
    /////////////
    const auto sh = vector_u8::load(&tables::utf8_to_utf16::shufutf8[idx]);
#if SIMDUTF_IS_BIG_ENDIAN
    const auto perm =
        as_vector_u32(sh.lookup_32(in, vector_u8::zero())).swap_bytes();
#else
    const auto perm = as_vector_u32(sh.lookup_32(in, vector_u8::zero()));
#endif // SIMDUTF_IS_BIG_ENDIAN
    const auto ascii = perm & uint32_t(0x00000007f);
    const auto middlebyte = perm & uint32_t(0x00003f00);
    const auto middlebyte_shifted = middlebyte.shr<2>();

    auto middlehighbyte = perm & uint32_t(0x003f0000);
    // correct for spurious high bit

    const auto correct = (perm & uint32_t(0x00400000)).shr<1>();
    middlehighbyte = correct ^ middlehighbyte;
    const auto middlehighbyte_shifted = middlehighbyte.shr<4>();
    // We deliberately carry the leading four bits in highbyte if they are
    // present, we remove them later when computing hightenbits.
    const auto highbyte = perm & uint32_t(0xff000000);
    const auto highbyte_shifted = highbyte.shr<6>();
    // When we need to generate a surrogate pair (leading byte > 0xF0), then
    // the corresponding 32-bit value in 'composed'  will be greater than
    // > (0xff00000>>6) or > 0x3c00000. This can be used later to identify the
    // location of the surrogate pairs.
    const auto composed =
        ascii | middlebyte_shifted | highbyte_shifted | middlehighbyte_shifted;

    const auto composedminus = composed - uint32_t(0x10000);
    const auto lowtenbits = composedminus & uint32_t(0x3ff);
    // Notice the 0x3ff mask:
    const auto hightenbits = composedminus.shr<10>() & uint32_t(0x3ff);
    const auto lowtenbitsadd = lowtenbits + uint32_t(0xDC00);
    const auto hightenbitsadd = hightenbits + uint32_t(0xD800);
    const auto lowtenbitsaddshifted = lowtenbitsadd.shl<16>();
    auto surrogates = hightenbitsadd | lowtenbitsaddshifted;

    uint32_t basic_buffer[4];
    composed.store(basic_buffer);
    uint32_t surrogate_buffer[4];
    surrogates.swap_bytes().store(surrogate_buffer);

    for (size_t i = 0; i < 3; i++) {
      if (basic_buffer[i] > 0x3c00000) {
        const auto ch0 = uint16_t(surrogate_buffer[i] & 0xffff);
        const auto ch1 = uint16_t(surrogate_buffer[i] >> 16);
        if (match_system(big_endian)) {
          utf16_output[1] = scalar::u16_swap_bytes(ch0);
          utf16_output[0] = scalar::u16_swap_bytes(ch1);
        } else {
          utf16_output[1] = ch0;
          utf16_output[0] = ch1;
        }
        utf16_output += 2;
      } else {
        const auto chr = uint16_t(basic_buffer[i]);
        if (match_system(big_endian)) {
          utf16_output[0] = chr;
        } else {
          utf16_output[0] = scalar::u16_swap_bytes(chr);
        }

        utf16_output++;
      }
    }
  } else {
    // here we know that there is an error but we do not handle errors
  }
  return consumed;
}
