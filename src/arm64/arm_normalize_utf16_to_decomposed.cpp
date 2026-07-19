namespace internal {
template <DecomposedForm form>
simdutf_really_inline uint32x4_t arm_trie_lookup_utf16(uint16x4_t code_points) {
  uint32_t buf[4];
  buf[0] = scalar::utf16_to_decomposed::lookup_full_trie<form>(
      vget_lane_u16(code_points, 0));
  buf[1] = scalar::utf16_to_decomposed::lookup_full_trie<form>(
      vget_lane_u16(code_points, 1));
  buf[2] = scalar::utf16_to_decomposed::lookup_full_trie<form>(
      vget_lane_u16(code_points, 2));
  buf[3] = scalar::utf16_to_decomposed::lookup_full_trie<form>(
      vget_lane_u16(code_points, 3));
  return vld1q_u32(buf);
}

simdutf_really_inline bool arm_is_ccc_sorted_u32(uint32x4_t ccc_values,
                                                 uint8_t last_ccc) {
  uint32x4_t shifted_ccc = vextq_u32(vdupq_n_u32(last_ccc), ccc_values, 3);
  uint32x4_t starters = vceqq_u32(ccc_values, vdupq_n_u32(0));
  // We can use the special ccc value 255 for starters.
  uint32x4_t ccc_fixup = vbslq_u32(starters, vdupq_n_u32(255), ccc_values);
  uint32x4_t ccc_lt = vcltq_u32(ccc_fixup, shifted_ccc);
  return vmaxvq_u32(ccc_lt) == 0;
}

// Mask for high surrogates
uint16x8_t arm_make_utf16_surrogates_mask(uint16x8_t in) {
  return vandq_u16(vcleq_u16(in, vdupq_n_u16(0xDBFF)),
                   vcgeq_u16(in, vdupq_n_u16(0xD800)));
}

// Copy the input vector into the output buffer, restoring the destination
// byte order.
template <endianness big_endian>
simdutf_really_inline void arm_skip_decomp_utf16(uint16x8_t in, size_t length,
                                                 char16_t **out,
                                                 uint8_t *last_ccc) {
  *last_ccc = 0;
  if constexpr (!match_system(big_endian)) {
    in = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(in)));
  }
  vst1q_u8(reinterpret_cast<uint8_t *>(*out), vreinterpretq_u8_u16(in));
  *out += length;
}

