namespace internal {
template <ComposedForm form>
simdutf_really_inline uint16x4_t arm_comp_trie_lookup(uint16x4_t code_points) {
  uint16_t buf[4];
  buf[0] = scalar::normalization::lookup_comp_trie<form>(
      vget_lane_u16(code_points, 0));
  buf[1] = scalar::normalization::lookup_comp_trie<form>(
      vget_lane_u16(code_points, 1));
  buf[2] = scalar::normalization::lookup_comp_trie<form>(
      vget_lane_u16(code_points, 2));
  buf[3] = scalar::normalization::lookup_comp_trie<form>(
      vget_lane_u16(code_points, 3));
  return vld1_u16(buf);
}

template <ComposedForm form>
simdutf_really_inline uint16x8_t
arm_comp_trie_lookup_wide(uint16x8_t code_points) {
  uint16_t buf[8];
  buf[0] = scalar::normalization::lookup_comp_trie<form>(
      vgetq_lane_u16(code_points, 0));
  buf[1] = scalar::normalization::lookup_comp_trie<form>(
      vgetq_lane_u16(code_points, 1));
  buf[2] = scalar::normalization::lookup_comp_trie<form>(
      vgetq_lane_u16(code_points, 2));
  buf[3] = scalar::normalization::lookup_comp_trie<form>(
      vgetq_lane_u16(code_points, 3));
  buf[4] = scalar::normalization::lookup_comp_trie<form>(
      vgetq_lane_u16(code_points, 4));
  buf[5] = scalar::normalization::lookup_comp_trie<form>(
      vgetq_lane_u16(code_points, 5));
  buf[6] = 0;
  buf[7] = 0;
  return vld1q_u16(buf);
}

// Write four BMP code points that have value 0 or 1 from the NF(K)C trie. This
// means that they do not compose with anything, so this function essentially
// performs NF(K)D on the given code points.
template <ComposedForm form>
void arm_write_no_comp_utf8(uint16x8_t values, uint16x8_t code_points,
                            size_t nchars, uint8_t **out, size_t out_length,
                            const uint8_t *input, uint8_t *last_ccc) {
  constexpr auto dform = to_decomposed_form(form);
  uint8_t *start = *out;
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  uint16_t values_buf[8];
  uint16_t code_points_buf[8];
  vst1q_u16(values_buf, values);
  vst1q_u16(code_points_buf, code_points);
#endif

  for (size_t i = 0; i < nchars; i++) {
    uint8_t leading = input[0];
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
    uint8_t size = utf8_size[leading];
    if (value == 0) {
      vst1_u8(*out, vld1_u8(input));
      *out += size;
      input += size;
      *last_ccc = 0;
      continue;
    }
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint16_t code_point = code_points_buf[i];
#else
    uint16_t code_point = code_points[i];
#endif
    // Decompose the code point like we would in NF(K)D.
    uint32_t decomp_value =
        scalar::utf8_to_decomposed::lookup_full_trie<dform>(code_point);
    const uint8_t *decomp_offset =
        &tables::utf8_to_decomposed::decompositions[decomp_value & 0x7FFF];
    uint8_t length = (decomp_value >> 15) & 0x3F;
    // Note that decomposing this character might at first seem to break the
    // "important invariant" described in `arm_write_non_hangul_fallback`, but
    // this is actually not the case. Any composition-relevance-trie value 1
    // code points are guaranteed to decompose into a single code point, so the
    // byte difference might not be the same, but the code point difference is.
    // This also means the decomposition can never exceed a few bytes, well
    // within the 8-byte store below.
    vst1_u8(*out, vld1_u8(decomp_offset));
    *out += length;

    uint8_t ccc = (decomp_value >> 21) & 0xFF;
    if (simdutf_unlikely(ccc != 0 && *last_ccc > ccc)) {
      ccc = scalar::utf8_to_decomposed::sort_combining(
          reinterpret_cast<char *>(*out), out_length + (*out - start));
    }
    input += size;
    *last_ccc = ccc;
  }
}

template <ComposedForm form>
simdutf_really_inline size_t arm_normalize_code_points_utf8(
    uint8x16_t in, uint16x4_t code_points, const uint8_t *input,
    const uint8_t *input_base, size_t input_length, size_t n_bytes,
    uint8_t **out, size_t out_length, uint8_t *last_ccc) {
  uint16x4_t values = arm_comp_trie_lookup<form>(code_points);
  uint16_t max = vmaxv_u16(values);
  // No composition-relevant characters.
  if (max == 0) {
    vst1q_u8(*out, in);
    *out += n_bytes;
    *last_ccc = 0;
    return n_bytes;
  }
  // If the max value is 1, then we have only characters affected by
  // NF(K)D, not anything actually to compose (the first step of NF(K)C is
  // to run NF(K)D, and this guarantees that is the only thing we must do).
  // This allows us to cut out a large portion of work, especially for
  // compatibility composition.
  if (max == 1) {
    // Special writing function that essentially runs NF(K)D on `code_points`.
    // This is the only place we use last_ccc information.
    arm_write_no_comp_utf8<form>(vcombine_u16(values, vdup_n_u16(0)),
                                 vcombine_u16(code_points, vdup_n_u16(0)), 4,
                                 out, out_length, input, last_ccc);
    return n_bytes;
  }
  *last_ccc = 0;
  return scalar::utf8_to_composed::normalize_with_context<form>(
      reinterpret_cast<const char *>(input),
      reinterpret_cast<const char *>(input_base), input_length,
      reinterpret_cast<char **>(out), n_bytes);
}

template <ComposedForm form>
simdutf_really_inline size_t arm_normalize_code_points_utf8_wide(
    uint8x16_t in, uint16x8_t code_points, const uint8_t *input,
    const uint8_t *input_base, size_t input_length, size_t n_bytes,
    uint8_t **out, size_t out_length, uint8_t *last_ccc) {
  uint16x8_t values = arm_comp_trie_lookup_wide<form>(code_points);
  uint16_t max = vmaxvq_u16(values);
  if (max == 0) {
    vst1q_u8(*out, in);
    *out += n_bytes;
    *last_ccc = 0;
    return n_bytes;
  }
  if (max == 1) {
    arm_write_no_comp_utf8<form>(values, code_points, 6, out, out_length, input,
                                 last_ccc);
    return n_bytes;
  }
  *last_ccc = 0;
  return scalar::utf8_to_composed::normalize_with_context<form>(
      reinterpret_cast<const char *>(input),
      reinterpret_cast<const char *>(input_base), input_length,
      reinterpret_cast<char **>(out), n_bytes);
}
} // namespace internal

