namespace internal {
simdutf_really_inline uint8_t arm_movemask_u16(uint16x4_t v) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  const uint16x4_t mask = simdutf_make_uint16x4_t(0x1, 0x2, 0x4, 0x8);
#else
  const uint16x4_t mask = {0x1, 0x2, 0x4, 0x8};
#endif
  uint16x4_t mv = vand_u16(v, mask);
  return (uint8_t)(vaddv_u16(mv) & 0xF);
}

// Write 8 code points, assuming they all expand to three bytes.
void arm_write_8_3_byte_utf8(uint16x8_t in, uint8_t *out) {
  uint8x8x3_t bytes;

  // 1110xxxxxxxxxxxx
  uint16x8_t high = vsriq_n_u16(vdupq_n_u16(0xE000), in, 4);
  // 1110xxxx
  uint8x8_t high_narrow = vshrn_n_u16(high, 8);
  bytes.val[0] = high_narrow;

  // xxxxxxxx
  uint8x8_t middle = vshrn_n_u16(in, 6);
  // 00xxxxxx
  uint8x8_t middle_cleared = vand_u8(middle, vdup_n_u8(0b00111111));
  // 10xxxxxx
  bytes.val[1] = vorr_u8(middle_cleared, vdup_n_u8(0b10000000));

  // 0000000000xxxxxx
  uint16x8_t low = vandq_u16(in, vdupq_n_u16(0b00111111));
  uint8x8_t low_narrow = vmovn_u16(low);
  // 10xxxxxx
  bytes.val[2] = vorr_u8(low_narrow, vdup_n_u8(0b10000000));

  // Interleaved store into output
  vst3_u8(out, bytes);
}

void arm_decompose_all_hangul_utf8(uint16x4_t values, uint8_t **out,
                                   uint8_t *last_ccc) {
  // Hangul jamo are not combining characters
  *last_ccc = 0;

  uint16x4x3_t lvt = arm_compute_hangul_jamo(values);

  // Only 12 of the 16 uint16_t's will be used
  uint16_t tmp[16];
  // Interleave store by three, creating a code point buffer assuming
  // all precomposed Hangul characters decompose into three Hangul
  // syllables each.
  vst3_u16(tmp, lvt);

  uint16x4_t t =
      vsub_u16(lvt.val[2], vdup_n_u16(scalar::normalization::t_base));
  // Mask for all precomposed Hangul syllables that should not have a
  // trailing consonant
  uint16x4_t t_mask = vceqz_u16(t);
  uint8_t bitmask = arm_movemask_u16(t_mask);
  // Use the trailing consonant bitmask to get a shuffle vector
  auto shuf = simdutf::tables::utf8_to_decomposed::pack_hangul[bitmask];

  // Load the tmp buffer into a large byte table
  uint8x16_t tbl_low = vreinterpretq_u8_u16(vld1q_u16(tmp));
  uint8x16_t tbl_high = vreinterpretq_u8_u16(vld1q_u16(tmp + 8));
  uint8x16x2_t tbl;
  tbl.val[0] = tbl_low;
  tbl.val[1] = tbl_high;

  // Use the shuffle vector to reorder the syllables so that it (possibly)
  // corrects the previous code that assumed all characters decompose into
  // three syllables.
  //
  // NOTE: possible fast path: skip this if bitmask == 0b1111.
  uint8x16_t idx_low = vld1q_u8(shuf.tbl);
  uint16x8_t low = vreinterpretq_u16_u8(vqtbl2q_u8(tbl, idx_low));
  uint8x8_t idx_high = vld1_u8(shuf.tbl + 16);
  uint16x4_t high = vreinterpret_u16_u8(vqtbl2_u8(tbl, idx_high));

  arm_write_8_3_byte_utf8(low, *out);
  *out += 24;
  if (shuf.len > 24) {
    arm_write_8_3_byte_utf8(vcombine_u16(high, vdup_n_u16(0)), *out);
    *out += shuf.len - 24;
  }
}

simdutf_really_inline uint16x8_t arm_prefix_sum_uint16x8(uint16x8_t v) {
  uint16x8_t t;
  t = vextq_u16(vdupq_n_u16(0), v, 7);
  v = vaddq_u16(v, t);
  t = vextq_u16(vdupq_n_u16(0), v, 6);
  v = vaddq_u16(v, t);
  t = vextq_u16(vdupq_n_u16(0), v, 4);
  v = vaddq_u16(v, t);
  return v;
}

