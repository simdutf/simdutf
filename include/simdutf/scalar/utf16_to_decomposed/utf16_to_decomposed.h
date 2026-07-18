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

template <endianness big_endian>
uint8_t sort_combining(char16_t *out, size_t length) {
  if (length == 0) {
    return 0;
  }

  char16_t *start = out;
  uint8_t final_ccc = 255;

  uint8_t last_ccc = 255;
  bool needs_sort = false;
  out--;
  while (true) {
    uint32_t code_point = parse_code_point_reverse<big_endian>(out);
    if (code_point > 0xFFFF) {
      out--;
    }
    uint8_t ccc = normalization::lookup_ccc(code_point);
    if (final_ccc == 255) {
      final_ccc = ccc;
    }
    if (last_ccc < ccc) {
      needs_sort = true;
    }
    // Walk back until we have found the last starter
    if (ccc == 0 || size_t(start - out) == length) {
      break;
    }
    out--;
    last_ccc = ccc;
  }

  // Fast path for when the buffer is already sorted
  if (!needs_sort) {
    return final_ccc;
  }

  size_t n = start - out;
  while (true) {
    bool did_swap = false;
    uint8_t last_size;
    for (size_t j = 0; j < n; j += last_size) {
      uint8_t size1;
      uint8_t size2;
      uint32_t c1 = utf16::parse_code_point<big_endian>(out + j, &size1);
      if (j + size1 >= n) {
        break;
      }
      uint32_t c2 =
          utf16::parse_code_point<big_endian>(out + j + size1, &size2);
      uint8_t ccc1 = normalization::lookup_ccc(c1);
      uint8_t ccc2 = normalization::lookup_ccc(c2);
      last_size = size1;
      if (ccc1 > ccc2) {
        normalization::rotate(out + j, size1 + size2, size2);
        last_size = size2;
        did_swap = true;
        if (j + size1 + size2 == n) {
          final_ccc = ccc1;
        }
      }
    }
    if (!did_swap) {
      break;
    }
  }
  return final_ccc;
}

template <endianness big_endian, DecomposedForm form>
simdutf_really_inline size_t decompose_supplementary(uint32_t code_point,
                                                     char16_t *output,
                                                     uint8_t *first_ccc,
                                                     uint8_t *ccc) {
  char16_t *start{output};
  uint64_t kv =
      normalization::lookup_supplementary_code_point<form>(code_point);
  uint32_t k = kv & 0x1FFFFF;
  if (k == code_point) {
    uint32_t const *chars;
    uint16_t offset = (kv >> 21) & 0xFFFF;
    if constexpr (form == DecomposedForm::NFD) {
      chars = &simdutf::tables::normalization::nfd::lookup_chars[offset];
    } else {
      chars = &simdutf::tables::normalization::nfkd::lookup_chars[offset];
    }
    uint8_t len = (kv >> 53) & 0b11;
    for (size_t j = 0; j < len; j++) {
      output += utf16::write_code_point<big_endian>(chars[j], output);
    }
    uint8_t last_ccc = (kv >> 37) & 0xFF;
    *ccc = last_ccc;
  } else {
    *ccc = 0;
  }
  // first_ccc doesn't exist for any code points in the supplementary plane,
  // for now
  *first_ccc = 0;
  return output - start;
}

// Decompose character in BMP
template <endianness big_endian, DecomposedForm form>
simdutf_really_inline size_t decompose_bmp(uint16_t code_point,
                                           char16_t *output, uint8_t *first_ccc,
                                           uint8_t *ccc) {
  char16_t *start{output};
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint32_t value;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index =
        simdutf::tables::utf16_to_decomposed::nfd::trie_index[shift];
    value =
        simdutf::tables::utf16_to_decomposed::nfd::trie_data[index + masked];
  } else {
    uint16_t index =
        simdutf::tables::utf16_to_decomposed::nfkd::trie_index[shift];
    value =
        simdutf::tables::utf16_to_decomposed::nfkd::trie_data[index + masked];
  }
  if (value == 0) {
    *ccc = 0;
    return 0;
  }
  *ccc = uint8_t(value >> 24);
  uint8_t delta = (value >> 14) & 0x3F;
  uint8_t length = delta + 2;
  uint16_t offset = value & 0x3FFF;
  const uint8_t *bytes;
  if constexpr (form == DecomposedForm::NFD) {
    bytes = &simdutf::tables::utf16_to_decomposed::nfd::decompositions[offset];
  } else {
    bytes = &simdutf::tables::utf16_to_decomposed::nfkd::decompositions[offset];
  }
  for (size_t k = 0; k < length; k += 2) {
    // The table stores each decomposed code unit as two little-endian
    // bytes. TODO: store instead as char16_t
    uint16_t unit = uint16_t(bytes[k]) | uint16_t(uint16_t(bytes[k + 1]) << 8);
    *output++ = char16_t(scalar::utf16::swap_if_needed<big_endian>(unit));
  }
  uint8_t ccc_delta = (value >> 20) & 0b111;
  *first_ccc = ccc_delta == 0 ? 0 : *ccc - ccc_delta;
  return output - start;
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
          *output++ = code_point;
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
      ccc = sort_combining<big_endian>(output, (output - start) + out_offset);
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
simdutf_really_inline uint16_t lookup_check_trie(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint16_t value;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index =
        simdutf::tables::utf16_to_decomposed::nfd::check_trie_index[shift];
    value = simdutf::tables::utf16_to_decomposed::nfd::check_trie_data[index +
                                                                       masked];
  } else {
    uint16_t index =
        simdutf::tables::utf16_to_decomposed::nfkd::check_trie_index[shift];
    value = simdutf::tables::utf16_to_decomposed::nfkd::check_trie_data[index +
                                                                        masked];
  }
  return value;
}

template <DecomposedForm form>
bool check_code_point_bmp(uint16_t code_point, size_t *out_length,
                          uint8_t *ccc) {
  uint16_t value = lookup_check_trie<form>(code_point);
  *out_length += value & 0x3F;
  *ccc = uint8_t((value >> 6) & 0xFF);
  return !(value >> 15);
}

template <DecomposedForm form>
static bool check_code_point_supplementary(uint32_t code_point,
                                           size_t *out_length, uint8_t *ccc) {
  uint64_t kv =
      normalization::lookup_supplementary_code_point<form>(code_point);
  uint32_t k = kv & 0x1FFFFF;
  if (k == code_point) {
    size_t length = 0;
    uint16_t offset = (kv >> 21) & 0xFFFF;
    uint32_t const *chars;
    if constexpr (form == DecomposedForm::NFD) {
      chars = &simdutf::tables::normalization::nfd::lookup_chars[offset];
    } else {
      chars = &simdutf::tables::normalization::nfkd::lookup_chars[offset];
    }
    uint8_t len = (kv >> 53) & 0b11;
    for (size_t j = 0; j < len; j++) {
      // Each decomposed code point takes one UTF-16 code unit if it is BMP,
      // or a surrogate pair (two code units) otherwise.
      length += utf16::code_point_size(chars[j]);
    }
    *out_length += length;
    *ccc = (kv >> 45) & 0xFF;
    uint8_t qc = (kv >> 55) & 1;
    // Check 0th bit in kv.qc. If this bit is set, then we fail the NF(K)D
    // quick check
    return qc == 0;
  } else {
    // Code point has no decomposition and, since it took the supplementary
    // path, is guaranteed to require a surrogate pair (two code units) in
    // UTF-16.
    *out_length += 2;
    *ccc = 0;
    return true;
  }
}

template <endianness big_endian, DecomposedForm form>
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
