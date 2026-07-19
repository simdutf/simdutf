namespace internal {
template <ComposedForm form>
simdutf_really_inline uint16x8_t
arm_comp_trie_lookup_utf16(uint16x8_t code_points) {
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
  buf[6] = scalar::normalization::lookup_comp_trie<form>(
      vgetq_lane_u16(code_points, 6));
  buf[7] = scalar::normalization::lookup_comp_trie<form>(
      vgetq_lane_u16(code_points, 7));
  return vld1q_u16(buf);
}

// Write eight BMP code points that have value 0 or 1 from the NF(K)C trie.
// This means that they do not compose with anything, so this function
// essentially performs NF(K)D on the given code points.
template <endianness big_endian, ComposedForm form>
void arm_write_no_comp_utf16(uint16x8_t values, uint16x8_t code_points,
                             char16_t **out, size_t out_length,
                             const char16_t *input, uint8_t *last_ccc) {
  constexpr auto dform = to_decomposed_form(form);
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  uint16_t values_buf[8];
  uint16_t code_points_buf[8];
  vst1q_u16(values_buf, values);
  vst1q_u16(code_points_buf, code_points);
#endif

  for (size_t i = 0; i < 8; i++) {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint16_t value = values_buf[i];
#else
    uint16_t value = values[i];
#endif
    if (value == 0) {
      *(*out)++ = input[0];
      input++;
      *last_ccc = 0;
      continue;
    }
    // Decompose the code point like we would in NF(K)D. Note that this can
    // never trigger a resort or exceed a few code units, since composition-
    // relevance-trie value 1 code points are guaranteed to decompose into a
    // single code point.
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    uint16_t code_point = code_points_buf[i];
#else
    uint16_t code_point = code_points[i];
#endif
    uint32_t decomp_value =
        scalar::utf16_to_decomposed::lookup_full_trie<dform>(code_point);
    uint16_t offset = decomp_value & 0x3FFF;
    uint8_t length = uint8_t(((decomp_value >> 14) & 0x3F) + 1);
    uint16x4_t words =
        vld1_u16(&tables::utf16_to_decomposed::decompositions[offset]);
    if (big_endian == endianness::BIG) {
      // Table words are stored in little endian, so swap them if necessary
      words = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(words)));
    }
    vst1_u16(reinterpret_cast<uint16_t *>(*out), words);
    *out += length;
    out_length += length;

    uint8_t ccc = uint8_t(decomp_value >> 24);
    if (simdutf_unlikely(ccc != 0 && *last_ccc > ccc)) {
      ccc = scalar::utf16_to_decomposed::sort_combining<big_endian>(*out,
                                                                    out_length);
    }
    input += 1;
    *last_ccc = ccc;
  }
}
} // namespace internal

