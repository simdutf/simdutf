using vecu8 = __vector uint8_t;
using vecu16 = __vector uint16_t;
using vecu32 = __vector uint32_t;

// Convert up to 12 bytes from utf8 to utf16 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
size_t convert_masked_utf8_to_utf16(const char *input,
                           uint64_t utf8_end_of_code_point_mask,
                           char16_t *&utf16_output) {
  // We first try a few fast paths.
  const vecu8 in = (vecu8)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(values)));
  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask & 0xFFF;
  if(((utf8_end_of_code_point_mask & 0xFFFF) == 0xFFFF)) {
    // We process the data in chunks of 16 bytes.
    vec_mergel(in, );
    vec_vsx_st(vec_mergel(in, vec_splat_u16(0)), 0, reinterpret_cast<vecu8 *>(utf16_output));
    vec_vsx_st(vec_mergeh(in, vec_splat_u16(0)), 0, reinterpret_cast<vecu8 *>(utf16_output + 8));
    utf16_output += 16; // We wrote 16 16-bit characters.
    return 16; // We consumed 16 bytes.
  }
  if(((utf8_end_of_code_point_mask & 0xFFFF) == 0xaaaa)) {
    // We want to take 8 2-byte UTF-8 words and turn them into 8 2-byte UTF-16 words.
    // There is probably a more efficient sequence, but the following might do.
    const vecu8 sh = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14};
    const vecu8 perm = vec_perm(in, in, sh);
    const vecu8 ascii = vec_and(perm, vec_splat_u16(0x7f));
    const vecu16 highbyte = vec_and(perm, vec_splat_u16(0x1f00));
    const vecu16 composed = vec_or(ascii, vec_srl(highbyte, 2));
    vec_vsx_st(composed, 0, reinterpret_cast<vecu8 *>(utf16_output));
    utf16_output += 8; // We wrote 16 bytes, 8 code points.
    return 16;
  }
  if(input_utf8_end_of_code_point_mask == 0x924) {
    // We want to take 4 3-byte UTF-8 words and turn them into 4 2-byte UTF-16 words.
    // There is probably a more efficient sequence, but the following might do.
    const vecu8 sh = {2, 1, 0, -1, 5, 4, 3, -1, 8, 7, 6, -1, 11, 10, 9, -1};
    const vecu8 perm = vec_perm(in, in, sh);
    const vecu32 ascii =
        vec_and(perm, vec_splat_u32(0x7f)); // 7 or 6 bits
    const vecu32 middlebyte =
        vec_and(perm, vec_splat_u32(0x3f00)); // 5 or 6 bits
    const vecu32 middlebyte_shifted = vec_srl(middlebyte, 2);
    const vecu32 highbyte =
        vec_and(perm, vec_splat_u32(0x0f0000)); // 4 bits
    const vecu32 highbyte_shifted = vec_srl(highbyte, 4);
    const vecu32 composed =
        vec_or(vec_or(ascii, middlebyte_shifted), highbyte_shifted);
    const vecu8 composed_repacked = vec_packsu(composed, composed);
    vec_vsx_st(composed_repacked, 0, reinterpret_cast<vecu8 *>(utf16_output));
    utf16_output += 4;
    return 12;
  }
  /// We do not have a fast path available, so we fallback.

  const uint8_t idx =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][0];
  const uint8_t consumed =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][1];
  if (idx < 64) {
    // SIX (6) input code-words
    // this is a relatively easy scenario
    // we process SIX (6) input code-words. The max length in bytes of six code
    // words spanning between 1 and 2 bytes each is 12 bytes. On processors
    // where pdep/pext is fast, we might be able to use a small lookup table.
    const vecu8 sh = (vecu8)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(tables::utf8_to_utf16::shufutf8[idx])));
    const vecu8 perm = vec_perm(in, in, sh);
    const vecu16 ascii = vec_and(perm, vec_splat_u16(0x7f));
    const vecu16 highbyte = vec_and(perm, vec_splat_u16(0x1f00));
    const vecu16 composed = vec_or(ascii, vec_srl(highbyte, 2));
    vec_vsx_st(composed, 0, reinterpret_cast<vecu8 *>(utf16_output));
    utf16_output += 6; // We wrote 12 bytes, 6 code points.
  } else if (idx < 145) {
    // FOUR (4) input code-words
    const vecu8 sh = (vecu8)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(tables::utf8_to_utf16::shufutf8[idx])));
    const vecu8 perm = vec_perm(in, in, sh);
    const vecu32 ascii =
        vec_and(perm, vec_splat_u32(0x7f)); // 7 or 6 bits
    const vecu32 middlebyte =
        vec_and(perm, vec_splat_u32(0x3f00)); // 5 or 6 bits
    const vecu32 middlebyte_shifted = vec_srl(middlebyte, 2);
    const vecu32 highbyte =
        vec_and(perm, vec_splat_u32(0x0f0000)); // 4 bits
    const vecu32 highbyte_shifted = vec_srl(highbyte, 4);
    const vecu32 composed =
        vec_or(vec_or(ascii, middlebyte_shifted), highbyte_shifted);
    const vecu8 composed_repacked = vec_packsu(composed, composed);
    vec_vsx_st(composed_repacked, 0, reinterpret_cast<vecu8 *>(utf16_output));
    utf16_output += 4;
  } else if (idx < 209) {
    // TWO (2) input code-words
    const vecu8 sh = (vecu8)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(tables::utf8_to_utf16::shufutf8[idx])));
    const vecu8 perm = vec_perm(in, in, sh);
    const vecu32 ascii = vec_and(perm, vec_splat_u32(0x7f));
    const vecu32 middlebyte = vec_and(perm, vec_splat_u32(0x3f00));
    const vecu32 middlebyte_shifted = vec_srl(middlebyte, 2);
    vecu32 middlehighbyte = vec_and(perm, vec_splat_u32(0x3f0000));
    // correct for spurious high bit
    const vecu32 correct =
        vec_srl(vec_and(perm, vec_splat_u32(0x400000)), 1);
    middlehighbyte = vec_xor(correct, middlehighbyte);
    const vecu32 middlehighbyte_shifted = vec_srl(middlehighbyte, 4);
    const vecu32 highbyte = vec_and(perm, vec_splat_u32(0x07000000));
    const vecu32 highbyte_shifted = vec_srl(highbyte, 6);
    const vecu32 composed =
        vec_or(vec_or(ascii, middlebyte_shifted),
                     vec_or(highbyte_shifted, middlehighbyte_shifted));
    const vecu32 composedminus =
        vec_sub(composed, vec_splat_u32(0x10000));
    const vecu32 lowtenbits =
        vec_and(composedminus, vec_splat_u32(0x3ff));
    const vecu32 hightenbits = vec_srl(composedminus, 10);
    const vecu32 lowtenbitsadd =
        vec_add(lowtenbits, vec_splat_u32(0xDC00));
    const vecu32 hightenbitsadd =
        vec_add(hightenbits, vec_splat_u32(0xD800));
    const vecu32 lowtenbitsaddshifted = vec_srl(lowtenbitsadd, 16);
    const vecu32 surrogates =
        vec_or(hightenbitsadd, lowtenbitsaddshifted);
    uint32_t basic_buffer[4];
    vec_vsx_st(composed, 0, reinterpret_cast<vecu32 *>(basic_buffer));
    uint32_t surrogate_buffer[4];
    vec_vsx_st(surrogates, 0, reinterpret_cast<vecu32 *>(surrogate_buffer));
    for (size_t i = 0; i < 3; i++) {
      if (basic_buffer[i] < 65536) {
        utf16_output[0] = uint16_t(basic_buffer[i]);
        utf16_output++;
      } else {
        utf16_output[0] = uint16_t(surrogate_buffer[i] & 0xFFFF);
        utf16_output[1] = uint16_t(surrogate_buffer[i] >> 16);
        utf16_output += 2;
      }
    }
  } else {
    // here we know that there is an error but we do not handle errors
  }
  return consumed;
}