template <ComposedForm form>
size_t normalize_masked_utf8_to_composed(const uint8_t *input,
                                         const uint8_t *input_base,
                                         size_t input_length, uint64_t mask,
                                         uint8_t **out, size_t out_length,
                                         uint8_t *last_ccc) {
  // Eagerly skip ASCII, similar to NF(K)D.
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

  if (sml_mask == 0x924) {
    uint16x4_t code_points = arm_parse_3_byte_utf8(in);
    return internal::arm_normalize_code_points_utf8<form>(
        in, code_points, input, input_base, input_length, 12, out, out_length,
        last_ccc);
  }
  if (sml_mask == 0xAAA) {
    uint16x8_t code_points = arm_parse_2_byte_utf8(in);
    return internal::arm_normalize_code_points_utf8_wide<form>(
        in, code_points, input, input_base, input_length, 12, out, out_length,
        last_ccc);
  }

  uint8_t idx = simdutf::tables::utf8_to_utf16::utf8bigindex[sml_mask][0];
  uint8_t n_bytes = simdutf::tables::utf8_to_utf16::utf8bigindex[sml_mask][1];
  if (idx < 64) {
    uint16x8_t code_points = arm_parse_6_12_utf8(in, idx);
    return internal::arm_normalize_code_points_utf8_wide<form>(
        in, code_points, input, input_base, input_length, n_bytes, out,
        out_length, last_ccc);
  }
  if (idx < 145) {
    uint16x4_t code_points = arm_parse_4_123_utf8(in, idx);
    return internal::arm_normalize_code_points_utf8<form>(
        in, code_points, input, input_base, input_length, n_bytes, out,
        out_length, last_ccc);
  }

  // idx < 209: three 1..4-byte code points. Always fall back for this
  // case.
  *last_ccc = 0;
  return scalar::utf8_to_composed::normalize_with_context<form>(
      reinterpret_cast<const char *>(input),
      reinterpret_cast<const char *>(input_base), input_length,
      reinterpret_cast<char **>(out), n_bytes);
}

