#ifndef SIMDUTF_UTF16_TO_DECOMPOSED_H
#define SIMDUTF_UTF16_TO_DECOMPOSED_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_decomposed {

template <endianness big_endian>
simdutf_really_inline size_t decompose_hangul(uint32_t code_point,
                                              char16_t *out) {
  uint32_t s_index = code_point - normalization::s_base;
  uint32_t l_index = s_index / normalization::n_count;
  uint32_t v_index =
      (s_index % normalization::n_count) / normalization::t_count;
  uint32_t t_index = s_index % normalization::t_count;

  size_t nwritten = 0;
  nwritten +=
      utf16::write_code_point<big_endian>(normalization::l_base + l_index, out);
  nwritten += utf16::write_code_point<big_endian>(
      normalization::v_base + v_index, out + nwritten);
  if (t_index > 0) {
    nwritten += utf16::write_code_point<big_endian>(
        normalization::t_base + t_index, out + nwritten);
  }
  return nwritten;
}

template <endianness big_endian>
uint32_t parse_code_point_reverse(const char16_t *input) {
  char16_t word = input[0];
  uint32_t code_point = 0;
  if (utf16::is_low_surrogate<big_endian>(word)) {
    // If we found a low surrogate, parse the matching high surrogate
    uint8_t size;
    code_point = utf16::parse_code_point<big_endian>(input - 1, &size);
  } else {
    // In this case, should be BMP
    code_point = scalar::utf16::swap_if_needed<big_endian>(word);
  }
  return code_point;
}

template <DecomposedForm form>
simdutf_really_inline uint32_t lookup_full_trie_bmp(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint32_t value;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index =
        simdutf::tables::utf16_to_decomposed::nfd::trie_index[shift];
    value =
        simdutf::tables::utf16_to_decomposed::trie_data[index + masked];
  } else {
    uint16_t index =
        simdutf::tables::utf16_to_decomposed::nfkd::trie_index[shift];
    value =
        simdutf::tables::utf16_to_decomposed::trie_data[index + masked];
  }
  return value;
}

template <DecomposedForm form>
simdutf_really_inline uint32_t
lookup_full_trie_supplementary(uint32_t code_point) {
  uint32_t supplementary = code_point - 0x10000;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index1 =
        simdutf::tables::utf16_to_decomposed::nfd::trie_index1[supplementary >>
                                                               11];
    uint16_t index2 = simdutf::tables::utf16_to_decomposed::nfd::trie_index2
        [index1 + ((supplementary >> 6) & 31)];
    return simdutf::tables::utf16_to_decomposed::trie_data[index2 +
                                                                (code_point &
                                                                 63)];
  } else {
    uint16_t index1 =
        simdutf::tables::utf16_to_decomposed::nfkd::trie_index1[supplementary >>
                                                                11];
    uint16_t index2 = simdutf::tables::utf16_to_decomposed::nfkd::trie_index2
        [index1 + ((supplementary >> 6) & 31)];
    return simdutf::tables::utf16_to_decomposed::trie_data[index2 +
                                                                 (code_point &
                                                                  63)];
  }
}

template <DecomposedForm form>
simdutf_really_inline uint32_t lookup_full_trie(uint32_t code_point) {
  if (code_point <= 0xFFFF) {
    return lookup_full_trie_bmp<form>(uint16_t(code_point));
  } else {
    return lookup_full_trie_supplementary<form>(code_point);
  }
}

template <endianness big_endian, DecomposedForm form>
const char16_t *find_first_stable(const char16_t *input, size_t length) {
  size_t p = 0;
  while (p < length) {
    uint8_t size;
    uint32_t c = utf16::parse_code_point<big_endian>(input + p, &size);
    uint8_t ccc = normalization::lookup_ccc(c);
    uint32_t value = lookup_full_trie<form>(c);
    if (ccc == 0 && value == 0) {
      return input + p;
    }
    p += size;
  }
  return input + length;
}

