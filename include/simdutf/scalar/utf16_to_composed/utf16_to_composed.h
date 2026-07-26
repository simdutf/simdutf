#ifndef SIMDUTF_UTF16_TO_COMPOSED_H
#define SIMDUTF_UTF16_TO_COMPOSED_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_composed {

// Compose `input` into `out`, returning the number of code units written.
//
// When `stop_early` is set the walk returns as soon as it reaches a stable
// at least after `min_length`.
template <endianness big_endian, ComposedForm form, bool stop_early>
size_t normalize_impl(const char16_t *input, size_t length, char16_t *out,
                      size_t min_length, size_t *consumed) {
  using traits = normalization::utf16_normalization_traits<big_endian>;
  constexpr auto dform = to_decomposed_form(form);
  char16_t *start{out};
  size_t p = 0;
  uint8_t last_ccc = 0;
  // Information related to the most recent starter we've encountered
  size_t starter_pos = 0;
  char16_t *starter_out = out;
  // These decide if we have to run NF(K)D before composing the region we're
  // looking that. It is much faster to not have to run NF(K)D.
  bool span_has_decomposition = false;
  bool span_ordered = true;

  while (p < length) {
    uint32_t code_point;
    uint8_t size;
    uint16_t value;
    uint16_t leading = scalar::utf16::swap_if_needed<big_endian>(input[p]);
    if (simdutf_likely(!utf16::high_surrogate(leading))) {
      code_point = leading;
      size = 1;
      value = normalization::lookup_comp_trie_bmp<form>(leading);
    } else {
      code_point = utf16::parse_code_point<big_endian>(input + p, &size);
      value = normalization::lookup_comp_trie_supplementary<form>(code_point);
    }
    const uint16_t indicator = value & 0b11;

    if (simdutf_likely(indicator == 0)) {
      // Try to stop early if applicable
      if (stop_early && p >= min_length) {
        break;
      }
      starter_pos = p;
      starter_out = out;
      span_has_decomposition = (value & 0x1000) > 0;
      span_ordered = true;
      for (uint8_t i = 0; i < size; i++) {
        *out++ = input[p + i];
      }
      p += size;
      last_ccc = 0;
      continue;
    }

    // An indicator of one means the code point composes with nothing and maps
    // to exactly one code point, so it can be decomposed or copied on the spot
    if (indicator == 1) {
      char16_t *code_point_out = out;
      uint8_t ccc;
      if ((value & 0x1000) > 0) {
        // Decompose
        uint32_t decomp_value =
            utf16_to_decomposed::lookup_full_trie<dform>(code_point);
        uint8_t first_ccc;
        out += utf16_to_decomposed::write_decomposition<big_endian>(
            decomp_value, out, &first_ccc, &ccc);
      } else {
        // Copy
        for (uint8_t i = 0; i < size; i++) {
          *out++ = input[p + i];
        }
        ccc = (value & 0x8000) > 0 ? 0 : uint8_t(value >> 2);
      }

      p += size;
      bool decomposes = (value & 0x1000) > 0;
      // Check for combining character order
      if (ccc != 0 && last_ccc > ccc) {
        span_ordered = false;
        ccc = normalization::sort_combining<traits>(out,
                                                    size_t(out - starter_out));
      }
      // Update cached values
      if (ccc == 0) {
        starter_pos = p - size;
        starter_out = code_point_out;
        span_has_decomposition = decomposes;
        span_ordered = true;
      } else {
        span_has_decomposition |= decomposes;
      }
      last_ccc = ccc;
      continue;
    }

    // Hangul Jamo fast path. We first do a simple, naive check (size == 1 and
    // starter_out == out - 1). Then, we can try to compose Hangul Jamo entirely
    // in this path, without calling `recompose`.
    if (size == 1 && starter_out == out - 1) {
      const uint16_t prev = scalar::utf16::swap_if_needed<big_endian>(out[-1]);
      const uint32_t v_index = code_point - normalization::v_base;
      // The first valid T jamo is t_base + 1. See the note in `recompose`.
      const uint32_t t_index = code_point - normalization::t_base;
      if (v_index < normalization::v_count) {
        const uint32_t l_index = uint32_t(prev) - normalization::l_base;
        if (l_index < normalization::l_count) {
          uint32_t syllable = normalization::s_base +
                              (l_index * normalization::v_count + v_index) *
                                  normalization::t_count;
          size_t next = p + 1;
          // If this is true, we can sucessfully take the fast path and compose
          // the LV(T) syllable in this path. This can fail if we have a
          // backwards-composing character right after the Jamo sequence, which
          // means we should take the slow path (`recompose`).
          bool composes = true;
          if (next < length) {
            const uint16_t following =
                scalar::utf16::swap_if_needed<big_endian>(input[next]);
            const uint32_t next_t = uint32_t(following) - normalization::t_base;
            // Again, > 0 here is because the first valid T jamo is t_base + 1
            if (next_t > 0 && next_t < normalization::t_count) {
              // A Jamo T follows, so add it to the composite syllable
              syllable += next_t;
              next++;
            } else {
              // No Jamo T follows.
              composes = !utf16::high_surrogate(following) &&
                         normalization::is_stable<form>(following);
            }
          }
          if (composes) {
            out[-1] =
                scalar::utf16::swap_if_needed<big_endian>(char16_t(syllable));
            p = next;
            last_ccc = 0;
            continue;
          }
        }
      } else if (t_index > 0 && t_index < normalization::t_count) {
        // Check if we have a T syllable after a precomposed LV syllable
        const uint32_t s_index = uint32_t(prev) - normalization::s_base;
        if (s_index < normalization::s_count &&
            s_index % normalization::t_count == 0) {
          out[-1] = scalar::utf16::swap_if_needed<big_endian>(
              char16_t(uint32_t(prev) + t_index));
          p += 1;
          last_ccc = 0;
          continue;
        }
      }
    }

    // Slow path. Normalize the region from the last starter to the next stable.
    uint8_t ccc = (value & 0x8000) > 0 ? 0 : uint8_t(value >> 2);
    bool needs_decomposition = span_has_decomposition || !span_ordered ||
                               (value & 0x1000) > 0 ||
                               (ccc != 0 && last_ccc > ccc);
    // Reach forward until the next stable codepoint. This will assemble our
    // NF(K)D region
    size_t next_stable = p + size;
    uint8_t prev_ccc = ccc;
    while (next_stable < length) {
      uint8_t scan_size;
      uint32_t c =
          utf16::parse_code_point<big_endian>(input + next_stable, &scan_size);
      uint16_t scan_value = normalization::lookup_comp_trie<form>(c);
      if ((scan_value & 0b11) == 0) {
        break;
      }
      uint8_t scan_ccc =
          (scan_value & 0x8000) > 0 ? 0 : uint8_t(scan_value >> 2);
      needs_decomposition |=
          (scan_value & 0x1000) > 0 || (scan_ccc != 0 && prev_ccc > scan_ccc);
      prev_ccc = scan_ccc;
      next_stable += scan_size;
    }

    size_t region_length = next_stable - starter_pos;
    size_t decomposed_length;
    if (needs_decomposition) {
      decomposed_length = utf16_to_decomposed::normalize<big_endian, dform>(
          input + starter_pos, region_length, starter_out);
    } else {
      std::memcpy(starter_out, input + starter_pos,
                  region_length * sizeof(char16_t));
      decomposed_length = region_length;
    }
    // Call the recomposition function on the NF(K)D region
    out = starter_out + normalization::recompose<traits, form>(
                            starter_out, decomposed_length);
    p = next_stable;
    last_ccc = 0;
  }

  if (consumed != nullptr) {
    *consumed = p;
  }
  return out - start;
}

