// Convert up to 12 bytes from utf8 to utf32 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
size_t convert_masked_utf8_to_utf32(const char *input,
                           uint64_t utf8_end_of_code_point_mask,
                           char32_t *&utf32_out) {
  // we use an approach where we try to process up to 12 input bytes.
  // Why 12 input bytes and not 16? Because we are concerned with the size of
  // the lookup tables. Also 12 is nicely divisible by two and three.
  //
  uint32_t*& utf32_output = reinterpret_cast<uint32_t*&>(utf32_out);
  uint8x16_t in = vld1q_u8(reinterpret_cast<const uint8_t*>(input));
  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask & 0xFFF;
  //
  // Optimization note: our main path below is load-latency dependent. Thus it is maybe
  // beneficial to have fast paths that depend on branch prediction but have less latency.
  // This results in more instructions but, potentially, also higher speeds.
  //
  // We first try a few fast paths.
  if((utf8_end_of_code_point_mask & 0xffff) == 0xffff) {
    // We process in chunks of 16 bytes
    uint16x8_t low = vmovl_u8(vget_low_u8(in));
    uint16x8_t high = vmovl_high_u8(in);

    vst1q_u32(utf32_output, vmovl_u16(vget_low_u16(low)));
    vst1q_u32(utf32_output + 4, vmovl_high_u16(low));
    vst1q_u32(utf32_output + 8, vmovl_u16(vget_low_u16(high)));
    vst1q_u32(utf32_output + 12, vmovl_high_u16(high));
    utf32_output += 16; // We wrote 16 16-bit characters.
    return 16; // We consumed 16 bytes.
  }
  if((utf8_end_of_code_point_mask & 0xffff) == 0xaaaa) {
    // We want to take 8 2-byte UTF-8 words and turn them into 8 4-byte UTF-32 words.
    // 10bbbbbb110aaaaa
    uint16x8_t upper = vreinterpretq_u16_u8(in);
    // 00000000000aaaaa
    uint16x8_t upper_masked = vandq_u16(upper, vmovq_n_u16(0x1F));
    // (effectively in >> 8)
    // 110aaaaa10bbbbbb
    uint16x8_t lower = vreinterpretq_u16_u8(vrev16q_u8(in));
    // Assemble with shift left insert.
    // 00000aaaaabbbbbb
    uint16x8_t composed = vsliq_n_u16(lower, upper_masked, 6);
    // Zero extend and store
    vst1q_u32(utf32_output,  vmovl_u16(vget_low_u16(composed)));
    vst1q_u32(utf32_output+4,  vmovl_high_u16(composed));
    utf32_output += 8; // We wrote 32 bytes, 8 code points.
    return 16;
  }
  if(input_utf8_end_of_code_point_mask == 0x924) {
    // We want to take 4 3-byte UTF-8 words and turn them into 4 4-byte UTF-32 words.
    // XXX: Benchmark against not narrowing
    // This means that a more optimized table can be used.

    // Low half contains  10cccccc|1110aaaa
    // High half contains 10bbbbbb|10bbbbbb
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    const uint8x16_t sh = make_uint8x16_t(0, 2, 3, 5, 6, 8, 9, 11, 1, 1, 4, 4, 7, 7, 10, 10);
#else
    const uint8x16_t sh = {0, 2, 3, 5, 6, 8, 9, 11, 1, 1, 4, 4, 7, 7, 10, 10};
#endif
    uint8x16_t perm = vqtbl1q_u8(in, sh);
    // Split
    // 10cccccc|1110aaaa
    uint8x8_t perm_low = vget_low_u8(perm); // no-op
    // 10bbbbbb|10bbbbbb
    uint8x8_t perm_high = vget_high_u8(perm);
    // Equivalent to perm_low >> 8
    // xxxxxxxx 10cccccc
    uint16x4_t low = vreinterpret_u16_u8(vrev16_u8(perm_low));
    // xxxxxxxx 10bbbbbb
    uint16x4_t mid = vreinterpret_u16_u8(perm_high); // no-op
    // xxxxxxxx 1110aaaa
    uint16x4_t high = vreinterpret_u16_u8(perm_low); // no-op
    // Assemble with shift left insert.
    // xxxxxxaa aabbbbbb
    uint16x4_t mid_high = vsli_n_u16(mid, high, 6);
    // aaaabbbb bbcccccc
    uint16x4_t composed = vsli_n_u16(low, mid_high, 6);
    // Zero extend and store
    vst1q_u32(utf32_output, vmovl_u16(composed));
    utf32_output += 4;
    return 12;
  }
  /// Either no fast path or an unimportant fast path.

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
    uint16x8_t perm = vreinterpretq_u16_u8(vqtbl1q_u8(in, sh));
    // Mask
    // 1 byte: 00000000 0aaaaaaa
    // 2 byte: 00000000 00bbbbbb
    uint16x8_t ascii = vandq_u16(perm, vmovq_n_u16(0x7f)); // 6 or 7 bits
    // 1 byte: 00000000 00000000
    // 2 byte: 000aaaaa 00000000
    uint16x8_t highbyte = vandq_u16(perm, vmovq_n_u16(0x1f00)); // 5 bits
    // Combine with a shift right accumulate
    // 1 byte: 00000000 0aaaaaaa
    // 2 byte: 00000aaa aabbbbbb
    uint16x8_t composed = vsraq_n_u16(ascii, highbyte, 2);
    // Zero extend and store
    vst1q_u32(utf32_output,  vmovl_u16(vget_low_u16(composed)));
    vst1q_u32(utf32_output+4,  vmovl_high_u16(composed));
    utf32_output += 6; // We wrote 12 bytes, 6 code points.
  } else if (idx < 145) {
    // FOUR (4) input code-words
    // UTF-16 does these on half size vectors, UTF-32 keeps full vectors.
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(simdutf::tables::utf8_to_utf16::shufutf8[idx]));
    uint32x4_t perm = vreinterpretq_u32_u8(vqtbl1q_u8(in, sh));
    // Split
    // 00000000 00000000 0ccccccc
    uint32x4_t ascii = vandq_u32(perm, vmovq_n_u32(0x7F));    // 6 or 7 bits
    // Note: unmasked
    // xxxxxxxx aaaaxxxx xxxxxxxx
    uint32x4_t high = vshrq_n_u32(perm, 4);                   // 4 bits
    // Use 16 bit bic instead of and
    // The top bits will be corrected later
    // 00000000 10bbbbbb 00000000
    uint32x4_t middle =
        vreinterpretq_u32_u16(vbicq_u16(vreinterpretq_u16_u32(perm), vmovq_n_u16(0x00ff))); // 5 or 6 bits
    // Combine low and middle with shift right accumulate
    // 00000000 00xxbbbb bbcccccc
    uint32x4_t lowmid = vsraq_n_u32(ascii, middle, 2);
    // Insert top 4 bits from high byte with bitwise select
    // 00000000 aaaabbbb bbcccccc
    uint32x4_t composed = vbslq_u32(vmovq_n_u32(0x0000F000), high, lowmid);
    vst1q_u32(utf32_output, composed);
    utf32_output += 4;
  } else if (input_utf8_end_of_code_point_mask == 0x888) {
    // We want to take 3-4 4-byte UTF-8 words and turn them into 3-4 4-byte UTF-32 words.
    // This uses the same method as the fixed 3 byte version, reversing and shift left insert.
    // However, there is no need for a shuffle mask now, just rev16 and rev32.
    //
    // This version does not use the LUT, but 4 byte sequences are less common and the
    // overhead of the extra memory access is less important than the early branch overhead
    // in shorter sequences, so it comes last.

    // branchless
    size_t count = 3 + ((utf8_end_of_code_point_mask & 0xFFFF) == 0x8888);
    // Swap pairs of bytes
    // 10dddddd|10cccccc|10bbbbbb|11110aaa
    // 10cccccc 10dddddd|11110aaa 10bbbbbb
    uint16x8_t swap1 = vreinterpretq_u16_u8(vrev16q_u8(in));
    // Shift left and insert
    // xxxxcccc ccdddddd|xxxxxxxa aabbbbbb
    uint16x8_t merge1 = vsliq_n_u16(swap1, vreinterpretq_u16_u8(in), 6);
    // Swap 16-bit lanes
    // xxxxcccc ccdddddd xxxxxxxa aabbbbbb
    // xxxxxxxa aabbbbbb xxxxcccc ccdddddd
    uint32x4_t swap2 = vreinterpretq_u32_u16(vrev32q_u16(merge1));
    // Shift insert again
    // xxxxxxxx xxxaaabb bbbbcccc ccdddddd
    uint32x4_t merge2 = vsliq_n_u32(swap2, vreinterpretq_u32_u16(merge1), 12);
    // Clear the garbage
    // 00000000 000aaabb bbbbcccc ccdddddd
    uint32x4_t composed = vandq_u32(merge2, vmovq_n_u32(0x1FFFFF));
    // Store
    vst1q_u32(utf32_output, composed);

    utf32_output += count;
    return count * 4;
  } else if (idx < 209) {
    // TWO (2) input code-words
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(simdutf::tables::utf8_to_utf16::shufutf8[idx]));
    // 1 byte: 00000000 00000000 00000000 0ddddddd
    // 2 byte: 00000000 00000000 110ccccc 10dddddd
    // 3 byte: 00000000 1110bbbb 10cccccc 10dddddd
    // 4 byte: 11110aaa 10bbbbbb 10cccccc 10dddddd
    uint32x4_t perm = vreinterpretq_u32_u8(vqtbl1q_u8(in, sh));
    // Ascii
    uint32x4_t ascii = vandq_u32(perm, vmovq_n_u32(0x7F));
    uint32x4_t middle = vandq_u32(perm, vmovq_n_u32(0x3f00));
    // When converting the way we do, the three byte prefix will be interpreted as the
    // 18th bit being set. Correcting this is taking bit 6 of the third byte, shifting
    // right 1 bit, and either doing an xor or an 8-bit add. Since we have shift right
    // accumulate, we do the latter.
    //  4 byte   3 byte
    // 10bbbbbb 1110bbbb
    // 00000000 01000000 6th bit
    // 00000000 00100000 shift right
    // 10bbbbbb 0000bbbb add
    // 00bbbbbb 0000bbbb mask
    uint8x16_t correction =
        vreinterpretq_u8_u32(vandq_u32(perm, vmovq_n_u32(0x00400000)));
    uint32x4_t corrected =
        vreinterpretq_u32_u8(vsraq_n_u8(vreinterpretq_u8_u32(perm), correction, 1));
    // 00000000 00000000 0000cccc ccdddddd
    uint32x4_t cd = vsraq_n_u32(ascii, middle, 2);
    // Insert twice
    // xxxxxxxx xxxaaabb bbbbxxxx xxxxxxxx
    uint32x4_t ab = vbslq_u32(vmovq_n_u32(0x01C0000), vshrq_n_u32(corrected, 6), vshrq_n_u32(corrected, 4));
    // 00000000 000aaabb bbbbcccc ccdddddd
    uint32x4_t composed = vbslq_u32(vmovq_n_u32(0xFFE00FFF), cd, ab);
    // Store
    vst1q_u32(utf32_output, composed);
    utf32_output += 3;
  } else {
    // here we know that there is an error but we do not handle errors
  }
  return consumed;
}
