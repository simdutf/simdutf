#ifndef SIMDUTF_UTF8_TO_COMPOSED_H
#define SIMDUTF_UTF8_TO_COMPOSED_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_composed {

// Compose `input` into `out`, returning the number of bytes written.
//
// When `stop_early` is set the walk returns as soon as it reaches a stable
// position at least after `min_length`.
template <ComposedForm form, bool stop_early>
size_t normalize_impl(const char *input, size_t length, char *out,
                      size_t min_length, size_t *consumed) {
  using traits = normalization::utf8_normalization_traits;
  constexpr auto dform = to_decomposed_form(form);
  char *start{out};
  size_t p = 0;
  uint8_t last_ccc = 0;
  // Information related to the most recent starter we've encountered
  size_t starter_pos = 0;
  char *starter_out = out;
  // These decide if we have to run NF(K)D before composing the region we're
  // looking that. It is much faster to not have to run NF(K)D.
  bool span_has_decomposition = false;
  bool span_ordered = true;

  while (p < length) {
    const uint8_t leading = uint8_t(input[p]);
    if (simdutf_likely(leading < 0b10000000)) {
      // Try to stop early if applicable
      if (stop_early && p >= min_length) {
        break;
      }
      starter_pos = p;
      starter_out = out;
      span_has_decomposition = false;
      span_ordered = true;
      *out++ = char(leading);
      p++;
      last_ccc = 0;
      continue;
    }

    uint8_t size;
    uint32_t code_point = utf8::parse_code_point(input + p, &size);
    uint16_t value;
    if (simdutf_likely(size < 4)) {
      value = normalization::lookup_comp_trie_bmp<form>(uint16_t(code_point));
    } else {
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
      char *code_point_out = out;
      uint8_t ccc;
      if ((value & 0x1000) > 0) {
        // Decompose
        uint32_t decomp_value =
            utf8_to_decomposed::lookup_full_trie<dform>(code_point);
        uint8_t first_ccc;
        out += utf8_to_decomposed::write_decomposition(decomp_value, out,
                                                       &first_ccc, &ccc);
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

    // Hangul Jamo fast path. We first do a simple, naive check (the code point
    // is three bytes wide and the starter is the three bytes right before it;
    // every jamo and every precomposed syllable is three bytes in UTF-8). Then,
    // we can try to compose Hangul Jamo entirely in this path, without calling
    // `recompose`.
    if (size == 3 && size_t(out - starter_out) == 3) {
      const uint32_t prev = (uint32_t(uint8_t(out[-3]) & 0x0F) << 12) |
                            (uint32_t(uint8_t(out[-2]) & 0x3F) << 6) |
                            uint32_t(uint8_t(out[-1]) & 0x3F);
      const uint32_t v_index = code_point - normalization::v_base;
      // The first valid T jamo is t_base + 1. See the note in `recompose`.
      const uint32_t t_index = code_point - normalization::t_base;
      if (v_index < normalization::v_count) {
        const uint32_t l_index = prev - normalization::l_base;
        if (l_index < normalization::l_count) {
          uint32_t syllable = normalization::s_base +
                              (l_index * normalization::v_count + v_index) *
                                  normalization::t_count;
          size_t next = p + 3;
          // If this is true, we can sucessfully take the fast path and compose
          // the LV(T) syllable in this path. This can fail if we have a
          // backwards-composing character right after the Jamo sequence, which
          // means we should take the slow path (`recompose`).
          bool composes = true;
          if (next < length) {
            uint8_t following_size;
            const uint32_t following =
                utf8::parse_code_point(input + next, &following_size);
            const uint32_t next_t = following - normalization::t_base;
            // Again, > 0 here is because the first valid T jamo is t_base + 1
            if (next_t > 0 && next_t < normalization::t_count) {
              // A Jamo T follows, so add it to the composite syllable
              syllable += next_t;
              next += following_size;
            } else {
              // No Jamo T follows.
              composes = normalization::is_stable<form>(following);
            }
          }
          if (composes) {
            // Syllables live in U+AC00..U+D7A3, so this is a three-byte write
            // over the three bytes the Jamo L occupies.
            utf8::write_3_byte_code_point(uint16_t(syllable), out - 3);
            p = next;
            last_ccc = 0;
            continue;
          }
        }
      } else if (t_index > 0 && t_index < normalization::t_count) {
        // Check if we have a T syllable after a precomposed LV syllable
        const uint32_t s_index = prev - normalization::s_base;
        if (s_index < normalization::s_count &&
            s_index % normalization::t_count == 0) {
          utf8::write_3_byte_code_point(uint16_t(prev + t_index), out - 3);
          p += 3;
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
      // ASCII is always stable, so stop without touching the trie.
      if (uint8_t(input[next_stable]) < 0b10000000) {
        break;
      }
      uint8_t scan_size;
      uint32_t c = utf8::parse_code_point(input + next_stable, &scan_size);
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
      decomposed_length = utf8_to_decomposed::normalize<dform>(
          input + starter_pos, region_length, starter_out);
    } else {
      std::memcpy(starter_out, input + starter_pos, region_length);
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

template <ComposedForm form, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
size_t normalize(InputPtr input, size_t length, OutputPtr out) {
  return normalize_impl<form, /*stop_early=*/false>(input, length, out, 0,
                                                    nullptr);
}

// Get the byte position of the nth code point going backwards from buf.
simdutf_really_inline size_t get_code_point_pos_reverse(const char *buf,
                                                        size_t n) {
  if (n == 0) {
    return 0;
  }
  size_t count = n;
  size_t p = 0;
  while (true) {
    while ((*((buf - p) - 1) & 0b11000000) == 0b10000000) {
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
// nothing is known. Returns the number of input bytes consumed, which may
// exceed `length`.
template <ComposedForm form, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
size_t normalize_with_context(InputPtr input, InputPtr input_base,
                              size_t input_length, OutputPtr *out,
                              size_t length, size_t prev_boundary = 0) {
  using traits = normalization::utf8_normalization_traits;
  size_t offset = input - input_base;

  size_t prev_starter;
  size_t code_point_dist;
  // Get the region that we will NF(K)C normalize.
  uint8_t first_size;
  uint32_t first = utf8::parse_code_point(input, &first_size);
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
      const char *code_point_start =
          normalization::find_code_point_start_reverse<traits>(input_base + q -
                                                               1);
      count++;
      q = size_t(code_point_start - input_base);
      uint8_t size;
      uint32_t c = utf8::parse_code_point(code_point_start, &size);
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
  // rewinding through code points that had indicator == 1, which allows a code
  // point to be replaced by a single code point of a different UTF-8 width
  // (changing the byte difference between input and output).
  size_t prev_out_offset = get_code_point_pos_reverse(*out, code_point_dist);
  char *prev_out = *out - prev_out_offset;
  size_t consumed = 0;
  size_t nwritten = normalize_impl<form, /*stop_early=*/true>(
      input_base + prev_starter, input_length - prev_starter, prev_out,
      (offset + length) - prev_starter, &consumed);
  *out = prev_out + nwritten;
  return (prev_starter + consumed) - offset;
}

// NF(K)C and NF(K)D share one quick-check trie. Both need the same two fields --
// the decomposed length and the combining class -- and consult different flag
// bits, so the table lives in the decomposed namespace and NF(K)C reads its own
// bits out of it. See `create_check_values` in scripts/normalization.py.
template <ComposedForm form>
simdutf_really_inline uint16_t lookup_check_trie_bmp(uint16_t code_point) {
  return utf8_to_decomposed::lookup_check_trie_bmp<to_decomposed_form(form)>(
      code_point);
}

template <ComposedForm form>
simdutf_really_inline uint16_t
lookup_check_trie_supplementary(uint32_t code_point) {
  return utf8_to_decomposed::lookup_check_trie_supplementary<
      to_decomposed_form(form)>(code_point);
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
  *ccc = uint8_t((value >> 6) & 0x3F);
  return !(value & 0x2000);
}

template <ComposedForm form>
static bool check_code_point_supplementary(uint32_t code_point,
                                           size_t *out_length, uint8_t *ccc) {
  uint16_t value = lookup_check_trie_supplementary<form>(code_point);
  *out_length += value & 0x3F;
  *ccc = uint8_t((value >> 6) & 0x3F);
  return !(value & 0x2000);
}

template <ComposedForm form, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr>)
#endif
bool check_with_context(InputPtr input, size_t length, size_t *out_length,
                        uint8_t *last_ccc) {
  bool is_qc = true;
  size_t p = 0;
  while (p < length) {
    uint8_t leading = input[p];
    uint8_t ccc;
    if (leading < 0b10000000) {
      (*out_length)++;
      p++;
      ccc = 0;
    } else if ((leading & 0b11100000) == 0b11000000) {
      uint32_t code_point =
          (leading & 0b00011111) << 6 | (input[p + 1] & 0b00111111);
      is_qc &=
          check_code_point_bmp<form>(uint16_t(code_point), out_length, &ccc);
      p += 2;
    } else if ((leading & 0b11110000) == 0b11100000) {
      uint32_t code_point = (leading & 0b00001111) << 12 |
                            (input[p + 1] & 0b00111111) << 6 |
                            (input[p + 2] & 0b00111111);
      is_qc &=
          check_code_point_bmp<form>(uint16_t(code_point), out_length, &ccc);
      p += 3;
    } else {
      uint32_t code_point =
          (leading & 0b00000111) << 18 | (input[p + 1] & 0b00111111) << 12 |
          (input[p + 2] & 0b00111111) << 6 | (input[p + 3] & 0b00111111);
      is_qc &=
          check_code_point_supplementary<form>(code_point, out_length, &ccc);
      p += 4;
    }
    if (*last_ccc > ccc && ccc != 0) {
      is_qc = false;
    }
    *last_ccc = ccc;
  }
  return is_qc;
}

template <ComposedForm form, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr>)
#endif
bool check(InputPtr input, size_t length, size_t *out_length) {
  *out_length = 0;
  uint8_t last_ccc = 0;
  return check_with_context<form>(input, length, out_length, &last_ccc);
}

} // namespace utf8_to_composed
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_UTF8_TO_COMPOSED_H