simdutf_really_inline bool arm_is_ccc_sorted_full(uint16x8_t ccc_values,
                                                  uint8_t last_ccc) {
  uint16x8_t shifted_ccc = vextq_u16(vdupq_n_u16(last_ccc), ccc_values, 7);
  uint16x8_t starters = vceqq_u16(ccc_values, vdupq_n_u16(0));
  // We can use the special ccc value 255 for starters
  uint16x8_t ccc_fixup = vbslq_u16(starters, vdupq_n_u16(255), ccc_values);
  uint16x8_t ccc_lt = vcltq_u16(ccc_fixup, shifted_ccc);
  return vmaxvq_u16(ccc_lt) == 0;
}

simdutf_really_inline bool arm_is_ccc_sorted(uint16x4_t ccc_values,
                                             uint8_t last_ccc) {
  uint16x4_t shifted_ccc = vext_u16(vdup_n_u16(last_ccc), ccc_values, 3);
  uint16x4_t starters = vceq_u16(ccc_values, vdup_n_u16(0));
  uint16x4_t ccc_fixup = vbsl_u16(starters, vdup_n_u16(255), ccc_values);
  uint16x4_t ccc_lt = vclt_u16(ccc_fixup, shifted_ccc);
  return vmaxv_u16(ccc_lt) == 0;
}

void arm_decompose_hangul_utf8(uint16x4_t chars, uint16x4_t relevant,
                               uint16x4_t values, uint8_t **out,
                               const uint8_t *input, uint8_t *last_ccc) {
  *last_ccc = 0;
  // Decompose everything (assuming they're all Hangul syllables). This
  // assumption is made because, empirically, most of the time this function is
  // called, it is because there is a single ASCII space in the input vector,
  // which causes a branch miss for the pure Hangul code path, and puts is in
  // this path. Therefore, we can still eagerly compute the Hangul jamo values,
  // and then only write the relevant ones.
  uint16x4x3_t lvt = arm_compute_hangul_jamo(chars);

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  uint16_t relevant_buf[4];
  uint16_t values_buf[4];
  uint16_t l_buf[4];
  uint16_t v_buf[4];
  uint16_t t_buf[4];
  vst1_u16(relevant_buf, relevant);
  vst1_u16(values_buf, values);
  vst1_u16(l_buf, lvt.val[0]);
  vst1_u16(v_buf, lvt.val[1]);
  vst1_u16(t_buf, lvt.val[2]);
#endif

  for (size_t i = 0; i < 4; i++) {
    // ASCII fast path
    if (input[0] <= 0x7F) {
      *(*out)++ = input[0];
      input++;
      continue;
    }
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    if (relevant_buf[i] == 0) {
      // Not a Hangul syllable, just copy the input.
      uint16_t value = values_buf[i];
#else
    if (relevant[i] == 0) {
      // Not a Hangul syllable, just copy the input.
      uint16_t value = values[i];
#endif
      size_t size = value & 0b11;
      for (size_t j = 0; j < size; j++) {
        *(*out)++ = input[j];
      }
      input += size;
      continue;
    }

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint16_t l = l_buf[i];
    uint16_t v = v_buf[i];
    uint16_t t = t_buf[i];
#else
    uint16_t l = lvt.val[0][i];
    uint16_t v = lvt.val[1][i];
    uint16_t t = lvt.val[2][i];
#endif

    scalar::utf8::write_3_byte_code_point(l, reinterpret_cast<char *>(*out));
    *out += 3;
    scalar::utf8::write_3_byte_code_point(v, reinterpret_cast<char *>(*out));
    *out += 3;
    // Naively write the T code point, even if it is zero, and branchlessly
    // increment the output pointer if it is non-zero. Although this appears
    // like extra work, it is actually faster than branching on the T code point
    // being zero or not, because the branch miss penalty is quite high.
    scalar::utf8::write_3_byte_code_point(t, reinterpret_cast<char *>(*out));
    *out += 3 * (t - scalar::normalization::t_base > 0);
    input += 3;
  }
}

template <DecomposedForm form>
simdutf_really_inline uint16x4_t arm_trie_lookup(uint16x4_t code_points) {
  // Neon has no gather instruction, so just do it in a scalar register
  uint16_t buf[4];
  buf[0] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vget_lane_u16(code_points, 0));
  buf[1] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vget_lane_u16(code_points, 1));
  buf[2] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vget_lane_u16(code_points, 2));
  buf[3] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vget_lane_u16(code_points, 3));
  return vld1_u16(buf);
}