template <endianness big_endian, ComposedForm form>
size_t normalize(const char16_t *input, size_t length, char16_t *out) {
  return normalize_impl<big_endian, form, /*stop_early=*/false>(
      input, length, out, 0, nullptr);
}

// Get the code unit position of the nth code point going backwards from buf.
template <endianness big_endian>
simdutf_really_inline size_t get_code_point_pos_reverse(const char16_t *buf,
                                                        size_t n) {
  if (n == 0) {
    return 0;
  }
  size_t count = n;
  size_t p = 0;
  while (true) {
    if (utf16::is_low_surrogate<big_endian>(*(buf - p - 1))) {
      p++;
    }
    count--;
    if (count == 0) {
      return p + 1;
    }
    p++;
  }
}

// Normalize the region around `input`, widening it to the nearest stable
// positions on either side. `prev_boundary` is an offset into `input_base` that
// the caller has already proven to be a composition boundary; the backward
// search stops there instead of running to the start of the input. Pass zero if
// nothing is known. Returns the number of input code units consumed, which may
// exceed `length`.
template <endianness big_endian, ComposedForm form>
size_t normalize_with_context(const char16_t *input, const char16_t *input_base,
                              size_t input_length, char16_t **out,
                              size_t length, size_t prev_boundary = 0) {
  using traits = normalization::utf16_normalization_traits<big_endian>;
  size_t offset = input - input_base;

  size_t prev_starter;
  size_t code_point_dist;
  // Unrolled first step of the loop
  uint8_t first_size;
  uint32_t first = utf16::parse_code_point<big_endian>(input, &first_size);
  if (normalization::is_stable<form>(first)) {
    prev_starter = offset;
    code_point_dist = 0;
  } else {
    // Walk backwards to the last stable position, counting code points on the
    // way
    size_t q = offset;
    size_t count = 0;
    prev_starter = prev_boundary;
    while (q > prev_boundary) {
      const char16_t *code_point_start =
          normalization::find_code_point_start_reverse<traits>(input_base + q -
                                                               1);
      count++;
      q = size_t(code_point_start - input_base);
      uint8_t size;
      uint32_t c = utf16::parse_code_point<big_endian>(code_point_start, &size);
      if (normalization::is_stable<form>(c)) {
        prev_starter = q;
        break;
      }
    }
    code_point_dist = count;
  }

  // Rewind the output by the same number of code points, so we know where the
  // left edge of the region lives in the output buffer. Everything between a
  // stable position and here maps one input code point to one output code
  // point, which is what makes this valid. A natural question is also why we
  // are counting code points instead of bytes; the answer is that we may be
  // rewinding through code points that had indicator == 1, which allows code
  // points to go from BMP to supplementary and vice versa (changing the
  // lengths byte difference between input and output).
  size_t prev_out_offset =
      get_code_point_pos_reverse<big_endian>(*out, code_point_dist);
  char16_t *prev_out = *out - prev_out_offset;
  size_t consumed = 0;
  size_t nwritten = normalize_impl<big_endian, form, /*stop_early=*/true>(
      input_base + prev_starter, input_length - prev_starter, prev_out,
      (offset + length) - prev_starter, &consumed);
  *out = prev_out + nwritten;

  return (prev_starter + consumed) - offset;
}