// Decompose UTF-16 code points that have some number of precomposed Hangul
// syllables in them, but no table-based decompositions.
template <endianness big_endian>
static void arm_write_hangul_utf16(uint16x4_t chars, uint16x4_t relevant,
                                   char16_t **out, const char16_t *input,
                                   uint8_t *last_ccc) {
  *last_ccc = 0;

  // Naively compute Hangul jamo. In practice, if this function is called,
  // most characters will be precomposed syllables (for example, when an
  // ASCII space is present in between some Korean text).
  uint16x4x3_t lvt = arm_compute_hangul_jamo(chars);

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  uint16_t relevant_buf[4];
  uint16_t l_buf[4];
  uint16_t v_buf[4];
  uint16_t t_buf[4];
  vst1_u16(relevant_buf, relevant);
  vst1_u16(l_buf, lvt.val[0]);
  vst1_u16(v_buf, lvt.val[1]);
  vst1_u16(t_buf, lvt.val[2]);
#endif

  for (size_t i = 0; i < 4; i++) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    if (relevant_buf[i] == 0) {
#else
    if (relevant[i] == 0) {
#endif
      // Not a Hangul syllable, just copy the input.
      *(*out)++ = input[0];
      input++;
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

    scalar::utf16::write_code_point<big_endian>(l, *out);
    *out += 1;
    scalar::utf16::write_code_point<big_endian>(v, *out);
    *out += 1;
    // Naively write the T code point, even if it is zero, and branchlessly
    // increment the output pointer if it is non-zero. Although this appears
    // like extra work, it is actually faster than branching on the T code
    // point being zero or not, because the branch miss penalty is quite high.
    scalar::utf16::write_code_point<big_endian>(t, *out);
    *out += (t - scalar::normalization::t_base > 0);
    input += 1;
  }
}

// Decompose UTF-16 code points that have some number of precomposed or
// combining characters in them, but no precomposed Hangul syllables.
template <endianness big_endian, DecomposedForm form>
void arm_write_non_hangul_utf16_fallback(uint32x4_t values, char16_t **out,
                                         size_t out_length,
                                         const char16_t *input,
                                         uint8_t *last_ccc) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  uint32_t values_buf[4];
  vst1q_u32(values_buf, values);
#endif

  for (size_t i = 0; i < 4; i++) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint32_t value = values_buf[i];
#else
    uint32_t value = values[i];
#endif
    if (value == 0) {
      *(*out)++ = input[0];
      input++;
      *last_ccc = 0;
      continue;
    }

    uint8_t ccc = uint8_t(value >> 24);
    uint8_t ccc_delta = uint8_t((value >> 20) & 0b111);
    uint8_t delta = uint8_t((value >> 14) & 0x3F);
    uint8_t length = uint8_t(delta + 1);
    uint16_t offset = value & 0x3FFF;
    const uint16_t *words =
        &tables::utf16_to_decomposed::decompositions[offset];
    uint16x8_t decomp_words = vld1q_u16(words);
    // The table's words are always stored in little endian
    if (big_endian == endianness::BIG) {
      decomp_words =
          vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(decomp_words)));
    }
    vst1q_u16(reinterpret_cast<uint16_t *>(*out), decomp_words);
    if constexpr (form == DecomposedForm::NFKD) {
      if (simdutf_unlikely(length > 8)) {
        for (size_t j = 8; j < length; j++) {
          if constexpr (big_endian == endianness::BIG) {
            (*out)[j] = char16_t(scalar::u16_swap_bytes(words[j]));
          } else {
            (*out)[j] = words[j];
          }
        }
      }
    }
    *out += length;
    out_length += length;

    uint8_t cmp_ccc = ccc_delta > 0 ? uint8_t(ccc - ccc_delta) : ccc;
    if (cmp_ccc != 0 && *last_ccc > cmp_ccc) {
      ccc = scalar::normalization::sort_combining<
          scalar::normalization::utf16_normalization_traits<big_endian>>(
          *out, out_length);
    }
    input += 1;
    *last_ccc = ccc;
  }
}