// Find the last stable code point boundary at or before `input + length`.
// Returns `input + length` if no such position exists.
template <endianness big_endian, DecomposedForm form>
const char16_t *find_last_stable(const char16_t *input, size_t length) {
  size_t cutoff = length;
  while (cutoff > 0) {
    auto result = normalization::rfind_starter<
        normalization::utf16_normalization_traits<big_endian>>(input, cutoff);
    if (!result.second) {
      return input + length;
    }
    cutoff = result.first;
    uint8_t size;
    uint32_t c = utf16::parse_code_point<big_endian>(input + cutoff, &size);
    uint32_t value = lookup_full_trie<form>(c);
    if (value == 0) {
      return input + cutoff;
    }
  }
  return input + length;
}

template <endianness big_endian>
simdutf_really_inline size_t write_decomposition(uint32_t value,
                                                 char16_t *output,
                                                 uint8_t *first_ccc,
                                                 uint8_t *ccc) {
  char16_t *start{output};
  *ccc = uint8_t(value >> 24);
  uint8_t delta = (value >> 14) & 0x3F;
  uint8_t length = delta + 1;
  uint16_t offset = value & 0x3FFF;
  const uint16_t *words =
      &simdutf::tables::utf16_to_decomposed::decompositions[offset];
  for (size_t k = 0; k < length; k++) {
    *output++ = char16_t(scalar::utf16::swap_if_needed<big_endian>(words[k]));
  }
  uint8_t ccc_delta = (value >> 20) & 0b111;
  *first_ccc = ccc_delta == 0 ? 0 : *ccc - ccc_delta;
  return output - start;
}

// Decompose character in BMP
template <endianness big_endian, DecomposedForm form>
simdutf_really_inline size_t decompose_bmp(uint16_t code_point,
                                           char16_t *output, uint8_t *first_ccc,
                                           uint8_t *ccc) {
  uint32_t value = lookup_full_trie_bmp<form>(code_point);
  if (value == 0) {
    *ccc = 0;
    return 0;
  }
  return write_decomposition<big_endian>(value, output, first_ccc, ccc);
}

// Decompose character in supplementary plane
template <endianness big_endian, DecomposedForm form>
simdutf_really_inline size_t decompose_supplementary(uint32_t code_point,
                                                     char16_t *output,
                                                     uint8_t *first_ccc,
                                                     uint8_t *ccc) {
  uint32_t value = lookup_full_trie_supplementary<form>(code_point);
  if (value == 0) {
    *ccc = 0;
    return 0;
  }
  return write_decomposition<big_endian>(value, output, first_ccc, ccc);
}

template <endianness big_endian, DecomposedForm form>
size_t normalize_with_context(const char16_t *data, size_t len,
                              char16_t *output, size_t out_offset,
                              uint8_t *last_ccc) {
  size_t pos = 0;
  char16_t *start{output};
  while (pos < len) {
    uint8_t first_ccc = 0;
    uint8_t ccc = 0;
    if (simdutf_likely(!utf16::is_high_surrogate<big_endian>(data[pos]))) {
      uint16_t code_point =
          scalar::utf16::swap_if_needed<big_endian>(data[pos]);
      if (normalization::is_hangul_syllable(code_point)) {
        output += decompose_hangul<big_endian>(code_point, output);
      } else {
        size_t nwritten = decompose_bmp<big_endian, form>(code_point, output,
                                                          &first_ccc, &ccc);
        if (nwritten == 0) {
          *output++ = scalar::utf16::swap_if_needed<big_endian>(code_point);
        } else {
          output += nwritten;
        }
      }
      pos++;
    } else {
      uint8_t size;
      uint32_t code_point =
          utf16::parse_code_point<big_endian>(data + pos, &size);
      size_t nwritten = decompose_supplementary<big_endian, form>(
          code_point, output, &first_ccc, &ccc);
      if (nwritten == 0) {
        *output++ = data[pos];
        *output++ = data[pos + 1];
      } else {
        output += nwritten;
      }
      pos += 2;
    }
    uint8_t cmp_ccc = first_ccc > 0 ? first_ccc : ccc;
    if (cmp_ccc != 0 && *last_ccc > cmp_ccc) {
      ccc = normalization::sort_combining<
          normalization::utf16_normalization_traits<big_endian>>(
          output, (output - start) + out_offset);
    }
    *last_ccc = ccc;
  }

  return output - start;
}

