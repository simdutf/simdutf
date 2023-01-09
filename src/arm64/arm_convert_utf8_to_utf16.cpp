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
  #ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  const uint8x16_t swap = make_uint8x16_t(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
  #else
  const uint8x16_t swap = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14};
  #endif
  uint8x16_t in = vld1q_u8(reinterpret_cast<const uint8_t*>(input));
  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask & 0xfff;
  //
  // Optimization note: our main path below is load-latency dependent. Thus it is maybe
  // beneficial to have fast paths that depend on branch prediction but have less latency.
  // This results in more instructions but, potentially, also higher speeds.
  //
  // We first try a few fast paths.
  if((utf8_end_of_code_point_mask & 0xffff) == 0xffff) {
    // We process in chunks of 16 bytes
    uint16x8_t ascii_first = vmovl_u8(vget_low_u8 (in));
    uint16x8_t ascii_second = vmovl_high_u8(in);
    if (!match_system(big_endian)) {
      ascii_first = vreinterpretq_u16_u8(vqtbl1q_u8(vreinterpretq_u8_u16(ascii_first), swap));
      ascii_second = vreinterpretq_u16_u8(vqtbl1q_u8(vreinterpretq_u8_u16(ascii_second), swap));
    }
    vst1q_u16(reinterpret_cast<uint16_t*>(utf16_output), ascii_first);
    vst1q_u16(reinterpret_cast<uint16_t*>(utf16_output) + 8, ascii_second);
    utf16_output += 16; // We wrote 16 16-bit characters.
    return 16; // We consumed 16 bytes.
  }
  if((utf8_end_of_code_point_mask & 0xffff) == 0xaaaa) {
    // We want to take 8 2-byte UTF-8 words and turn them into 8 2-byte UTF-16 words.
    // There is probably a more efficient sequence, but the following might do.
    uint8x16_t perm = vqtbl1q_u8(in, swap);
    uint8x16_t ascii = vandq_u8(perm, vreinterpretq_u8_u16(vmovq_n_u16(0x7f)));
    uint8x16_t highbyte = vandq_u8(perm, vreinterpretq_u8_u16(vmovq_n_u16(0x1f00)));
    uint8x16_t composed = vorrq_u8(ascii, vreinterpretq_u8_u16(vshrq_n_u16(vreinterpretq_u16_u8(highbyte), 2)));
    if (!match_system(big_endian)) composed = vqtbl1q_u8(composed, swap);
    vst1q_u8(reinterpret_cast<uint8_t*>(utf16_output), composed);
    utf16_output += 8; // We wrote 16 bytes, 8 code points.
    return 16;
  }
  if(input_utf8_end_of_code_point_mask == 0x924) {
    // We want to take 4 3-byte UTF-8 words and turn them into 4 2-byte UTF-16 words.
    // There is probably a more efficient sequence, but the following might do.
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    const uint8x16_t sh = make_uint8x16_t(2, 1, 0, 255, 5, 4, 3, 255, 8, 7, 6, 255, 11, 10, 9, 255);
#else
    const uint8x16_t sh = {2, 1, 0, 255, 5, 4, 3, 255, 8, 7, 6, 255, 11, 10, 9, 255};
#endif
    uint8x16_t perm = vqtbl1q_u8(in, sh);
    uint8x16_t ascii =
        vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x7f))); // 7 or 6 bits
    uint8x16_t middlebyte =
        vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x3f00))); // 5 or 6 bits
    uint8x16_t middlebyte_shifted = vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(middlebyte), 2));
    uint32x4_t highbyte =
        vreinterpretq_u32_u8(vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x0f0000)))); // 4 bits
    uint32x4_t highbyte_shifted = vshrq_n_u32(highbyte, 4);
    uint32x4_t composed =
        vorrq_u32(vorrq_u32(vreinterpretq_u32_u8(ascii), vreinterpretq_u32_u8(middlebyte_shifted)), highbyte_shifted);
    uint16x8_t composed_repacked = vmovn_high_u32(vmovn_u32(composed), composed);
    if (!match_system(big_endian)) composed_repacked = vreinterpretq_u16_u8(vqtbl1q_u8(vreinterpretq_u8_u16(composed_repacked), swap));
    vst1q_u16(reinterpret_cast<uint16_t*>(utf16_output), composed_repacked);
    utf16_output += 4;
    return 12;
  }
  /// We do not have a fast path available, so we fallback.

  const uint8_t idx =
      simdutf::tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][0];
  const uint8_t consumed =
      simdutf::tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][1];


  if (idx < 64) {
    // SIX (6) input code-words
    // this is a relatively easy scenario
    // we process SIX (6) input code-words. The max length in bytes of six code
    // words spanning between 1 and 2 bytes each is 12 bytes.
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(simdutf::tables::utf8_to_utf16::shufutf8[idx]));
    uint8x16_t perm = vqtbl1q_u8(in, sh);
    uint8x16_t ascii = vandq_u8(perm, vreinterpretq_u8_u16(vmovq_n_u16(0x7f)));
    uint8x16_t highbyte = vandq_u8(perm, vreinterpretq_u8_u16(vmovq_n_u16(0x1f00)));
    uint8x16_t composed = vorrq_u8(ascii, vreinterpretq_u8_u16(vshrq_n_u16(vreinterpretq_u16_u8(highbyte), 2)));
    if (!match_system(big_endian)) composed = vqtbl1q_u8(composed, swap);
    vst1q_u8(reinterpret_cast<uint8_t*>(utf16_output), composed);
    utf16_output += 6; // We wrote 12 bytes, 6 code points.
  } else if (idx < 145) {
    // FOUR (4) input code-words
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(simdutf::tables::utf8_to_utf16::shufutf8[idx]));
    uint8x16_t perm = vqtbl1q_u8(in, sh);
    uint8x16_t ascii =
        vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x7f))); // 7 or 6 bits
    uint8x16_t middlebyte =
        vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x3f00))); // 5 or 6 bits
    uint8x16_t middlebyte_shifted = vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(middlebyte), 2));
    uint32x4_t highbyte =
        vreinterpretq_u32_u8(vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x0f0000)))); // 4 bits
    uint32x4_t highbyte_shifted = vshrq_n_u32(highbyte, 4);
    uint32x4_t composed =
        vorrq_u32(vorrq_u32(vreinterpretq_u32_u8(ascii), vreinterpretq_u32_u8(middlebyte_shifted)), highbyte_shifted);
    uint16x8_t composed_repacked = vmovn_high_u32(vmovn_u32(composed), composed);
    if (!match_system(big_endian)) composed_repacked = vreinterpretq_u16_u8(vqtbl1q_u8(vreinterpretq_u8_u16(composed_repacked), swap));
    vst1q_u16(reinterpret_cast<uint16_t*>(utf16_output), composed_repacked);
    utf16_output += 4;
  } else if (idx < 209) {
    // TWO (2) input code-words
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(simdutf::tables::utf8_to_utf16::shufutf8[idx]));
    uint8x16_t perm = vqtbl1q_u8(in, sh);
    uint8x16_t ascii = vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x7f)));
    uint8x16_t middlebyte = vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x3f00)));
    uint8x16_t middlebyte_shifted = vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(middlebyte), 2));
    uint8x16_t middlehighbyte = vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x3f0000)));
    // correct for spurious high bit
    uint8x16_t correct =
        vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x400000)))), 1));
    middlehighbyte = veorq_u8(correct, middlehighbyte);
    uint8x16_t middlehighbyte_shifted = vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(middlehighbyte), 4));
    uint8x16_t highbyte = vandq_u8(perm, vreinterpretq_u8_u32(vmovq_n_u32(0x07000000)));
    uint8x16_t highbyte_shifted =vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(highbyte), 6));
    uint8x16_t composed =
        vorrq_u8(vorrq_u8(ascii, middlebyte_shifted),
                     vorrq_u8(highbyte_shifted, middlehighbyte_shifted));
    uint32x4_t composedminus =
        vsubq_u32(vreinterpretq_u32_u8(composed), vmovq_n_u32(0x10000));
    uint32x4_t lowtenbits =
        vandq_u32(composedminus, vmovq_n_u32(0x3ff));
    uint32x4_t hightenbits = vshrq_n_u32(composedminus, 10);
    uint32x4_t lowtenbitsadd =
        vaddq_u32(lowtenbits, vmovq_n_u32(0xDC00));
    uint32x4_t hightenbitsadd =
        vaddq_u32(hightenbits, vmovq_n_u32(0xD800));
    uint32x4_t lowtenbitsaddshifted = vshlq_n_u32(lowtenbitsadd, 16);
    uint32x4_t surrogates =
        vorrq_u32(hightenbitsadd, lowtenbitsaddshifted);
    uint32_t basic_buffer[4];
    uint32_t basic_buffer_swap[4];
    if (!match_system(big_endian)) {
      vst1q_u32(basic_buffer_swap, vreinterpretq_u32_u8(vqtbl1q_u8(composed, swap)));
      surrogates = vreinterpretq_u32_u8(vqtbl1q_u8(vreinterpretq_u8_u32(surrogates), swap));
    }
    vst1q_u32(basic_buffer, vreinterpretq_u32_u8(composed));
    uint32_t surrogate_buffer[4];
    vst1q_u32(surrogate_buffer, surrogates);
    for (size_t i = 0; i < 3; i++) {
      if (basic_buffer[i] < 65536) {
        utf16_output[0] = !match_system(big_endian) ? uint16_t(basic_buffer_swap[i]) : uint16_t(basic_buffer[i]);
        utf16_output++;
      } else {
        utf16_output[0] = uint16_t(surrogate_buffer[i] & 0xffff);
        utf16_output[1] = uint16_t(surrogate_buffer[i] >> 16);
        utf16_output += 2;
      }
    }
  } else {
    // here we know that there is an error but we do not handle errors
  }
  return consumed;
}