// Decompose up to four non-Hangul BMP code points into their UTF-16
// representations.
//
// This function assumes that the total decomposition of the (up to) four
// `chars` code points cannot exceed eight code units in size, and that no
// combining characters are out of order.
template <endianness big_endian, DecomposedForm form>
simdutf_really_inline void
arm_write_non_hangul_simple_utf16(uint16x4_t chars, uint32x4_t delta,
                                  uint32x4_t values, char16_t *out) {

  // Get all characters that can be decomposed
  uint32x4_t decomps =
      vcgtq_u32(vandq_u32(values, vdupq_n_u32(1 << 23)), vdupq_n_u32(0));
  // Replace non-decomposable character deltas with 0b11
  uint32x4_t length_fixup = vbslq_u32(decomps, delta, vdupq_n_u32(0b11));
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  const int32x4_t shifts = simdutf_make_int32x4_t(0, 2, 4, 6);
#else
  const int32x4_t shifts = {0, 2, 4, 6};
#endif
  uint32x4_t shifted_lengths = vshlq_u32(length_fixup, shifts);
  // Create a packed integer representing entries of `length_fixup`
  uint32_t packed_shift = vaddvq_u32(shifted_lengths);
  // Load the pre-made index table. In the UTF-8 NF(K)D implementation, we
  // create this index table at runtime. But we can take advantage of the
  // simplicity of UTF-16 to generate all possible index tables at compile time
  // without taking up too much space.
  uint8x16_t index =
      vld1q_u8(simdutf::tables::utf16_to_decomposed::shuf_utf16[packed_shift]);
  // Each 4 word sub-table at position `i` holds the potential decomposition of
  // the character `chars[i / 4]`. There are four 4-word sub-tables because
  // there are four characters.
  uint16_t tbls[4 * 4];
  // Create a bitmask corresponding to `decomps` that we can iterate through
  uint64_t bitmask16 =
      arm_bitmask4(vreinterpretq_u8_u32(decomps)) & 0x8000800080008000ULL;
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  uint32_t values_buf[4];
  vst1q_u32(values_buf, values);
#endif
  for (; bitmask16 > 0; bitmask16 &= bitmask16 - 1) {
    uint32_t i = trailing_zeroes(bitmask16) >> 4;
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint32_t value = values_buf[i];
#else
    uint32_t value = values[i];
#endif
    uint16x4_t words = vld1_u16(
        &simdutf::tables::utf16_to_decomposed::decompositions[value & 0x3FFF]);
    // Table words are stored in little endian, so swap if we need big endian
    if constexpr (big_endian == endianness::BIG) {
      words = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(words)));
    }
    vst1_u16(&tbls[i * 4], words);
  }
  uint8x16_t original =
      vreinterpretq_u8_u16(vcombine_u16(chars, vdup_n_u16(0)));
  if constexpr (!match_system(big_endian)) {
    original = vrev16q_u8(original);
  }
  // Create the table that holds the original bytes and the decomposition
  // sub-tables.
  uint8x16x3_t tbl = {original,
                      vld1q_u8(reinterpret_cast<const uint8_t *>(&tbls[0])),
                      vld1q_u8(reinterpret_cast<const uint8_t *>(&tbls[8]))};
  uint16x8_t decomposed = vreinterpretq_u16_u8(vqtbl3q_u8(tbl, index));
  // Write the decomposition table
  vst1q_u16(reinterpret_cast<uint16_t *>(out), decomposed);
}

// Decompose up to four UTF-16 code points in the BMP. The code points here
// may or may not be Hangul.
template <endianness big_endian, DecomposedForm form>
simdutf_really_inline void
arm_decompose_utf16(uint16x4_t chars, const char16_t *input, char16_t **out,
                    size_t out_length, uint8_t *last_ccc) {
  uint16x4_t hangul_mask = arm_hangul_mask(chars);
  bool hangul_result = vmaxv_u16(hangul_mask) > 0;
  uint32x4_t values = arm_trie_lookup_utf16<form>(chars);
  bool decomp_result = vmaxvq_u32(values) > 0;
  // With no Hangul characters and no decomposable code points, we can skip.
  if (!hangul_result && !decomp_result) {
    *last_ccc = 0;
    uint16x8_t in_dummy = vcombine_u16(chars, vdup_n_u16(0));
    arm_skip_decomp_utf16<big_endian>(in_dummy, 4, out, last_ccc);
    return;
  }
  // Note: we mask 9 bits here, even though the delta only needs 6. This is an
  // optimization that takes advantage of the fact that the ccc_delta bits are
  // right next to the delta. In the event of a non-zero ccc_delta, this 9-bit
  // value will be very large and will cause the total <= 8 check below to
  // fail, putting us in the slow path, which is what we want because non-zero
  // ccc_deltas should put us in the slow path. This means we can avoid
  // explicitly checking ccc_delta bits.
  uint32x4_t delta = vandq_u32(vshrq_n_u32(values, 14), vdupq_n_u32(0x1FF));
  uint32_t total = 4 + vaddvq_u32(delta);
  uint32x4_t ccc_values = vshrq_n_u32(values, 24);
  bool is_sorted = arm_is_ccc_sorted_u32(ccc_values, *last_ccc);
  // There are three conditions in which we enter the slow path: we have a
  // precomposed Hangul character, the total number of code units needed to
  // write the decomposition of the input would be greater than 8, or we've
  // detected that combining characters are out-of-order. All of these
  // conditions are rather uncommon in practice, except for the first one when
  // dealing with Korean text.
  if (simdutf_likely(!hangul_result && total <= 8 && is_sorted)) {
    *last_ccc = uint8_t(vgetq_lane_u32(ccc_values, 3));
    arm_write_non_hangul_simple_utf16<big_endian, form>(chars, delta, values,
                                                        *out);
    *out += total;
  } else if (!hangul_result) {
    arm_write_non_hangul_utf16_fallback<big_endian, form>(
        values, out, out_length, input, last_ccc);
  } else if (hangul_result && !decomp_result) {
    arm_write_hangul_utf16<big_endian>(chars, hangul_mask, out, input,
                                       last_ccc);
  } else {
    // Case where we have both precomposed characters and Hangul syllables.
    // Very rare in practice, so we just fall back to the scalar
    // implementation.
    *out +=
        scalar::utf16_to_decomposed::normalize_with_context<big_endian, form>(
            input, 4, *out, out_length, last_ccc);
  }
}
} // namespace internal