template <ComposedForm form>
simdutf_really_inline uint16_t lookup_check_trie_bmp(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint16_t index;
  uint16_t value;
  if constexpr (form == ComposedForm::NFC) {
    index = simdutf::tables::utf16_to_composed::nfc::check_trie_index[shift];
    value = simdutf::tables::utf16_to_composed::nfc::check_trie_data[index +
                                                                     masked];
  } else {
    index = simdutf::tables::utf16_to_composed::nfkc::check_trie_index[shift];
    value = simdutf::tables::utf16_to_composed::nfkc::check_trie_data[index +
                                                                      masked];
  }
  return value;
}

template <ComposedForm form>
simdutf_really_inline uint16_t
lookup_check_trie_supplementary(uint32_t code_point) {
  uint32_t supplementary = code_point - 0x10000;
  if constexpr (form == ComposedForm::NFC) {
    uint16_t index1 = simdutf::tables::utf16_to_composed::nfc::check_trie_index1
        [supplementary >> 11];
    uint16_t index2 = simdutf::tables::utf16_to_composed::nfc::check_trie_index2
        [index1 + ((supplementary >> 6) & 31)];
    return simdutf::tables::utf16_to_composed::nfc::check_trie_data
        [index2 + (code_point & 63)];
  } else {
    uint16_t index1 = simdutf::tables::utf16_to_composed::nfkc::
        check_trie_index1[supplementary >> 11];
    uint16_t index2 = simdutf::tables::utf16_to_composed::nfkc::
        check_trie_index2[index1 + ((supplementary >> 6) & 31)];
    return simdutf::tables::utf16_to_composed::nfkc::check_trie_data
        [index2 + (code_point & 63)];
  }
}

template <ComposedForm form>
simdutf_really_inline uint16_t lookup_check_trie(uint32_t code_point) {
  if (code_point <= 0xFFFF) {
    return lookup_check_trie_bmp<form>(uint16_t(code_point));
  } else {
    return lookup_check_trie_supplementary<form>(code_point);
  }
}

template <ComposedForm form>
bool check_code_point_bmp(uint16_t code_point, size_t *out_length,
                          uint8_t *ccc) {
  uint16_t value = lookup_check_trie_bmp<form>(code_point);
  *out_length += value & 0x3F;
  *ccc = uint8_t((value >> 6) & 0xFF);
  return !(value >> 15);
}

template <ComposedForm form>
static bool check_code_point_supplementary(uint32_t code_point,
                                           size_t *out_length, uint8_t *ccc) {
  uint16_t value = lookup_check_trie_supplementary<form>(code_point);
  *out_length += value & 0x3F;
  *ccc = uint8_t((value >> 6) & 0xFF);
  return !(value >> 15);
}

template <endianness big_endian, ComposedForm form>
bool check_with_context(const char16_t *input, size_t length,
                        size_t *out_length, uint8_t *last_ccc) {
  bool is_qc = true;
  size_t p = 0;
  while (p < length) {
    uint8_t size;
    uint32_t code_point = utf16::parse_code_point<big_endian>(input + p, &size);
    uint8_t ccc;
    if (size == 1) {
      is_qc &=
          check_code_point_bmp<form>(uint16_t(code_point), out_length, &ccc);
    } else {
      is_qc &=
          check_code_point_supplementary<form>(code_point, out_length, &ccc);
    }
    p += size;
    if (*last_ccc > ccc && ccc != 0) {
      is_qc = false;
    }
    *last_ccc = ccc;
  }
  return is_qc;
}

template <endianness big_endian, ComposedForm form>
bool check(const char16_t *input, size_t length, size_t *out_length) {
  *out_length = 0;
  uint8_t last_ccc = 0;
  return check_with_context<big_endian, form>(input, length, out_length,
                                              &last_ccc);
}

} // namespace utf16_to_composed
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_UTF16_TO_COMPOSED_H