template <endianness big_endian, DecomposedForm form>
size_t normalize(const char16_t *data, size_t len, char16_t *output) {
  uint8_t last_ccc = 0;
  return normalize_with_context<big_endian, form>(data, len, output, 0,
                                                  &last_ccc);
}

template <DecomposedForm form>
simdutf_really_inline uint16_t lookup_check_trie_bmp(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint16_t value;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index =
        simdutf::tables::utf16_to_decomposed::nfd::check_trie_index[shift];
    value = simdutf::tables::utf16_to_decomposed::check_trie_data[index +
                                                                       masked];
  } else {
    uint16_t index =
        simdutf::tables::utf16_to_decomposed::nfkd::check_trie_index[shift];
    value = simdutf::tables::utf16_to_decomposed::check_trie_data[index +
                                                                        masked];
  }
  return value;
}

template <DecomposedForm form>
simdutf_really_inline uint16_t
lookup_check_trie_supplementary(uint32_t code_point) {
  uint32_t supplementary = code_point - 0x10000;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index1 = simdutf::tables::utf16_to_decomposed::nfd::
        check_trie_index1[supplementary >> 11];
    uint16_t index2 = simdutf::tables::utf16_to_decomposed::nfd::
        check_trie_index2[index1 + ((supplementary >> 6) & 31)];
    return simdutf::tables::utf16_to_decomposed::check_trie_data
        [index2 + (code_point & 63)];
  } else {
    uint16_t index1 = simdutf::tables::utf16_to_decomposed::nfkd::
        check_trie_index1[supplementary >> 11];
    uint16_t index2 = simdutf::tables::utf16_to_decomposed::nfkd::
        check_trie_index2[index1 + ((supplementary >> 6) & 31)];
    return simdutf::tables::utf16_to_decomposed::check_trie_data
        [index2 + (code_point & 63)];
  }
}

template <DecomposedForm form>
simdutf_really_inline uint16_t lookup_check_trie(uint32_t code_point) {
  if (code_point <= 0xFFFF) {
    return lookup_check_trie_bmp<form>(uint16_t(code_point));
  } else {
    return lookup_check_trie_supplementary<form>(code_point);
  }
}

template <endianness big_endian, DecomposedForm form>
bool check_with_context(const char16_t *input, size_t length,
                        size_t *out_length, uint8_t *last_ccc) {
  bool is_qc = true;
  size_t p = 0;
  while (p < length) {
    uint16_t value;
    if (!utf16::is_high_surrogate<big_endian>(input[p])) {
      uint16_t code_point = scalar::utf16::swap_if_needed<big_endian>(input[p]);
      value = lookup_check_trie_bmp<form>(code_point);
      p++;
    } else {
      uint8_t size;
      uint32_t code_point =
          utf16::parse_code_point<big_endian>(input + p, &size);
      value = lookup_check_trie_supplementary<form>(code_point);
      p += 2;
    }
    *out_length += value & 0x3F;
    uint8_t ccc = uint8_t((value >> 6) & 0x3F);
    is_qc &= !(value >> 15);
    if (*last_ccc > ccc && ccc != 0) {
      is_qc = false;
    }
    *last_ccc = ccc;
  }
  return is_qc;
}

template <endianness big_endian, DecomposedForm form>
bool check(const char16_t *input, size_t length, size_t *out_length) {
  *out_length = 0;
  uint8_t last_ccc = 0;
  return check_with_context<big_endian, form>(input, length, out_length,
                                              &last_ccc);
}

} // namespace utf16_to_decomposed
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_UTF16_TO_DECOMPOSED_H
