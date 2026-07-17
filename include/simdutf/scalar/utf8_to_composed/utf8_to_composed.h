#ifndef SIMDUTF_UTF8_TO_COMPOSED_H
#define SIMDUTF_UTF8_TO_COMPOSED_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_composed {

template <ComposedForm form>
simdutf_really_inline uint16_t lookup_comp_trie(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint16_t index;
  uint16_t value;
  if constexpr (form == ComposedForm::NFC) {
    index = simdutf::tables::normalization::nfc::trie_index[shift];
    value = simdutf::tables::normalization::nfc::trie_data[index + masked];
  } else {
    index = simdutf::tables::normalization::nfkc::trie_index[shift];
    value = simdutf::tables::normalization::nfkc::trie_data[index + masked];
  }
  return value;
}

template <ComposedForm form> bool is_relevant(uint32_t code_point) {
  if (code_point <= 0xFFFF) {
    return lookup_comp_trie<form>(uint16_t(code_point)) > 0;
  }
  constexpr auto dform = to_decomposed_form(form);
  uint64_t kv =
      utf8_to_decomposed::lookup_supplementary_code_point<dform>(code_point);
  uint32_t k = kv & 0x1FFFFF;
  if (k == code_point) {
    uint8_t qc = uint8_t(kv >> 56);
    return qc != 0;
  }
  return false;
}

std::pair<size_t, bool> rfind_starter(const char *input, size_t length) {
  size_t p = 0;
  while (p < length) {
    // Go back until leading UTF-8 byte
    while ((input[length - p - 1] & 0b11000000) == 0b10000000) {
      p++;
    }
    uint8_t size;
    uint32_t c = utf8::parse_code_point(input + (length - p - 1), &size);
    uint8_t ccc = normalization::lookup_ccc(c);
    // If we found a starter, then we're done
    if (ccc == 0) {
      return std::make_pair(length - p - 1, true);
    }
    p++;
  }
  return std::make_pair(0, false);
}

template <ComposedForm form>
std::pair<size_t, bool> find_first_stable(const char *input, size_t length) {
  uint32_t p = 0;
  while (p < length) {
    uint8_t size;
    uint32_t c = utf8::parse_code_point(input + p, &size);
    uint8_t ccc = normalization::lookup_ccc(c);
    if (ccc == 0 && !is_relevant<form>(c)) {
      return std::make_pair(p, true);
    }
    p += size;
  }
  return std::make_pair(0, false);
}

template <ComposedForm form>
std::pair<size_t, bool> find_last_stable(const char *input, size_t length) {
  size_t cutoff = length;
  while (cutoff > 0) {
    auto result = rfind_starter(input, cutoff);
    if (!result.second) {
      return std::make_pair(0, false);
    }
    cutoff = result.first;
    uint8_t size;
    uint32_t c = utf8::parse_code_point(input + cutoff, &size);
    if (!is_relevant<form>(c)) {
      return std::make_pair(cutoff, true);
    }
  }
  return std::make_pair(0, false);
}

void shift_right(char *buf, size_t length, size_t amt) {
  for (char *i = buf + length - 1; i >= buf; i--) {
    *(i + amt) = *i;
  }
}

void shift_left(char *buf, size_t length, size_t amt) {
  for (char *i = buf; i < buf + (length - amt); i++) {
    *i = *(i + amt);
  }
}

