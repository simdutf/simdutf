
/*
 * Returns if a vector of type uint8x16_t is all zero.
 */
simdutf_really_inline int veq_non_zero(uint8x16_t v) {
  // might compile to two instructions:
  //	umaxv   s0, v0.4s
  //	fmov	w0, s0
  // On Apple hardware, they both have a latency of 3 cycles, with a throughput
  // of four instructions per cycle. So that's 6 cycles of latency (!!!) for the
  // two instructions. A narrowing shift has the same latency and throughput.
  return vmaxvq_u32(vreinterpretq_u32_u8(v));
}

/*
 * Process one block of 16 characters.  If in_place is false,
 * copy the block from in to out.  If there is a sequencing
 * error in the block, overwrite the illsequenced characters
 * with the replacement character.  This function reads one
 * character before the beginning of the buffer as a lookback.
 * If that character is illsequenced, it too is overwritten.
 */
template <endianness big_endian, bool inplace>
void utf16fix_block(char16_t *out, const char16_t *in) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  uint8x16x2_t lb, block;
  uint8x16_t lb_masked, block_masked, lb_is_high, block_is_low;
  uint8x16_t illseq;

  const int idx = !match_system(big_endian) ? 0 : 1;

  /* TODO: compute lookback using shifts */
  lb = vld2q_u8((const uint8_t *)(in - 1));
  block = vld2q_u8((const uint8_t *)in);
  lb_masked = vandq_u8(lb.val[idx], vdupq_n_u8(0xfc));
  block_masked = vandq_u8(block.val[idx], vdupq_n_u8(0xfc));
  lb_is_high = vceqq_u8(lb_masked, vdupq_n_u8(0xd8));
  block_is_low = vceqq_u8(block_masked, vdupq_n_u8(0xdc));

  illseq = veorq_u8(lb_is_high, block_is_low);
  if (veq_non_zero(illseq)) {
    uint8x16_t lb_illseq, block_illseq;
    char16_t lbc;
    int ill;

    /* compute the cause of the illegal sequencing */
    lb_illseq = vbicq_u8(lb_is_high, block_is_low);
    block_illseq = vorrq_u8(vbicq_u8(block_is_low, lb_is_high),
                            vextq_u8(lb_illseq, vdupq_n_u8(0), 1));

    /* fix illegal sequencing in the lookback */
    ill = vgetq_lane_u8(lb_illseq, 0);
    lbc = out[-1];
    out[-1] = ill ? replacement : lbc;

    /* fix illegal sequencing in the main block */
    if (!match_system(big_endian)) {
      block.val[1] = vbslq_u8(block_illseq, vdupq_n_u8(0xfd), block.val[1]);
      block.val[0] = vorrq_u8(block_illseq, block.val[0]);
    } else {
      block.val[0] = vbslq_u8(block_illseq, vdupq_n_u8(0xfd), block.val[0]);
      block.val[1] = vorrq_u8(block_illseq, block.val[1]);
    }

    vst2q_u8((uint8_t *)out, block);
  } else if (!inplace) {
    vst2q_u8((uint8_t *)out, block);
  }
}

template <endianness big_endian, bool inplace>
uint8x16_t get_mismatch_copy(const char16_t *in, char16_t *out) {
  const int idx = !match_system(big_endian) ? 0 : 1;
  uint8x16x2_t lb = vld2q_u8((const uint8_t *)(in - 1));
  uint8x16x2_t block = vld2q_u8((const uint8_t *)in);
  uint8x16_t lb_masked = vandq_u8(lb.val[idx], vdupq_n_u8(0xfc));
  uint8x16_t block_masked = vandq_u8(block.val[idx], vdupq_n_u8(0xfc));
  uint8x16_t lb_is_high = vceqq_u8(lb_masked, vdupq_n_u8(0xd8));
  uint8x16_t block_is_low = vceqq_u8(block_masked, vdupq_n_u8(0xdc));
  uint8x16_t illseq = veorq_u8(lb_is_high, block_is_low);
  if (!inplace) {
    vst2q_u8((uint8_t *)out, block);
  }
  return illseq;
}