template <endianness big_endian, ComposedForm form>
size_t arm_normalize_utf16_to_composed(const char16_t *input, size_t length,
                                       char16_t *output) {
  char16_t **out_ptr = &output;
  char16_t *start = output;

  const size_t SAFETY_MARGIN = 8;
  uint8_t last_ccc = 0;
  size_t p = 0;
  while (p + SAFETY_MARGIN < length) {
    uint16x8_t raw_in =
        vld1q_u16(reinterpret_cast<const uint16_t *>(input + p));
    uint16x8_t in;
    if constexpr (!match_system(big_endian)) {
      in = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(raw_in)));
    } else {
      in = raw_in;
    }
    // ASCII fast path.
    if (vmaxvq_u16(in) <= 0x7F) {
      // Get the next eight code units and check if they are also ASCII.
      uint16x8_t raw_nextin =
          vld1q_u16(reinterpret_cast<const uint16_t *>(input + p) + 8);
      uint16x8_t nextin;
      if constexpr (!match_system(big_endian)) {
        nextin =
            vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(raw_nextin)));
      } else {
        nextin = raw_nextin;
      }
      if (vmaxvq_u16(nextin) > 0x7F) {
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr), raw_in);
        *out_ptr += 8;
        p += 8;
        last_ccc = 0;
        raw_in = raw_nextin;
        in = nextin;
      } else {
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr), raw_in);
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr) + 8, raw_nextin);
        *out_ptr += 16;
        p += 16;
        last_ccc = 0;
        continue;
      }
    }
    uint16x8_t surrogates_mask = internal::arm_make_utf16_surrogates_mask(in);
    // Check if we have no surrogate pairs.
    if (vmaxvq_u16(surrogates_mask) == 0) {
      uint16x8_t values = internal::arm_comp_trie_lookup_utf16<form>(in);
      uint16_t max = vmaxvq_u16(values);
      // No composition-relevant characters.
      if (max == 0) {
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr), raw_in);
        *out_ptr += 8;
        p += 8;
        last_ccc = 0;
        continue;
      }
      // If the max value is 1, then we have only characters affected by
      // NF(K)D, not anything actually to compose (the first step of NF(K)C is
      // to run NF(K)D, and this guarantees that is the only thing we must
      // do). This allows us to cut out a large portion of work, especially
      // for compatibility composition.
      if (max == 1) {
        internal::arm_write_no_comp_utf16<big_endian, form>(
            values, in, out_ptr, *out_ptr - start, input + p, &last_ccc);
        p += 8;
        continue;
      }
      p += scalar::utf16_to_composed::normalize_with_context<big_endian, form>(
          input + p, input, length, out_ptr, 8);
      last_ccc = 0;
    } else {
      // With surrogate pairs, we fall back to the scalar implementation.
      size_t normalize_range = 8;
      if (vgetq_lane_u16(surrogates_mask, 7) == 0xFFFF) {
        // Include the low surrogate in the normalization range.
        normalize_range += 1;
      }
      p += scalar::utf16_to_composed::normalize_with_context<big_endian, form>(
          input + p, input, length, out_ptr, normalize_range);
      last_ccc = 0;
    }
  }

  if (p < length) {
    (void)scalar::utf16_to_composed::normalize_with_context<big_endian, form>(
        input + p, input, length, out_ptr, length - p);
  }

  return *out_ptr - start;
}

namespace internal {
template <ComposedForm form>
simdutf_really_inline uint16x8_t
arm_comp_check_trie_lookup_utf16(uint16x8_t code_points) {
  uint16_t buf[8];
  buf[0] = scalar::utf16_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 0));
  buf[1] = scalar::utf16_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 1));
  buf[2] = scalar::utf16_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 2));
  buf[3] = scalar::utf16_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 3));
  buf[4] = scalar::utf16_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 4));
  buf[5] = scalar::utf16_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 5));
  buf[6] = scalar::utf16_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 6));
  buf[7] = scalar::utf16_to_composed::lookup_check_trie<form>(
      vgetq_lane_u16(code_points, 7));
  return vld1q_u16(buf);
}

template <ComposedForm form>
simdutf_really_inline void
arm_comp_check_code_points_utf16(uint16x8_t code_points, size_t *out_length,
                                 bool *is_qc, uint8_t *last_ccc) {
  uint16x8_t values = arm_comp_check_trie_lookup_utf16<form>(code_points);
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

template <endianness big_endian, ComposedForm form>
bool arm_normalize_utf16_to_composed_check(const char16_t *input, size_t length,
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
      // Get the next eight code units and check if they are also ASCII.
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
      internal::arm_comp_check_code_points_utf16<form>(in, out_length, &is_qc,
                                                       &last_ccc);
    } else {
      size_t normalize_range = 8;
      if (vgetq_lane_u16(surrogates_mask, 7) == 0xFFFF) {
        normalize_range += 1;
      }
      is_qc &= scalar::utf16_to_composed::check_with_context<big_endian, form>(
          input + p, normalize_range, out_length, &last_ccc);
      p += normalize_range - 8;
    }
    p += 8;
  }
  if (p < length) {
    is_qc &= scalar::utf16_to_composed::check_with_context<big_endian, form>(
        input + p, length - p, out_length, &last_ccc);
  }
  return is_qc;
}
