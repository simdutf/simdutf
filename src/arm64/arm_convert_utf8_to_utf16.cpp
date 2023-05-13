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
  uint8x16_t in = vld1q_u8(reinterpret_cast<const uint8_t*>(input));
  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask & 0xfff;
  //
  // Optimization note: our main path below is load-latency dependent. Thus it is maybe
  // beneficial to have fast paths that depend on branch prediction but have less latency.
  // This results in more instructions but, potentially, also higher speeds.

  // We first try a few fast paths.
  if((utf8_end_of_code_point_mask & 0xFFFF) == 0xffff) {
    // We process in chunks of 16 bytes
    uint16x8_t ascii_first, ascii_second;
    if (match_system(big_endian)) {
      // zero extend
      ascii_first = vmovl_u8(vget_low_u8(in));
      ascii_second = vmovl_high_u8(in);
    } else {
      // zero extend and shift left 8
      // effectively zero extend and byteswap
      ascii_first = vshll_n_u8(vget_low_u8(in), 8);
      ascii_second = vshll_high_n_u8(in, 8);
    }
    utf16_output += 16; // We wrote 16 16-bit characters.
    vst1q_u16(reinterpret_cast<uint16_t*>(utf16_output) - 16, ascii_first);
    vst1q_u16(reinterpret_cast<uint16_t*>(utf16_output) - 8, ascii_second);
    return 16; // We consumed 16 bytes.
  }

  if ((utf8_end_of_code_point_mask & 0xFFFF) == 0xaaaa) {
    // We want to take 8 2-byte UTF-8 words and turn them into 8 2-byte UTF-16 words.
    // 10bbbbbb110aaaaa
    uint16x8_t upper = vreinterpretq_u16_u8(in);
    // (effectively in >> 8)
    // 110aaaaa10bbbbbb
    uint16x8_t lower = vreinterpretq_u16_u8(vrev16q_u8(in));
    // 00000000000aaaaa
    uint16x8_t upper_masked = vandq_u16(upper, vmovq_n_u16(0x1F));
    // Assemble with shift left insert.
    // 00000aaaaabbbbbb
    uint16x8_t composed = vsliq_n_u16(lower, upper_masked, 6);

    if (!match_system(big_endian))
      composed = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(composed)));
    utf16_output += 8; // We wrote 16 bytes, 8 code points.
    vst1q_u16(reinterpret_cast<uint16_t *>(utf16_output - 8), composed);
    return 16;
  }

  if (input_utf8_end_of_code_point_mask == 0x924) {
    // We want to take 4 3-byte UTF-8 words and turn them into 4 2-byte UTF-16 words.
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
    // xxxxxxxx 10bbbbbb
    uint16x4_t mid = vreinterpret_u16_u8(perm_high); // no-op
    // xxxxxxxx 1110aaaa
    uint16x4_t high = vreinterpret_u16_u8(perm_low); // no-op
    // Assemble with shift left insert.
    // xxxxxxaa aabbbbbb
    uint16x4_t mid_high = vsli_n_u16(mid, high, 6);
    // Equivalent to perm_low >> 8
    // xxxxxxxx 10cccccc
    uint16x4_t low = vreinterpret_u16_u8(vrev16_u8(perm_low));
    // aaaabbbb bbcccccc
    uint16x4_t composed = vsli_n_u16(low, mid_high, 6);
    if (!match_system(big_endian))
      composed = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(composed)));
    utf16_output += 4;
    vst1_u16(reinterpret_cast<uint16_t*>(utf16_output - 4), composed);
    return 12;
  }

  /// We do not have a fast path available, or the fast path is unimportant, so we fallback.

  const uint8_t consumed =
      simdutf::tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][1];

  const uint8_t idx =
      simdutf::tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][0];

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
    uint16x8_t highbyte = vandq_u16(perm, vmovq_n_u16(0x3f00)); // 5 bits
    // Combine with a shift right accumulate
    // 1 byte: 00000000 0aaaaaaa
    // 2 byte: 00000aaa aabbbbbb
    uint16x8_t composed = vsraq_n_u16(ascii, highbyte, 2);
    if (!match_system(big_endian))
      composed = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(composed)));
    vst1q_u16(reinterpret_cast<uint16_t*>(utf16_output), composed);
    utf16_output += 6; // We wrote 12 bytes, 6 code points.
    return consumed;
  } else if (idx < 145) {
    // FOUR (4) input code-words
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(simdutf::tables::utf8_to_utf16::shufutf8[idx]));
    // XXX: depending on the system scalar instructions might be faster.
    // 1 byte: 00000000 00000000 0ccccccc
    // 2 byte: 00000000 110bbbbb 10cccccc
    // 3 byte: 1110aaaa 10bbbbbb 10cccccc
    uint32x4_t perm = vreinterpretq_u32_u8(vqtbl1q_u8(in, sh));
    // 1 byte: 00000000 0ccccccc
    // 2 byte: xx0bbbbb x0cccccc
    // 3 byte: xxbbbbbb x0cccccc
    uint16x4_t lowperm = vmovn_u32(perm);
    // Partially mask with bic (doesn't require a temporary register unlike and)
    // The sli will clear the top bits.
    // 1 byte: 00000000 00000000
    // 2 byte: xx0bbbbb 00000000
    // 3 byte: xxbbbbbb 00000000
    uint16x4_t middlebyte = vbic_u16(lowperm, vmov_n_u16(0xFF));
    // ASCII
    // 1 byte: 00000000 0ccccccc
    // 2+byte: 00000000 00cccccc
    uint16x4_t ascii = vand_u16(lowperm, vmov_n_u16(0x7F));
    // Split into narrow vectors.
    // 2 byte: 00000000 00000000
    // 3 byte: 00000000 xxxxaaaa
    uint16x4_t highperm = vshrn_n_u32(perm, 16);
    // Shift right accumulate the middle byte
    // 1 byte: 00000000 0ccccccc
    // 2 byte: 00xx0bbb bbcccccc
    // 3 byte: 00xxbbbb bbcccccc
    uint16x4_t middlelow = vsra_n_u16(ascii, middlebyte, 2);
    // Shift left and insert the top 4 bits, overwriting the garbage
    // 1 byte: 00000000 0ccccccc
    // 2 byte: 00000bbb bbcccccc
    // 3 byte: aaaabbbb bbcccccc
    uint16x4_t composed = vsli_n_u16(middlelow, highperm, 12);
    if (!match_system(big_endian))
      composed = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(composed)));
    vst1_u16(reinterpret_cast<uint16_t*>(utf16_output), composed);

    utf16_output += 4;
    return consumed;
  } else if (idx < 209) {
    // TWO (2) input code-words
    if (input_utf8_end_of_code_point_mask == 0x888) {
      // We want to take 3-4 4-byte UTF-8 words and turn them into 3-4 4-byte UTF-16 pairs.
      // Generating surrogate pairs is a little tricky though, but it is easier when we
      // can assume they are all pairs.
      // This version does not use the LUT, but 4 byte sequences are less common and the
      // overhead of the extra memory access is less important than the early branch overhead
      // in shorter sequences.

      // This is branchless on arm
      size_t count = 3 + ((utf8_end_of_code_point_mask & 0xFFFF) == 0x8888);
      // Swap byte pairs
      // 10dddddd 10cccccc|10bbbbbb 11110aaa
      // 10cccccc 10dddddd|11110aaa 10bbbbbb
      uint8x16_t swap = vrev16q_u8(in);
      // Shift left 2 bits
      // cccccc00 dddddd00 xxxxxxxx bbbbbb00
      uint32x4_t shift = vreinterpretq_u32_u8(vshlq_n_u8(swap, 2));
      // Create a magic number containing the low 2 bits of the trail surrogatw and all the corrections
      // needed to create the pair
      // UTF-8 4b prefix   = -0x0000|0xF000
      // surrogate offset  = -0x0000|0x0040 (0x10000 << 6)
      // surrogate high    = +0x0000|0xD800
      // surrogate low     = +0xDC00|0x0000
      // -------------------------------
      //                   = +0xDC00|0xE7C0
      uint32x4_t magic = vmovq_n_u32(0xDC00E7C0);
      // Generate unadjusted trail surrogate minus lowest 2 bits
      // xxxxxxxx xxxxxxxx|11110aaa bbbbbb00
      uint32x4_t trail = vbslq_u32(vmovq_n_u32(0x0000FF00), vreinterpretq_u32_u8(swap), shift);
      // Insert low 2 bits of trail surrogate to magic number for later
      // 11011100 00000000 11100111 110000cc
      uint16x8_t magic_with_low_2 = vreinterpretq_u16_u32(vsraq_n_u32(magic, shift, 30));
      // Generate lead surrogate
      // xxxxcccc ccdddddd|xxxxxxxx xxxxxxxx
      uint32x4_t lead = vreinterpretq_u32_u16(vsliq_n_u16(vreinterpretq_u16_u8(swap), vreinterpretq_u16_u8(in), 6));
      // Mask out lead
      // 000000cc ccdddddd|xxxxxxxx xxxxxxxx
      lead = vbicq_u32(lead, vmovq_n_u32(0xFC000000));
      // Blend pairs
      // 000000cc ccdddddd|11110aaa bbbbbb00
      uint16x8_t blend = vreinterpretq_u16_u32(vbslq_u32(vmovq_n_u32(0x0000FFFF), trail, lead));
      // Add magic number to finish the result
      // 110111CC CCDDDDDD|110110AA BBBBBBCC
      uint16x8_t composed = vaddq_u16(blend, magic_with_low_2);
      if (!match_system(big_endian))
        composed = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(composed)));
      vst1q_u16(reinterpret_cast<uint16_t *>(utf16_output), composed);
      utf16_output += count * 2;
      return count * 4;
    }
    // Mixed up to 3 1-4 byte sequences
    uint8x16_t sh = vld1q_u8(reinterpret_cast<const uint8_t*>(simdutf::tables::utf8_to_utf16::shufutf8[idx]));

    // 1 byte: 00000000 00000000 00000000 0ddddddd
    // 3 byte: 00000000 00000000 110ccccc 10dddddd
    // 3 byte: 00000000 1110bbbb 10cccccc 10dddddd
    // 4 byte: 11110aaa 10bbbbbb 10cccccc 10dddddd
    uint32x4_t perm = vreinterpretq_u32_u8(vqtbl1q_u8(in, sh));
    // Mask the low and middle bytes
    // 00000000 00000000 00000000 0ddddddd
    uint32x4_t ascii = vandq_u32(perm, vmovq_n_u32(0x7f));
    // Because the surrogates need more work, the high surrogate is computed first.
    uint32x4_t middlehigh = vshlq_n_u32(perm, 2);
    // 00000000 00000000 00cccccc 00000000
    uint32x4_t middlebyte = vandq_u32(perm, vmovq_n_u32(0x3F00));
    // Start assembling the sequence. Since the 4th byte is in the same position as it
    // would be in a surrogate and there is no dependency, shift left instead of right.
    // 3 byte: 00000000 10bbbbxx xxxxxxxx xxxxxxxx
    // 4 byte: 11110aaa bbbbbbxx xxxxxxxx xxxxxxxx
    uint32x4_t ab = vbslq_u32(vmovq_n_u32(0xFF000000), perm, middlehigh);
    // Top 16 bits contains the high ten bits of the surrogate pair before correction
    // 3 byte: 00000000 10bbbbcc|cccc0000 00000000
    // 4 byte: 11110aaa bbbbbbcc|cccc0000 00000000 - high 10 bits correct w/o correction
    uint32x4_t abc = vbslq_u32(vmovq_n_u32(0xFFFC0000), ab, vshlq_n_u32(middlebyte, 4));
    // Combine the low 6 or 7 bits by a shift right accumulate
    // 3 byte: 00000000 00000010|bbbbcccc ccdddddd - low 16 bits correct
    // 4 byte: 00000011 110aaabb|bbbbcccc ccdddddd - low 10 bits correct w/o correction
    uint32x4_t composed = vsraq_n_u32(ascii, abc, 6);
    // After this is for surrogates
    // Blend the low and high surrogates
    // 4 byte: 11110aaa bbbbbbcc|bbbbcccc ccdddddd
    uint32x4_t mixed = vbslq_u32(vmovq_n_u32(0xFFFF0000), abc, composed);
    // Clear the upper 6 bits of the low surrogate. Don't clear the upper bits yet as
    // 0x10000 was not subtracted from the codepoint yet.
    // 4 byte: 11110aaa bbbbbbcc|000000cc ccdddddd
    uint16x8_t masked_pair = vreinterpretq_u16_u32(vbicq_u32(mixed, vmovq_n_u32(~0xFFFF03FF)));
    // Correct the remaining UTF-8 prefix, surrogate offset, and add the surrogate prefixes
    // in one magic addition.
    // similar magic number but without the continue byte adjust and halfword swapped
    // UTF-8 4b prefix   = -0xF000|0x0000
    // surrogate offset  = -0x0040|0x0000 (0x10000 << 6)
    // surrogate high    = +0xD800|0x0000
    // surrogate low     = +0x0000|0xDC00
    // -----------------------------------
    //                   = +0xE7C0|0xDC00
    // 4 byte: 110110AA BBBBBBCC|110111CC CCDDDDDD - surrogate pair complete
    uint32x4_t surrogates =
        vreinterpretq_u32_u16(vaddq_u16(masked_pair, vreinterpretq_u16_u32(vmovq_n_u32(0xE7C0DC00))));
    // If the high bit is 1 (s32 less than zero), this needs a surrogate pair
    uint32x4_t is_pair = vcltzq_s32(vreinterpretq_s32_u32(perm));

    // Select either the 4 byte surrogate pair or the 2 byte solo codepoint
    // 3 byte: 0xxxxxxx xxxxxxxx|bbbbcccc ccdddddd
    // 4 byte: 110110AA BBBBBBCC|110111CC CCDDDDDD
    uint32x4_t selected = vbslq_u32(is_pair, surrogates, composed);
    // Byte swap
    if (!match_system(big_endian))
      selected = vreinterpretq_u32_u8(vrev16q_u8(vreinterpretq_u8_u32(selected)));
    // Attempting to shuffle and store would be complex, just scalarize.
    uint32_t buffer[4];
    vst1q_u32(buffer, selected);
    // Test for the top bit of the surrogate mask.
    const uint32_t SURROGATE_MASK = match_system(big_endian) ? 0x80000000 : 0x00800000;
    for (size_t i = 0; i < 3; i++) {
      // Surrogate
      if (buffer[i] & SURROGATE_MASK) {
        utf16_output[0] = uint16_t(buffer[i] >> 16);
        utf16_output[1] = uint16_t(buffer[i] & 0xFFFF);
        utf16_output += 2;
      } else {
        utf16_output[0] = uint16_t(buffer[i] & 0xFFFF);
        utf16_output++;
      }
    }
    return consumed;
  } else {
    // here we know that there is an error but we do not handle errors
    return consumed;
  }
}