template <endianness big_endian, DecomposedForm form>
size_t arm_normalize_utf16_to_decomposed(const char16_t *input, size_t length,
                                         char16_t *output) {
  char16_t **out_ptr = &output;
  char16_t *start = output;

  const size_t SAFETY_MARGIN = 8;
  uint8_t last_ccc = 0;
  size_t p = 0;
  while (p + SAFETY_MARGIN < length) {
    uint16x8_t in = vld1q_u16(reinterpret_cast<const uint16_t *>(input + p));
    if constexpr (!match_system(big_endian)) {
      in = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(in)));
    }
    // ASCII fast path
    if (vmaxvq_u16(in) <= 0x7F) {
      // Get the next 8 bytes and check if they are also ASCII
      uint16x8_t nextin =
          vld1q_u16(reinterpret_cast<const uint16_t *>(input + p) + 8);
      if constexpr (!match_system(big_endian)) {
        nextin = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(nextin)));
      }
      if (vmaxvq_u16(nextin) > 0x7F) {
        internal::arm_skip_decomp_utf16<big_endian>(in, 8, out_ptr, &last_ccc);
        p += 8;
        in = nextin;
      } else {
        internal::arm_skip_decomp_utf16<big_endian>(in, 8, out_ptr, &last_ccc);
        internal::arm_skip_decomp_utf16<big_endian>(nextin, 8, out_ptr,
                                                    &last_ccc);
        p += 16;
        continue;
      }
    }
    uint16x8_t surrogates_mask = internal::arm_make_utf16_surrogates_mask(in);
    // Check if we have no surrogate pairs
    if (vmaxvq_u16(surrogates_mask) == 0) {
      uint16x4_t in1 = vget_low_u16(in);
      uint16x4_t in2 = vget_high_u16(in);
      // Decompose the low code points and the high code points separately.
      // NOTE: our throughput is most likely much worse by doing this. It would
      // be a good idea to experiment with a narrow/full trie like UTF-8 has so
      // that we can support processing 8 code points cleanly per iteration.
      internal::arm_decompose_utf16<big_endian, form>(
          in1, input + p, out_ptr, *out_ptr - start, &last_ccc);
      internal::arm_decompose_utf16<big_endian, form>(
          in2, input + p + 4, out_ptr, *out_ptr - start, &last_ccc);
    } else {
      // In the case that we do have surrogate pairs, we fall back to a
      // scalar implementation.
      size_t normalize_range = 8;
      if (vgetq_lane_u16(surrogates_mask, 7) == 0xFFFF) {
        // Include the low surrogate in the normalization range
        normalize_range += 1;
      }
      *out_ptr +=
          scalar::utf16_to_decomposed::normalize_with_context<big_endian, form>(
              input + p, normalize_range, *out_ptr, *out_ptr - start,
              &last_ccc);
      p += normalize_range - 8;
    }
    p += 8;
  }

  if (p < length) {
    *out_ptr +=
        scalar::utf16_to_decomposed::normalize_with_context<big_endian, form>(
            input + p, length - p, *out_ptr, *out_ptr - start, &last_ccc);
  }

  return *out_ptr - start;
}