template <DecomposedForm form>
simdutf_really_inline uint16x8_t arm_trie_lookup_wide(uint16x8_t code_points) {
  uint16_t buf[8];
  buf[0] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vgetq_lane_u16(code_points, 0));
  buf[1] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vgetq_lane_u16(code_points, 1));
  buf[2] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vgetq_lane_u16(code_points, 2));
  buf[3] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vgetq_lane_u16(code_points, 3));
  buf[4] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vgetq_lane_u16(code_points, 4));
  buf[5] = scalar::utf8_to_decomposed::lookup_narrow_trie<form>(
      vgetq_lane_u16(code_points, 5));
  buf[6] = 0;
  buf[7] = 0;
  return vld1q_u16(buf);
}

// Decompose up to eight code points into their UTF-8 representations. This
// function assumes that the input code points are not Hangul syllables.
template <DecomposedForm form>
void arm_write_non_hangul_fallback(uint16x8_t values, uint16x8_t chars,
                                   uint8_t n_chars, uint8_t **out,
                                   size_t out_length, const uint8_t *input,
                                   uint8_t *last_ccc) {
  uint8_t *start = *out;

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  uint16_t values_buf[8];
  uint16_t chars_buf[8];
  vst1q_u16(values_buf, values);
  vst1q_u16(chars_buf, chars);
#endif

  for (size_t i = 0; i < n_chars; i++) {
    uint8_t leading = input[0];
    // ASCII code point, no decomposition needed.
    if (leading <= 0x7F) {
      *(*out)++ = leading;
      input++;
      *last_ccc = 0;
      continue;
    }

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint16_t value = values_buf[i];
#else
    uint16_t value = values[i];
#endif
    uint8_t size = value & 0b11;
    if (value <= 3) {
      vst1_u8(*out, vld1_u8(input));
      *out += size;
      input += size;
      *last_ccc = 0;
      continue;
    }

    // `ccc` represents the combining class of the last character in the
    // decomposition of the character we're on, not the actual ccc value of
    // the character.
    uint8_t ccc = uint8_t((value >> 2) & 0xFF);

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint32_t data =
        scalar::utf8_to_decomposed::lookup_full_trie<form>(chars_buf[i]);
#else
    uint32_t data =
        scalar::utf8_to_decomposed::lookup_full_trie<form>(chars[i]);
#endif
    uint16_t offset = data & 0x7FFF;
    uint8_t length = (data >> 15) & 0x3F;
    uint8_t first_ccc_delta = uint8_t(data >> 29);

    const uint8_t *decomp_offset =
        &tables::utf8_to_decomposed::decompositions[offset];
    vst1q_u8(*out, vld1q_u8(decomp_offset));
    // NFKD decompositions can get very large (check out 0xFDFA!).
    if constexpr (form == DecomposedForm::NFKD) {
      if (simdutf_unlikely(length > 16)) {
        vst1q_u8(*out + 16, vld1q_u8(decomp_offset + 16));
        for (size_t j = 32; j < length; j++) {
          (*out)[j] = decomp_offset[j];
        }
      }
    }
    *out += length;

    uint8_t cmp_ccc = first_ccc_delta > 0 ? ccc - first_ccc_delta : ccc;
    if (cmp_ccc != 0 && *last_ccc > cmp_ccc) {
      ccc = scalar::normalization::sort_combining<
          scalar::normalization::utf8_normalization_traits>(
          reinterpret_cast<char *>(*out), out_length + (*out - start));
    }
    input += size;
    *last_ccc = ccc;
  }
}

