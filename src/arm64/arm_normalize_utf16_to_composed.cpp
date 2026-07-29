namespace internal {
template <ComposedForm form>
simdutf_really_inline uint16x8_t
arm_comp_trie_lookup_utf16(uint16x8_t code_points) {
  uint16_t buf[8];
  buf[0] = scalar::normalization::lookup_comp_trie_bmp<form>(
      vgetq_lane_u16(code_points, 0));
  buf[1] = scalar::normalization::lookup_comp_trie_bmp<form>(
      vgetq_lane_u16(code_points, 1));
  buf[2] = scalar::normalization::lookup_comp_trie_bmp<form>(
      vgetq_lane_u16(code_points, 2));
  buf[3] = scalar::normalization::lookup_comp_trie_bmp<form>(
      vgetq_lane_u16(code_points, 3));
  buf[4] = scalar::normalization::lookup_comp_trie_bmp<form>(
      vgetq_lane_u16(code_points, 4));
  buf[5] = scalar::normalization::lookup_comp_trie_bmp<form>(
      vgetq_lane_u16(code_points, 5));
  buf[6] = scalar::normalization::lookup_comp_trie_bmp<form>(
      vgetq_lane_u16(code_points, 6));
  buf[7] = scalar::normalization::lookup_comp_trie_bmp<form>(
      vgetq_lane_u16(code_points, 7));
  return vld1q_u16(buf);
}

// Write eight BMP code points that have value 0 or 1 from the NF(K)C trie.
// This means that they do not compose with anything, so this function
// essentially performs NF(K)D on the given code points.
template <endianness big_endian, ComposedForm form>
void arm_write_no_comp_utf16(uint16x8_t indicators, uint16x8_t code_points,
                             char16_t **out, const char16_t *input) {
  constexpr auto dform = to_decomposed_form(form);
  if constexpr (!match_system(big_endian)) {
    vst1q_u16(
        reinterpret_cast<uint16_t *>(*out),
        vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(code_points))));
  } else {
    vst1q_u16(reinterpret_cast<uint16_t *>(*out), code_points);
  }
  size_t displacement = 0;
  uint16x8_t indicator1 = vceqq_u16(indicators, vdupq_n_u16(1));
  uint64_t bitmask8 =
      arm_bitmask4(vreinterpretq_u8_u16(indicator1)) & 0x8080808080808080ULL;
  for (; bitmask8 > 0; bitmask8 &= bitmask8 - 1) {
    uint32_t i = trailing_zeroes(bitmask8) >> 3;
    uint16_t code_point = code_points[i];
    uint32_t value =
        scalar::utf16_to_decomposed::lookup_full_trie_bmp<dform>(code_point);
    const uint16_t *words =
        &simdutf::tables::utf16_to_decomposed::decompositions[value & 0x3FFF];
    size_t delta = (value >> 14) & 0x3F;
    if (simdutf_unlikely(delta == 1)) {
      scalar::normalization::shift_right(*out + i + displacement + 1,
                                         (8 + displacement) - (i + 1), 1);
      if constexpr (big_endian == endianness::BIG) {
        (*out)[i + displacement] = scalar::u16_swap_bytes(words[0]);
        (*out)[i + displacement + 1] = scalar::u16_swap_bytes(words[1]);
      } else {
        (*out)[i + displacement] = words[0];
        (*out)[i + displacement + 1] = words[1];
      }
      displacement++;
    } else {
      if constexpr (big_endian == endianness::BIG) {
        (*out)[i + displacement] = scalar::u16_swap_bytes(words[0]);
      } else {
        (*out)[i + displacement] = words[0];
      }
    }
  }
  *out += 8 + displacement;
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
  // The most recent input offset proven to be a composition boundary, in the
  // sense of ICU's `prevBoundary`. Nothing before it can interact with anything
  // at or after it, so the scalar fallback never has to look further back than
  // this. Offset zero is trivially a boundary. Every position between this and
  // `p` maps one input code point to exactly one output code point, which is
  // what lets `normalize_with_context` rewind the output pointer.
  size_t prev_boundary = 0;
  while (p + SAFETY_MARGIN < length) {
    uint16x8_t raw_in =
        vld1q_u16(reinterpret_cast<const uint16_t *>(input + p));
    uint16x8_t in;
    if constexpr (!match_system(big_endian)) {
      in = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(raw_in)));
    } else {
      in = raw_in;
    }
    // Fast path for code points that are irrelevant to NF(K)C.
    if (vmaxvq_u16(in) < scalar::normalization::min_relevant_cp<form>) {
      if (p + 16 > length) {
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr), raw_in);
        *out_ptr += 8;
        prev_boundary = p + 7;
        p += 8;
        last_ccc = 0;
        continue;
      }
      // Get the next eight code units and check if they are also irrelevant.
      uint16x8_t raw_nextin =
          vld1q_u16(reinterpret_cast<const uint16_t *>(input + p) + 8);
      uint16x8_t nextin;
      if constexpr (!match_system(big_endian)) {
        nextin =
            vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(raw_nextin)));
      } else {
        nextin = raw_nextin;
      }
      if (vmaxvq_u16(nextin) >= scalar::normalization::min_relevant_cp<form>) {
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr), raw_in);
        *out_ptr += 8;
        prev_boundary = p + 7;
        p += 8;
        last_ccc = 0;
        raw_in = raw_nextin;
        in = nextin;
      } else {
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr), raw_in);
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr) + 8, raw_nextin);
        *out_ptr += 16;
        prev_boundary = p + 15;
        p += 16;
        last_ccc = 0;
        continue;
      }
    }
    uint16x8_t surrogates_mask = internal::arm_make_utf16_surrogates_mask(in);
    // Check if we have no surrogate pairs.
    if (vmaxvq_u16(surrogates_mask) == 0) {
      // We use this to compose normalize a larger window using scalar if we
      // detect Jamo. Note that this means inputs that sparsely put Jamo
      // randomly throughout the input can be slower. This doesn't seem very
      // common in real-world inputs, though.
      uint16x8_t jamo_vt =
          vcltq_u16(vsubq_u16(in, vdupq_n_u16(scalar::normalization::v_base)),
                    vdupq_n_u16(scalar::normalization::t_base +
                                scalar::normalization::t_count -
                                scalar::normalization::v_base));
      if (vmaxvq_u16(jamo_vt) != 0) {
        p +=
            scalar::utf16_to_composed::normalize_with_context<big_endian, form>(
                input + p, input, length, out_ptr, 32, prev_boundary);
        prev_boundary = p;
        last_ccc = 0;
        continue;
      }
      uint16x8_t values = internal::arm_comp_trie_lookup_utf16<form>(in);
      uint16x8_t indicators = vandq_u16(values, vdupq_n_u16(0b11));
      uint16_t max = vmaxvq_u16(indicators);
      // No composition-relevant characters.
      if (max == 0) {
        vst1q_u16(reinterpret_cast<uint16_t *>(*out_ptr), raw_in);
        *out_ptr += 8;
        prev_boundary = p + 7;
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
        uint16x8_t forward_starter =
            vcgtq_u16(vandq_u16(values, vdupq_n_u16(0x8000)), vdupq_n_u16(0));
        uint16x8_t raw_ccc_values =
            vandq_u16(vshrq_n_u16(values, 2), vdupq_n_u16(0xFF));
        uint16x8_t ccc_values =
            vbslq_u16(forward_starter, vdupq_n_u16(0), raw_ccc_values);
        if (simdutf_unlikely(
                !internal::arm_is_ccc_sorted_full(ccc_values, last_ccc))) {
          p += scalar::utf16_to_composed::normalize_with_context<big_endian,
                                                                 form>(
              input + p, input, length, out_ptr, 8, prev_boundary);
          prev_boundary = p;
          last_ccc = 0;
          continue;
        }
        internal::arm_write_no_comp_utf16<big_endian, form>(indicators, in,
                                                            out_ptr, input + p);
        p += 8;
        last_ccc = uint8_t(vgetq_lane_u16(ccc_values, 7));
        continue;
      }
      p += scalar::utf16_to_composed::normalize_with_context<big_endian, form>(
          input + p, input, length, out_ptr, 8, prev_boundary);
      // The fallback consumes up to the next stable position, so wherever it
      // stopped is a boundary.
      prev_boundary = p;
      last_ccc = 0;
    } else {
      // With surrogate pairs, we fall back to the scalar implementation.
      size_t normalize_range = 8;
      if (vgetq_lane_u16(surrogates_mask, 7) == 0xFFFF) {
        // Include the low surrogate in the normalization range.
        normalize_range += 1;
      }
      p += scalar::utf16_to_composed::normalize_with_context<big_endian, form>(
          input + p, input, length, out_ptr, normalize_range, prev_boundary);
      prev_boundary = p;
      last_ccc = 0;
    }
  }

  if (p < length) {
    (void)scalar::utf16_to_composed::normalize_with_context<big_endian, form>(
        input + p, input, length, out_ptr, length - p, prev_boundary);
  }

  return *out_ptr - start;
}