namespace internal {
template <ComposedForm form>
simdutf_really_inline uint16x4_t
arm_comp_check_trie_lookup(uint16x4_t code_points) {
  uint16_t buf[4];
  buf[0] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vget_lane_u16(code_points, 0));
  buf[1] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vget_lane_u16(code_points, 1));
  buf[2] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vget_lane_u16(code_points, 2));
  buf[3] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vget_lane_u16(code_points, 3));
  return vld1_u16(buf);
}

template <ComposedForm form>
simdutf_really_inline uint16x8_t
arm_comp_check_trie_lookup_wide(uint16x8_t code_points) {
  uint16_t buf[8];
  buf[0] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 0));
  buf[1] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 1));
  buf[2] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 2));
  buf[3] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 3));
  buf[4] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 4));
  buf[5] = scalar::utf8_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 5));
  buf[6] = 0;
  buf[7] = 0;
  return vld1q_u16(buf);
}

template <ComposedForm form>
simdutf_really_inline void
arm_comp_check_code_points_utf8(uint16x4_t code_points, size_t *out_length,
                                bool *is_qc, uint8_t *last_ccc) {
  uint16x4_t values = arm_comp_check_trie_lookup<form>(code_points);
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

template <ComposedForm form>
simdutf_really_inline void
arm_comp_check_code_points_utf8_wide(uint16x8_t code_points, size_t *out_length,
                                     bool *is_qc, uint8_t *last_ccc) {
  uint16x8_t values = arm_comp_check_trie_lookup_wide<form>(code_points);
  *out_length += vaddvq_u16(vandq_u16(values, vdupq_n_u16(0x3F)));
  uint16x8_t ccc_values = vandq_u16(vshrq_n_u16(values, 6), vdupq_n_u16(0xFF));
  if (*is_qc) {
    *is_qc &= !vmaxvq_u16(vshrq_n_u16(values, 15)) &&
              arm_is_ccc_sorted_full(ccc_values, *last_ccc);
  }
  *last_ccc = uint8_t(vgetq_lane_u16(ccc_values, 5));
}
} // namespace internal

template <ComposedForm form>
size_t normalize_masked_utf8_to_composed_check(const uint8_t *input,
                                               uint64_t mask,
                                               size_t *out_length, bool *is_qc,
                                               uint8_t *last_ccc) {
  // Unlike the writer path, there is no store to amortize here, so it is
  // worth skipping even a single leading ASCII byte.
  int t1 = trailing_zeroes(~mask);
  if (t1 > 0) {
    size_t min = t1 > 52 ? 52 : t1;
    *out_length += min;
    *last_ccc = 0;
    return min;
  }
  uint8x16_t in = vld1q_u8(input);
  uint16_t sml_mask = mask & 0xFFF;

  if (sml_mask == 0x924) {
    uint16x4_t code_points = arm_parse_3_byte_utf8(in);
    internal::arm_comp_check_code_points_utf8<form>(code_points, out_length,
                                                    is_qc, last_ccc);
    return 12;
  }
  if (sml_mask == 0xAAA) {
    uint16x8_t code_points = arm_parse_2_byte_utf8(in);
    internal::arm_comp_check_code_points_utf8_wide<form>(
        code_points, out_length, is_qc, last_ccc);
    return 12;
  }

  uint8_t idx = simdutf::tables::utf8_to_utf16::utf8bigindex[sml_mask][0];
  uint8_t n_bytes = simdutf::tables::utf8_to_utf16::utf8bigindex[sml_mask][1];
  if (idx < 64) {
    uint16x8_t code_points = arm_parse_6_12_utf8(in, idx);
    internal::arm_comp_check_code_points_utf8_wide<form>(
        code_points, out_length, is_qc, last_ccc);
    return n_bytes;
  }
  if (idx < 145) {
    uint16x4_t code_points = arm_parse_4_123_utf8(in, idx);
    internal::arm_comp_check_code_points_utf8<form>(code_points, out_length,
                                                    is_qc, last_ccc);
    return n_bytes;
  }

  // idx < 209: three 1..4-byte code points. Always fall back for this
  // case.
  *is_qc &= scalar::utf8_to_composed::check_with_context<form>(
      reinterpret_cast<const char *>(input), n_bytes, out_length, last_ccc);
  return n_bytes;
}