// Decompose code points into their UTF-8 representations.
//
// This function assumes that the input code points are not Hangul syllables
// and that they are "simple". In particular, this means the total
// decomposition of the (up to) six `chars` code points cannot exceed 16
// bytes in size. Also, no combining characters can be out of order.
template <DecomposedForm form>
simdutf_really_inline void
arm_write_non_hangul_simple_utf8(uint8x16_t in, uint16x8_t chars,
                                 int16x8_t delta, uint16x8_t values,
                                 uint8_t *out) {
  // UTF-8 lengths of each code point in the input.
  uint16x8_t length = vandq_u16(values, vdupq_n_u16(0b11));
  uint16x8_t length_psum = arm_prefix_sum_uint16x8(length);
  int8x16_t shift = vdupq_n_s8(0);
  // Each 8-byte block corresponds to one decomposition in the input. It is
  // only possible to have a maximum of six code points.
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  int8x16_t iota = simdutf_make_int8x16_t(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                          12, 13, 14, 15);
#else
  int8x16_t iota = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
#endif
  // Table of decomposition bytes for every code point with a decomposition.
  uint8_t tbls[6 * 8];
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  int16_t delta_buf[8];
  uint16_t length_buf[8];
  uint16_t length_psum_buf[8];
  uint16_t chars_buf[8];
  vst1q_s16(delta_buf, delta);
  vst1q_u16(length_buf, length);
  vst1q_u16(length_psum_buf, length_psum);
  vst1q_u16(chars_buf, chars);
#endif
  uint16x8_t decomps = vcgtq_u16(values, vdupq_n_u16(0x3FF));
  // We can iterate through this bitmask to get the positions of all code
  // points we should decompose.
  uint64_t bitmask8 =
      arm_bitmask4(vreinterpretq_u8_u16(decomps)) & 0x8080808080808080ULL;
  // Keeps track of which sub-table we're writing to.
  uint8_t j = 0;
  // We could use a prefix sum involving `delta` to compute displacement as a
  // vector, but going scalar here is better because the loop below usually
  // executes only one or two times.
  int8_t displacement = 0;
  // Iterate through the 1 bits of `bitmask8`.
  // NOTE: I'd like a way to build the `shift` vector with fewer instructions
  // per decomposable code point. We could also try getting rid of loop
  // dependent state (displacement and j) here, which is possible both using
  // SIMD or pre-computing these values with a tiny loop.
  for (; bitmask8 > 0; bitmask8 &= bitmask8 - 1) {
    // We have 7 redundant bits per useful 1 bit (masked out above, but still
    // present), so divide them out here.
    uint32_t i = trailing_zeroes(bitmask8) >> 3;
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    int8_t dlt = int8_t(delta_buf[i]);
    // The end of each code point before displacement.
    int8_t size = int8_t(length_buf[i]);
    int8_t end = int8_t(length_psum_buf[i]);
#else
    int8_t dlt = int8_t(delta[i]);
    // The end of each code point before displacement.
    int8_t size = int8_t(length[i]);
    int8_t end = int8_t(length_psum[i]);
#endif
    // The start of each code point after displacement. To decompose the
    // code point at `i`, we need to shift the bytes in the buffer by the
    // amount the original code point expands during decomposition. This
    // mask tells us which bytes to shift.
    int8_t dlt_start = int8_t((end - size) + displacement);
    uint8x16_t shift_mask =
        vcgeq_s8(iota, vdupq_n_s8(int8_t(dlt_start + size + dlt)));
    uint8x16_t upper_mask =
        vcltq_s8(iota, vdupq_n_s8(int8_t(end + displacement + dlt)));
    uint8x16_t lower_mask = vcgeq_s8(iota, vdupq_n_s8(dlt_start));
    // Shift by `dlt`.
    uint8x16_t decomp_mask = vandq_u8(upper_mask, lower_mask);
    int8x16_t contrib =
        vandq_s8(vreinterpretq_s8_u8(shift_mask), vdupq_n_s8(dlt));
    // Performing `iota - tbl_offset` should get us an index into the
    // appropriate section of `tbls`, given by the formula below.
    int8_t offset_diff = int8_t(-((int16_t)((j + 2) * 8) - dlt_start));
    int8x16_t tbl_offset = vdupq_n_s8(offset_diff);
    // For our decomposition, select from `tbl_offset`, not the shift.
    shift = vbslq_s8(decomp_mask, tbl_offset, vaddq_s8(shift, contrib));
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint16_t code_point = chars_buf[i];
#else
    uint16_t code_point = chars[i];
#endif
    uint32_t value =
        scalar::utf8_to_decomposed::lookup_full_trie<form>(code_point);
    vst1_u8(
        &tbls[j * 8],
        vld1_u8(&tables::utf8_to_decomposed::decompositions[value & 0x7FFF]));
    j++;
    displacement += dlt;
  }
  uint8x16x4_t tbl = {in, vld1q_u8(&tbls[0]), vld1q_u8(&tbls[16]),
                      vld1q_u8(&tbls[32])};
  uint8x16_t index = vreinterpretq_u8_s8(vsubq_s8(iota, shift));
  uint8x16_t decomposed = vqtbl4q_u8(tbl, index);
  vst1q_u8(out, decomposed);
}

