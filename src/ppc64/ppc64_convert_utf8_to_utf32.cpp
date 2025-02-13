// depends on "tables/utf8_to_utf16_tables.h"

// Convert up to 12 bytes from utf8 to utf32 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
size_t convert_masked_utf8_to_utf32(const char *input,
                                    uint64_t utf8_end_of_code_point_mask,
                                    char32_t *&utf32_output) {
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
    in.store_bytes_as_utf32(utf32_output);
    utf32_output += 12; // We wrote 12 32-bit characters.
    return 12;          // We consumed 12 bytes.
  }
  if (((utf8_end_of_code_point_mask & 0xffff) == 0xaaaa)) {
    // We want to take 8 2-byte UTF-8 code units and turn them into 8 4-byte
    // UTF-32 code units. There is probably a more efficient sequence, but the
    // following might do.
    const auto perm = as_vector_u16(in);
    const auto ascii = perm & uint16_t(0x007f);
    const auto highbyte = perm & uint16_t(0x1f00);
    const auto composed = ascii | highbyte.shr<2>();

    const auto composed8 = as_vector_u8(composed);
    composed8.store_words_as_utf32(utf32_output);

    utf32_output += 8; // We wrote 32 bytes, 8 code points.
    return 16;
  }
  if (input_utf8_end_of_code_point_mask == 0x924) {
    // We want to take 4 3-byte UTF-8 code units and turn them into 4 4-byte
    // UTF-32 code units. There is probably a more efficient sequence, but the
    // following might do.
    const auto sh =
        vector_u8(2, 1, 0, -1, 5, 4, 3, -1, 8, 7, 6, -1, 11, 10, 9, -1);
    const auto perm =
        as_vector_u32(sh.lookup_32(in, vector_u8::zero())).swap_bytes();
    const auto ascii = perm & uint32_t(0x0000007f);
    const auto middlebyte = perm & uint32_t(0x00003f00);
    const auto middlebyte_shifted = middlebyte.shr<2>();
    const auto highbyte = perm & uint32_t(0x0f0000);
    const auto highbyte_shifted = highbyte.shr<4>();
    const auto composed = ascii | middlebyte_shifted | highbyte_shifted;

    composed.store(utf32_output);
    utf32_output += 4;
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
    const auto perm =
        as_vector_u16(sh.lookup_32(in, vector_u8::zero())).swap_bytes();
    const auto ascii = perm & uint16_t(0x7f);
    const auto highbyte = perm & uint16_t(0x1f00);
    const auto composed = ascii | highbyte.shr<2>();

    as_vector_u8(composed).store_words_as_utf32(utf32_output);
    utf32_output += 6; // We wrote 12 bytes, 6 code points.
  } else if (idx < 145) {
    // FOUR (4) input code-code units
    const auto sh = vector_u8::load(&tables::utf8_to_utf16::shufutf8[idx]);
    const auto perm =
        as_vector_u32(sh.lookup_32(in, vector_u8::zero())).swap_bytes();
    const auto ascii = perm & uint32_t(0x7f);
    const auto middlebyte = perm & uint32_t(0x3f00);
    const auto middlebyte_shifted = middlebyte.shr<2>();
    const auto highbyte = perm & uint32_t(0x0f0000);
    const auto highbyte_shifted = highbyte.shr<4>();
    const auto composed = ascii | middlebyte_shifted | highbyte_shifted;

    composed.store(utf32_output);
    utf32_output += 4;
  } else if (idx < 209) {
    // TWO (2) input code-code units
    const auto sh = vector_u8::load(&tables::utf8_to_utf16::shufutf8[idx]);
    const auto perm =
        as_vector_u32(sh.lookup_32(in, vector_u8::zero())).swap_bytes();
    const auto ascii = perm & uint32_t(0x0000007f);
    const auto middlebyte = perm & uint32_t(0x3f00);
    const auto middlebyte_shifted = middlebyte.shr<2>();
    auto middlehighbyte = perm & uint32_t(0x003f0000);
    // correct for spurious high bit
    const auto correct0 = perm & uint32_t(0x00400000);
    const auto correct = correct0.shr<1>();
    middlehighbyte = correct ^ middlehighbyte;
    const auto middlehighbyte_shifted = middlehighbyte.shr<4>();
    const auto highbyte = perm & uint32_t(0x07000000);
    const auto highbyte_shifted = highbyte.shr<6>();
    const auto composed =
        ascii | middlebyte_shifted | highbyte_shifted | middlehighbyte_shifted;
    composed.store(utf32_output);
    utf32_output += 3;
  } else {
    // here we know that there is an error but we do not handle errors
  }
  return consumed;
}