namespace internal {
template <ComposedForm form>
simdutf_really_inline uint16x8_t
arm_comp_check_trie_lookup_utf16(uint16x8_t code_points) {
  uint16_t buf[8];
  buf[0] = scalar::utf16_to_composed::lookup_check_trie_bmp<form>(
      vgetq_lane_u16(code_points, 0));
  buf[1] = scalar::utf16_to_composed::lookup_check_trie_bmp<form>(
      vgetq_lane_u16(code_points, 1));
  buf[2] = scalar::utf16_to_composed::lookup_check_trie_bmp<form>(
      vgetq_lane_u16(code_points, 2));
  buf[3] = scalar::utf16_to_composed::lookup_check_trie_bmp<form>(
      vgetq_lane_u16(code_points, 3));
  buf[4] = scalar::utf16_to_composed::lookup_check_trie_bmp<form>(
      vgetq_lane_u16(code_points, 4));
  buf[5] = scalar::utf16_to_composed::lookup_check_trie_bmp<form>(
      vgetq_lane_u16(code_points, 5));
  buf[6] = scalar::utf16_to_composed::lookup_check_trie_bmp<form>(
      vgetq_lane_u16(code_points, 6));
  buf[7] = scalar::utf16_to_composed::lookup_check_trie_bmp<form>(
      vgetq_lane_u16(code_points, 7));
  return vld1q_u16(buf);
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
    // Code points below `min_relevant_cp` are NF(K)C_QC=Yes with ccc zero, so
    // the block passes the quick check and contributes its own length. See the
    // note on `min_relevant_cp` for why this is a valid output-length bound.
    if (vmaxvq_u16(in) < scalar::normalization::min_relevant_cp<form>) {
      // Only load a second vector when sixteen code units are available.
      if (p + 16 > length) {
        *out_length += 8;
        p += 8;
        last_ccc = 0;
        continue;
      }
      // Get the next eight code units and check if they are also irrelevant.
      uint16x8_t nextin =
          vld1q_u16(reinterpret_cast<const uint16_t *>(input + p) + 8);
      if constexpr (!match_system(big_endian)) {
        nextin = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(nextin)));
      }
      if (vmaxvq_u16(nextin) >= scalar::normalization::min_relevant_cp<form>) {
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
      uint16x8_t values = internal::arm_comp_check_trie_lookup_utf16<form>(in);
      *out_length += vaddvq_u16(vandq_u16(values, vdupq_n_u16(0x3F)));
      uint16x8_t indicators = vandq_u16(values, vdupq_n_u16(0x3000));
      uint16_t max = vmaxvq_u16(indicators);
      if (max == 0) {
        last_ccc = 0;
        p += 8;
        continue;
      }
      uint16x8_t ccc_values =
          vandq_u16(vshrq_n_u16(values, 6), vdupq_n_u16(0x3F));
      if (is_qc) {
        // Checking combining classes is expensive, so we only do it if we
        // haven't already failed the quick check.
        is_qc &= max < 0x2000 &&
                 internal::arm_is_ccc_sorted_full(ccc_values, last_ccc);
      }
      last_ccc = uint8_t(vgetq_lane_u16(ccc_values, 7));
      p += 8;
    } else {
      size_t total = 0;
      // Scalar quick check in the supplementary plane
      while (total < 8) {
        uint8_t sz;
        uint32_t code_point =
            scalar::utf16::parse_code_point<big_endian>(input + p + total, &sz);
        uint16_t value =
            scalar::utf16_to_composed::lookup_check_trie<form>(code_point);
        *out_length += value & 0x3F;
        if (simdutf_likely((value & 0x1000) == 0)) {
          last_ccc = 0;
          total += sz;
          continue;
        }
        uint8_t ccc = uint8_t((value >> 6) & 0x3F);
        if (last_ccc > ccc && ccc != 0) {
          is_qc = false;
        }
        is_qc &= !(value & 0x2000);
        total += sz;
        last_ccc = ccc;
      }
      p += total;
    }
  }
  if (p < length) {
    is_qc &= scalar::utf16_to_composed::check_with_context<big_endian, form>(
        input + p, length - p, out_length, &last_ccc);
  }
  return is_qc;
}