// Decompose input code points, assuming they are not precomposed Hangul
// syllables. `n_bytes` is the number of bytes that the six `chars` code
// points occupy within the 16-byte `in` vector.
template <DecomposedForm form>
simdutf_really_inline void
arm_decompose_non_hangul_utf8(uint8x16_t in, uint16x8_t chars, size_t n_bytes,
                              const uint8_t *input, uint8_t **out,
                              size_t out_length, uint8_t *last_ccc) {
  // Each value contains the UTF-8 code point size (from 0 to 3) in the
  // lowest two bits. If there's nothing special about the code point, it
  // will have zero bits past those two bits.
  uint16x8_t values = arm_trie_lookup_wide<form>(chars);
  if (vmaxvq_u16(values) <= 0b11) {
    *last_ccc = 0;
    vst1q_u8(*out, in);
    *out += n_bytes;
    return;
  }
  int16x8_t delta = vshrq_n_s16(vreinterpretq_s16_u16(values), 11);
  int16_t total = (int16_t)n_bytes + vaddvq_s16(delta);
  uint16x8_t ccc_values = vandq_u16(vshrq_n_u16(values, 2), vdupq_n_u16(0xFF));
  bool is_sorted = arm_is_ccc_sorted_full(ccc_values, *last_ccc);
  // There are two conditions in which we enter the slow path: the total
  // number of bytes needed to write the decomposition of the input would be
  // greater than 16, or we've detected that combining characters are
  // out-of-order. Both conditions are rather uncommon in practice.
  if (simdutf_likely(total <= 16 && is_sorted)) {
    *last_ccc = uint8_t(vgetq_lane_u16(ccc_values, 5));
    arm_write_non_hangul_simple_utf8<form>(in, chars, delta, values, *out);
    *out += total;
  } else {
    arm_write_non_hangul_fallback<form>(values, chars, 6, out, out_length,
                                        input, last_ccc);
  }
}

// Generalized decomposition for a 16-byte input vector of UTF-8 code
// points. The `chars` parameter is a 4x16-bit vector of BMP code points,
// and `n_bytes` indicates how many bytes of the input vector are used for
// `chars`. `input` is the original pointer to UTF-8 bytes.
//
// The code points here may or may not be Hangul. A faster variant of this
// function is available if Hangul cannot be present.
template <DecomposedForm form>
simdutf_really_inline void
arm_decompose_utf8(uint8x16_t in, uint16x4_t chars, size_t n_bytes,
                   const uint8_t *input, uint8_t **out, size_t out_length,
                   uint8_t *last_ccc) {
  uint16x4_t hangul_mask = arm_hangul_mask(chars);
  bool hangul_result = vmaxv_u16(hangul_mask) > 0;
  uint16x4_t values = arm_trie_lookup<form>(chars);
  // Case where we have no Hangul syllables and no relevant characters.
  bool decomp_result = vmaxv_u16(values) > 3;
  if (!hangul_result && !decomp_result) {
    *last_ccc = 0;
    vst1q_u8(*out, in);
    *out += n_bytes;
    return;
  }
  int16x4_t delta = vshr_n_s16(vreinterpret_s16_u16(values), 11);
  int16_t total = (int16_t)n_bytes + vaddv_s16(delta);
  uint16x4_t ccc_values = vand_u16(vshr_n_u16(values, 2), vdup_n_u16(0xFF));
  bool is_sorted = arm_is_ccc_sorted(ccc_values, *last_ccc);
  if (!hangul_result && total <= 16 && is_sorted) {
    *last_ccc = uint8_t(vget_lane_u16(ccc_values, 3));
    arm_write_non_hangul_simple_utf8<form>(
        in, vcombine_u16(chars, vdup_n_u16(0)),
        vcombine_s16(delta, vdup_n_s16(0)), vcombine_u16(values, vdup_n_u16(0)),
        *out);
    *out += total;
  } else if (!hangul_result) {
    arm_write_non_hangul_fallback<form>(vcombine_u16(values, vdup_n_u16(0)),
                                        vcombine_u16(chars, vdup_n_u16(0)), 4,
                                        out, out_length, input, last_ccc);
  } else if (hangul_result && !decomp_result) {
    arm_decompose_hangul_utf8(chars, hangul_mask, values, out, input, last_ccc);
  } else {
    // Case where we have both precomposed characters and Hangul syllables.
    // Very rare in practice, so we just fall back to the scalar
    // implementation.
    *out += scalar::utf8_to_decomposed::normalize_with_context<form>(
        reinterpret_cast<const char *>(input), n_bytes,
        reinterpret_cast<char *>(*out), out_length, last_ccc);
  }
}