template <ComposedForm form, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
size_t normalize(InputPtr input, size_t length, OutputPtr out) {
  char *start{out};
  size_t p = 0;
  uint8_t last_ccc = 0;

  while (p < length) {
    uint8_t size;
    uint32_t c = utf8::parse_code_point(input + p, &size);

    // ASCII fast path to skip ccc lookup
    if (c <= 0x7F) {
      *out++ = (uint8_t)c;
      p++;
      last_ccc = 0;
      continue;
    }

    uint8_t ccc = normalization::lookup_ccc(c);

    // We can skip this character if it the combining classes are in the right
    // order and if it is irrelevant
    if (ccc <= last_ccc && !is_relevant<form>(c)) {
      for (size_t i = 0; i < size; i++) {
        *out++ = input[p + i];
      }
      p += size;
      last_ccc = ccc;
      continue;
    }

    last_ccc = ccc;

    // This starter should be NF(K)C irrelevant
    auto previous_starter_pos_result = rfind_starter(input, p);
    size_t previous_starter_pos = previous_starter_pos_result.first;
    // If we couldn't find a starter, use index 0
    if (!previous_starter_pos_result.second) {
      previous_starter_pos = 0;
    }

    auto next_irrelevant_starter_pos_result =
        find_first_stable<form>(input + p + size, length - p - size);
    size_t next_irrelevant_starter_pos =
        next_irrelevant_starter_pos_result.first;
    if (!next_irrelevant_starter_pos_result.second) {
      next_irrelevant_starter_pos = length;
    } else {
      next_irrelevant_starter_pos += p + size;
    }

    // Jump to the previous starter in the output. This is valid because
    // everything in between two stable code points should have a 1:1 mapping
    // between input and output for NF(K)C. So we can use the distance traveled
    // in the input to get distance traveled from the output.
    char *normalized_out = out - (p - previous_starter_pos);
    // NF(K)D normalize a localized region in between the two starters that are
    // NF(K)C irrelevant. This guarantees that, if we NF(K)C normalize this
    // range, no characters after the end of the range in the input will
    // combine/interact with the range we normalized. In other words, we run
    // NF(K)C on the largest possible sub-range of characters that may (or may
    // not) have to do with the NF(K)C relevant character `c`  that we initially
    // detected.
    constexpr auto dform = to_decomposed_form(form);
    size_t normalized_length = utf8_to_decomposed::normalize<dform>(
        input + previous_starter_pos,
        next_irrelevant_starter_pos - previous_starter_pos, normalized_out);

    size_t normalized_pos = 0;
    uint8_t normalized_last_ccc = 255;
    // Iterate through each code point, seeking back until a starter is found
    // and trying to combine with that. This part of the algorithm closely
    // matches up with the spec. See:
    // https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G49614
    while (normalized_pos < normalized_length) {
      uint8_t normalized_size;
      uint32_t normalized_c = utf8::parse_code_point(
          normalized_out + normalized_pos, &normalized_size);
      uint8_t normalized_ccc = normalization::lookup_ccc(normalized_c);

      // Find the preceding starter. It should be composition irrelevant
      // NOTE: we can cache this if it shows up in a profile
      auto starter_pos_result = rfind_starter(normalized_out, normalized_pos);
      size_t starter_pos = starter_pos_result.first;
      // Skip if we don't have a starter before this
      if (!starter_pos_result.second) {
        normalized_pos += normalized_size;
        normalized_last_ccc = normalized_ccc;
        continue;
      }

      uint8_t starter_size;
      uint32_t starter =
          utf8::parse_code_point(normalized_out + starter_pos, &starter_size);
      // Skip if we're blocked from the starter
      if (normalized_ccc <= normalized_last_ccc &&
          starter_pos + starter_size != normalized_pos) {
        normalized_pos += normalized_size;
        normalized_last_ccc = normalized_ccc;
        continue;
      }

      uint32_t composed;
      if (starter <= 0xFFFF && normalized_c <= 0xFFFF) {
        composed = normalization::compose_bmp(uint16_t(starter),
                                              uint16_t(normalized_c));
      } else {
        composed = simdutf::tables::normalization::compose_supplementary(
            starter, normalized_c);
      }
      // Skip if no composed character
      if (composed == 0) {
        normalized_pos += normalized_size;
        normalized_last_ccc = normalized_ccc;
        continue;
      }
      size_t composed_size = utf8::code_point_size(composed);

      // Shift left to delete the combining character
      shift_left(normalized_out + normalized_pos,
                 normalized_length - normalized_pos, normalized_size);
      // Account for combining character deletion
      normalized_length -= normalized_size;

      // Shift everything right to make room for new composed code point
      shift_right(normalized_out + starter_pos + starter_size,
                  normalized_length - starter_pos - starter_size,
                  composed_size - starter_size);
      // Overwrite the starter with the new composed code point
      (void)utf8::write_code_point(composed, normalized_out + starter_pos);
      normalized_length += composed_size - starter_size;
      normalized_pos += composed_size - starter_size;
    }

    // Set the out pointer to the end of the normalized buffer
    out = normalized_out + normalized_length;
    // Set the input offset to the next starter that is garuanteed to not be
    // relevant to NF(K)C
    p = next_irrelevant_starter_pos;
  }

  return out - start;
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

template <ComposedForm form, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
size_t normalize_with_context(InputPtr input, InputPtr input_base,
                              size_t input_length, OutputPtr *out,
                              size_t length) {
  size_t offset = input - input_base;
  // Get the region that we will NF(K)C normalize.
  uint8_t first_size;
  utf8::parse_code_point(input, &first_size);
  auto prev_starter_result =
      find_last_stable<form>(input_base, offset + first_size);
  size_t prev_starter =
      prev_starter_result.second ? prev_starter_result.first : 0;
  auto next_starter_result = find_first_stable<form>(
      input_base + offset + length, input_length - offset - length);
  size_t next_starter = next_starter_result.second
                            ? next_starter_result.first + offset + length
                            : input_length;
  size_t region_size = next_starter - prev_starter;
  size_t code_point_dist =
      utf8::count_code_points(input_base + prev_starter, offset - prev_starter);
  // This is the position we will write to. It is the same number of code
  // points away that the tail of the input is from the previous starter
  // code point. This property being true is an important invariant in the
  // algorithm, because we need to know where the left boundary of the
  // region we found is in the output buffer.
  size_t prev_out_offset = get_code_point_pos_reverse(*out, code_point_dist);
  char *prev_out = *out - prev_out_offset;
  size_t nwritten =
      normalize<form>(input_base + prev_starter, region_size, prev_out);
  *out = prev_out + nwritten;

  return next_starter - offset;
}

template <ComposedForm form>
simdutf_really_inline uint16_t lookup_check_trie(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint16_t index;
  uint16_t value;
  if constexpr (form == ComposedForm::NFC) {
    index = simdutf::tables::utf8_to_composed::nfc::check_trie_index[shift];
    value =
        simdutf::tables::utf8_to_composed::nfc::check_trie_data[index + masked];
  } else {
    index = simdutf::tables::utf8_to_composed::nfkc::check_trie_index[shift];
    value = simdutf::tables::utf8_to_composed::nfkc::check_trie_data[index +
                                                                     masked];
  }
  return value;
}

template <ComposedForm form>
bool check_code_point_bmp(uint16_t code_point, size_t *out_length,
                          uint8_t *ccc) {
  uint16_t value = lookup_check_trie<form>(code_point);
  *out_length += value & 0x3F;
  *ccc = uint8_t((value >> 6) & 0xFF);
  return !(value >> 15);
}

template <ComposedForm form>
static bool check_code_point_supplementary(uint32_t code_point,
                                           size_t *out_length, uint8_t *ccc) {
  constexpr auto dform = to_decomposed_form(form);
  uint64_t kv =
      utf8_to_decomposed::lookup_supplementary_code_point<dform>(code_point);
  uint32_t k = kv & 0x1FFFFF;
  if (k == code_point) {
    size_t length = 0;
    uint16_t offset = (kv >> 21) & 0xFFFF;
    uint32_t const *chars;
    if constexpr (form == ComposedForm::NFC) {
      chars = &simdutf::tables::normalization::nfd::lookup_chars[offset];
    } else {
      chars = &simdutf::tables::normalization::nfkd::lookup_chars[offset];
    }
    uint8_t len = (kv >> 53) & 0b11;
    for (size_t j = 0; j < len; j++) {
      length += utf8::code_point_size(chars[j]);
    }
    *out_length += length;
    *ccc = (kv >> 45) & 0xFF;
    uint8_t qc = uint8_t(kv >> 56);
    return qc == 0;
  } else {
    *out_length += 4;
    *ccc = 0;
    return true;
  }
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