simdutf_really_inline uint64_t get_mask(uint8x16_t illse0, uint8x16_t illse1,
                                        uint8x16_t illse2, uint8x16_t illse3) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  uint8x16_t bit_mask =
      simdutf_make_uint8x16_t(0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                              0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80);
#else
  uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                         0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
#endif
  uint8x16_t sum0 =
      vpaddq_u8(vandq_u8(illse0, bit_mask), vandq_u8(illse1, bit_mask));
  uint8x16_t sum1 =
      vpaddq_u8(vandq_u8(illse2, bit_mask), vandq_u8(illse3, bit_mask));
  sum0 = vpaddq_u8(sum0, sum1);
  sum0 = vpaddq_u8(sum0, sum0);
  return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
}

// The idea is to process 64 characters at a time, and if there is a mismatch
// we can fix it with a bit of scalar code. When the input is correct, this
// function might be faster than alternative implementations working on small
// blocks of input.
template <endianness big_endian, bool inplace>
bool utf16fix_block64(char16_t *out, const char16_t *in) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();

  uint8x16_t illse0 = inplace ? get_mismatch_copy<big_endian, true>(in, out)
                              : get_mismatch_copy<big_endian, false>(in, out);
  uint8x16_t illse1 =
      inplace ? get_mismatch_copy<big_endian, true>(in + 16, out + 16)
              : get_mismatch_copy<big_endian, false>(in + 16, out + 16);
  uint8x16_t illse2 =
      inplace ? get_mismatch_copy<big_endian, true>(in + 32, out + 32)
              : get_mismatch_copy<big_endian, false>(in + 32, out + 32);
  uint8x16_t illse3 =
      inplace ? get_mismatch_copy<big_endian, true>(in + 48, out + 48)
              : get_mismatch_copy<big_endian, false>(in + 48, out + 48);
  // this branch could be marked as unlikely:
  if (veq_non_zero(
          vorrq_u8(vorrq_u8(illse0, illse1), vorrq_u8(illse2, illse3)))) {
    uint64_t matches = get_mask(illse0, illse1, illse2, illse3);
    // Given that ARM has a fast bitreverse instruction, we can
    // reverse once and then use clz to find the first bit set.
    // It is how it is done in simdjson and *might* be beneficial.
    //
    // We might also proceed in reverse to reduce the RAW hazard,
    // but it might require more instructions.

    while (matches != 0) {
      int r = trailing_zeroes(matches); // generates rbit + clz
      // Either we have a high surrogate followed by a non-low surrogate
      // or we have a low surrogate not preceded by a high surrogate.
      bool is_high = scalar::utf16::is_high_surrogate<big_endian>(in[r - 1]);
      out[r - is_high] = replacement;
      matches = clear_least_significant_bit(matches);
    }
    return false;
  }
  return true;
}

template <endianness big_endian>
void utf16fix_neon_64bits(const char16_t *in, size_t n, char16_t *out) {
  size_t i;
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  if (n < 17) {
    return scalar::utf16::to_well_formed_utf16<big_endian>(in, n, out);
  }
  out[0] =
      scalar::utf16::is_low_surrogate<big_endian>(in[0]) ? replacement : in[0];
  i = 1;

  /* duplicate code to have the compiler specialise utf16fix_block() */
  if (in == out) {
    for (i = 1; i + 64 < n; i += 64) {
      utf16fix_block64<big_endian, true>(out + i, in + i);
    }

    for (; i + 16 < n; i += 16) {
      utf16fix_block<big_endian, true>(out + i, in + i);
    }

    /* tbd: find carry */
    utf16fix_block<big_endian, true>(out + n - 16, in + n - 16);
  } else {
    for (i = 1; i + 64 < n; i += 64) {
      utf16fix_block64<big_endian, false>(out + i, in + i);
    }
    for (; i + 16 < n; i += 16) {
      utf16fix_block<big_endian, false>(out + i, in + i);
    }

    utf16fix_block<big_endian, false>(out + n - 16, in + n - 16);
  }
  out[n - 1] = scalar::utf16::is_high_surrogate<big_endian>(out[n - 1])
                   ? replacement
                   : out[n - 1];
}