// Decompose six non-Hangul BMP code points into their UTF-8 representations.
// This function is specially optimized for inputs where the total number of
// bytes spanned by `chars` is small (we currently use <= 8). It functions
// exactly the same as `arm_decompose_non_hangul_utf8`. But note that this
// function is slower when the input is not primarily comprised of ASCII.
template <DecomposedForm form>
simdutf_really_inline void
arm_decompose_small_utf8(uint16x8_t chars, const uint8_t *input, uint8_t **out,
                         size_t out_length, uint8_t *last_ccc) {
  uint8_t *start = *out;
#if SIMDUTF_REGULAR_VISUAL_STUDIO
  uint16_t chars_buf[8];
  vst1q_u16(chars_buf, chars);
#endif
  for (uint8_t i = 0; i < 6; i++) {
    uint8_t leading = input[0];
    if (simdutf_likely(leading <= 0x7F)) {
      *(*out)++ = leading;
      input++;
      *last_ccc = 0;
      continue;
    }

#if SIMDUTF_REGULAR_VISUAL_STUDIO
    uint16_t c = chars_buf[i];
#else
    uint16_t c = chars[i];
#endif
    uint32_t value = scalar::utf8_to_decomposed::lookup_full_trie<form>(c);
    if (value == 0) {
      *(*out)++ = leading;
      *(*out)++ = input[1];
      // Non-ASCII code points are guaranteed to be 2-byte in this function
      input += 2;
      *last_ccc = 0;
      continue;
    }

    uint16_t offset = value & 0x7FFF;
    uint8_t length = (value >> 15) & 0x3F;
    uint8_t ccc = (value >> 21) & 0xFF;
    uint8_t first_ccc_delta = uint8_t(value >> 29);

    const uint8_t *decomp_offset =
        &simdutf::tables::utf8_to_decomposed::decompositions[offset];
    vst1q_u8(*out, vld1q_u8(decomp_offset));
    if constexpr (form == DecomposedForm::NFKD) {
      if (simdutf_unlikely(length > 16)) {
        vst1q_u8(*out + 16, vld1q_u8(decomp_offset + 16));
        for (size_t j = 32; j < length; j++) {
          (*out)[j] = decomp_offset[j];
        }
      }
    }
    *out += length;
    uint8_t cmp_ccc = first_ccc_delta > 0 ? ccc - first_ccc_delta : ccc;
    if (cmp_ccc != 0 && *last_ccc > cmp_ccc) {
      ccc = scalar::normalization::sort_combining<
          scalar::normalization::utf8_normalization_traits>(
          reinterpret_cast<char *>(*out), out_length + (*out - start));
    }
    input += 2;
    *last_ccc = ccc;
  }
}
} // namespace internal