namespace internal {
template <DecomposedForm form>
simdutf_really_inline uint16x8_t
arm_check_trie_lookup_utf16(uint16x8_t code_points) {
  uint16_t buf[8];
  buf[0] = scalar::utf16_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 0));
  buf[1] = scalar::utf16_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 1));
  buf[2] = scalar::utf16_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 2));
  buf[3] = scalar::utf16_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 3));
  buf[4] = scalar::utf16_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 4));
  buf[5] = scalar::utf16_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 5));
  buf[6] = scalar::utf16_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 6));
  buf[7] = scalar::utf16_to_decomposed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 7));
  return vld1q_u16(buf);
}

template <DecomposedForm form>
simdutf_really_inline void
arm_check_code_points_utf16(uint16x8_t code_points, size_t *out_length,
                            bool *is_qc, uint8_t *last_ccc) {
  uint16x8_t values = arm_check_trie_lookup_utf16<form>(code_points);
  *out_length += vaddvq_u16(vandq_u16(values, vdupq_n_u16(0x3F)));
  uint16x8_t ccc_values = vandq_u16(vshrq_n_u16(values, 6), vdupq_n_u16(0xFF));
  if (*is_qc) {
    // Checking combining classes is expensive, so we only do it if we
    // haven't already failed the quick check.
    *is_qc &= !vmaxvq_u16(vshrq_n_u16(values, 15)) &&
              arm_is_ccc_sorted_full(ccc_values, *last_ccc);
  }
  *last_ccc = uint8_t(vgetq_lane_u16(ccc_values, 7));
}
} // namespace internal

template <endianness big_endian, DecomposedForm form>
bool arm_normalize_utf16_to_decomposed_check(const char16_t *input,
                                             size_t length,
                                             size_t *out_length) {
  *out_length = 0;
  uint8_t last_ccc = 0;
  bool is_qc = true;
  const size_t SAFETY_MARGIN = 8;
  size_t p = 0;
  while (p + SAFETY_MARGIN < length) {
    uint16x8_t in = vld1q_u16(reinterpret_cast<const uint16_t *>(input + p));
    if constexpr (!match_system(big_endian)) {
      in = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(in)));
    }
    if (vmaxvq_u16(in) <= 0x7F) {
      // Get the next 8 bytes and check if they are also ASCII
      uint16x8_t nextin =
          vld1q_u16(reinterpret_cast<const uint16_t *>(input + p) + 8);
      if constexpr (!match_system(big_endian)) {
        nextin = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(nextin)));
      }
      if (vmaxvq_u16(nextin) > 0x7F) {
        *out_length += 8;
        p += 8;
        in = nextin;
        last_ccc = 0;
      } else {
        *out_length += 16;
        p += 16;
        last_ccc = 0;
        continue;
      }
    }
    uint16x8_t surrogates_mask = internal::arm_make_utf16_surrogates_mask(in);
    if (vmaxvq_u16(surrogates_mask) == 0) {
      internal::arm_check_code_points_utf16<form>(in, out_length, &is_qc,
                                                  &last_ccc);
    } else {
      size_t normalize_range = 8;
      if (vgetq_lane_u16(surrogates_mask, 7) == 0xFFFF) {
        normalize_range += 1;
      }
      is_qc &=
          scalar::utf16_to_decomposed::check_with_context<big_endian, form>(
              input + p, normalize_range, out_length, &last_ccc);
      p += normalize_range - 8;
    }
    p += 8;
  }
  if (p < length) {
    is_qc &= scalar::utf16_to_decomposed::check_with_context<big_endian, form>(
        input + p, length - p, out_length, &last_ccc);
  }
  return is_qc;
}