// Normalize up to 16 bytes of UTF-8 using an end-of-code-point mask. Returns
// the number of bytes consumed.
template <DecomposedForm form>
size_t normalize_masked_utf8_to_decomposed(const uint8_t *input, uint64_t mask,
                                           uint8_t **out, size_t out_length,
                                           uint8_t *last_ccc) {
  // Count trailing ones to get the number of ASCII bytes at the start of input.
  // We skip ASCII eagerly because, even if the number of ASCII bytes is small,
  // benchmarks show that the cost of falling into the slow path for a
  // majority-ASCII input vector is quite high, especially for heavily Latin
  // alphabetic languages broken up by occasional diacritics, such as Spanish or
  // French.
  int t1 = trailing_zeroes(~mask);
  if (t1 > 2) {
    size_t min = t1 > 52 ? 52 : t1;
    arm_memcpy_small(*out, input);
    *out += min;
    *last_ccc = 0;
    return min;
  }

  uint8x16_t in = vld1q_u8(input);
  uint16_t sml_mask = mask & 0xFFF;

  // Fast path for four 3-byte code points.
  if (sml_mask == 0x924) {
    uint16x4_t chars = arm_parse_3_byte_utf8(in);
    uint16_t min = vminv_u16(chars);
    uint16_t max = vmaxv_u16(chars);

    // Precomposed Hangul range. Characters in this range are
    // algorithmically decomposable with a few arithmetic operations. They
    // are the only precomposed characters we can decompose without a table
    // lookup.
    //
    // Algorithm described here:
    // https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G59401
    if (min >= scalar::normalization::s_base &&
        max < scalar::normalization::s_base + scalar::normalization::s_count) {
      internal::arm_decompose_all_hangul_utf8(chars, out, last_ccc);
      return 12;
    }

    // Fallback path for four 3-byte characters.
    internal::arm_decompose_utf8<form>(in, chars, 12, input, out, out_length,
                                       last_ccc);
    return 12;
  }

  // Six two-byte code points.
  if (sml_mask == 0xAAA) {
    // Precomposed Hangul syllables are not possible in 2-byte code points.
    uint16x8_t chars = arm_parse_2_byte_utf8(in);
    internal::arm_decompose_non_hangul_utf8<form>(in, chars, 12, input, out,
                                                  out_length, last_ccc);
    return 12;
  }

  uint8_t idx = simdutf::tables::utf8_to_utf16::utf8bigindex[sml_mask][0];
  uint8_t n_bytes = simdutf::tables::utf8_to_utf16::utf8bigindex[sml_mask][1];

  if (idx < 22) {
    // Six one to two byte code points totaling <= 8 bytes in size
    uint16x8_t chars = arm_parse_6_12_utf8(in, idx);
    internal::arm_decompose_small_utf8<form>(chars, input, out, out_length,
                                             last_ccc);
  } else if (idx < 64) {
    // Six one to two byte code points. Precomposed Hangul syllables are
    // not possible in one to two byte code points.
    uint16x8_t chars = arm_parse_6_12_utf8(in, idx);
    internal::arm_decompose_non_hangul_utf8<form>(in, chars, n_bytes, input,
                                                  out, out_length, last_ccc);
  } else if (idx < 145) {
    // Four code points.
    uint16x4_t chars = arm_parse_4_123_utf8(in, idx);
    internal::arm_decompose_utf8<form>(in, chars, n_bytes, input, out,
                                       out_length, last_ccc);
  } else if (idx < 209) {
    // NOTE: right now, anytime we have three 1..4-byte code points, we
    // just fall back to scalar. We should not do this.
    *out += scalar::utf8_to_decomposed::normalize_with_context<form>(
        reinterpret_cast<const char *>(input), n_bytes,
        reinterpret_cast<char *>(*out), out_length, last_ccc);
  }

  return n_bytes;
}

namespace internal {
template <DecomposedForm form>
simdutf_really_inline uint16x4_t arm_check_trie_lookup(uint16x4_t code_points) {
  uint16_t buf[4];
  buf[0] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vget_lane_u16(code_points, 0));
  buf[1] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vget_lane_u16(code_points, 1));
  buf[2] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vget_lane_u16(code_points, 2));
  buf[3] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vget_lane_u16(code_points, 3));
  return vld1_u16(buf);
}

template <DecomposedForm form>
simdutf_really_inline uint16x8_t
arm_check_trie_lookup_wide(uint16x8_t code_points) {
  uint16_t buf[8];
  buf[0] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 0));
  buf[1] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 1));
  buf[2] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 2));
  buf[3] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 3));
  buf[4] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 4));
  buf[5] = scalar::utf8_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 5));
  buf[6] = 0;
  buf[7] = 0;
  return vld1q_u16(buf);
}

template <DecomposedForm form>
simdutf_really_inline void
arm_check_code_points_utf8(uint16x4_t code_points, size_t *out_length,
                           bool *is_qc, uint8_t *last_ccc) {
  uint16x4_t values = arm_check_trie_lookup<form>(code_points);
  *out_length += vaddv_u16(vand_u16(values, vdup_n_u16(0x3F)));
  uint16x4_t ccc_values = vand_u16(vshr_n_u16(values, 6), vdup_n_u16(0xFF));
  if (*is_qc) {
    // Checking combining classes is expensive, so we only do it if we
    // haven't already failed the quick check.
    *is_qc &= !vmaxv_u16(vshr_n_u16(values, 15)) &&
              arm_is_ccc_sorted(ccc_values, *last_ccc);
  }
  *last_ccc = uint8_t(vget_lane_u16(ccc_values, 3));
}

template <DecomposedForm form>
simdutf_really_inline void
arm_check_code_points_utf8_wide(uint16x8_t code_points, size_t *out_length,
                                bool *is_qc, uint8_t *last_ccc) {
  uint16x8_t values = arm_check_trie_lookup_wide<form>(code_points);
  *out_length += vaddvq_u16(vandq_u16(values, vdupq_n_u16(0x3F)));
  uint16x8_t ccc_values = vandq_u16(vshrq_n_u16(values, 6), vdupq_n_u16(0xFF));
  if (*is_qc) {
    *is_qc &= !vmaxvq_u16(vshrq_n_u16(values, 15)) &&
              arm_is_ccc_sorted_full(ccc_values, *last_ccc);
  }
  *last_ccc = uint8_t(vgetq_lane_u16(ccc_values, 5));
}
} // namespace internal

template <DecomposedForm form>
size_t
normalize_masked_utf8_to_decomposed_check(const uint8_t *input, uint64_t mask,
                                          size_t *out_length, bool *is_qc,
                                          uint8_t *last_ccc) {
  // Count trailing ones to get the number of ASCII bytes at the start of
  // input.
  int t1 = trailing_zeroes(~mask);
  if (t1 > 0) {
    size_t min = t1 > 52 ? 52 : t1;
    *out_length += min;
    *last_ccc = 0;
    return min;
  }
  uint8x16_t in = vld1q_u8(input);
  uint16_t sml_mask = mask & 0xFFF;

  // Fast path for four 3-byte code points.
  if (sml_mask == 0x924) {
    uint16x4_t code_points = arm_parse_3_byte_utf8(in);
    internal::arm_check_code_points_utf8<form>(code_points, out_length, is_qc,
                                               last_ccc);
    return 12;
  }
  // Six two-byte code points.
  if (sml_mask == 0xAAA) {
    uint16x8_t code_points = arm_parse_2_byte_utf8(in);
    internal::arm_check_code_points_utf8_wide<form>(code_points, out_length,
                                                    is_qc, last_ccc);
    return 12;
  }

  uint8_t idx = simdutf::tables::utf8_to_utf16::utf8bigindex[sml_mask][0];
  uint8_t n_bytes = simdutf::tables::utf8_to_utf16::utf8bigindex[sml_mask][1];
  if (idx < 64) {
    // Six one to two byte code points.
    uint16x8_t code_points = arm_parse_6_12_utf8(in, idx);
    internal::arm_check_code_points_utf8_wide<form>(code_points, out_length,
                                                    is_qc, last_ccc);
    return n_bytes;
  }
  if (idx < 145) {
    // Four code points.
    uint16x4_t code_points = arm_parse_4_123_utf8(in, idx);
    internal::arm_check_code_points_utf8<form>(code_points, out_length, is_qc,
                                               last_ccc);
    return n_bytes;
  }
  if (idx < 209) {
    // Three 1..4-byte code points.
    // It might be safe to call arm_check_code_points_utf8 and subtract 1 byte
    // from out_length after, since we can put a null code point in the last
    // position. NOTE: last_ccc would also have to be adjusted.
    *is_qc &= scalar::utf8_to_decomposed::check_with_context<form>(
        reinterpret_cast<const char *>(input), n_bytes, out_length, last_ccc);
  }
  return n_bytes;
}
