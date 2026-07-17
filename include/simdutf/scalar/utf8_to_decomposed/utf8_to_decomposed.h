#ifndef SIMDUTF_UTF8_TO_DECOMPOSED_H
#define SIMDUTF_UTF8_TO_DECOMPOSED_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_decomposed {

// Reverse a subsection of an array.
void reverse(char *array, size_t start, size_t end) {
  while (start < end) {
    char tmp = array[start];
    array[start] = array[end];
    array[end] = tmp;
    start++;
    end--;
  }
}

// Rotate a subsection of an array to the right by k positions.
void rotate(char *array, size_t size, size_t k) {
  reverse(array, 0, size - 1);
  reverse(array, 0, k - 1);
  reverse(array, k, size - 1);
}

// In-place canonical ordering as defined by the specification.
uint8_t sort_combining(char *output, size_t len) {
  if (len == 0) {
    return 0;
  }

  char *start{output};
  // Tracks the ccc of the final character in the sorting range.
  uint8_t final_ccc = 255;

  // We need to walk backwards until we find a starter character.
  uint8_t last_ccc = 255;
  bool needs_sort = false;
  output--;
  while (true) {
    // Backwards until leading UTF-8 byte
    while ((*output & 0b11000000) == 0b10000000) {
      output--;
    }
    uint8_t size;
    uint32_t code_point = utf8::parse_code_point(output, &size);
    uint8_t ccc = normalization::lookup_ccc(code_point);
    if (final_ccc == 255) {
      final_ccc = ccc;
    }
    if (last_ccc < ccc) {
      needs_sort = true;
    }
    // If we found a starter or reached the start of the buffer, then we're done
    if (ccc == 0 || size_t(start - output) == len) {
      break;
    }
    output--;
    last_ccc = ccc;
  }

  // Fast path if the combining characters are already sorted
  if (!needs_sort) {
    return final_ccc;
  }

  // We do bubble sort on starting at the starter code point, up until the next
  // starter. The implementation supports sorting any number of combining
  // characters with no memory allocation. Sorting is thus done entirely
  // in-place and still while all code points are in UTF-8-encoded form. In
  // practice, n will be small.
  size_t n = start - output;
  // This loop will run until we detect no more swaps, in which case we will
  // have sorted the buffer.
  while (true) {
    bool did_swap = false;
    uint8_t last_size;
    for (size_t j = 0; j < n; j += last_size) {
      uint8_t size1;
      uint8_t size2;
      uint32_t c1 = utf8::parse_code_point(output + j, &size1);
      // Going past the buffer is also a stop condition
      if (j + size1 >= n) {
        break;
      }
      uint32_t c2 = utf8::parse_code_point(output + j + size1, &size2);
      uint8_t ccc1 = normalization::lookup_ccc(c1);
      uint8_t ccc2 = normalization::lookup_ccc(c2);
      last_size = size1;
      if (ccc1 > ccc2) {
        // Swapping two adjacent, variably sized UTF-8 encoded code points can
        // be done with a right rotation by the size of the right code point.
        rotate(output + j, size1 + size2, size2);
        last_size = size2;
        did_swap = true;
        if (j + size1 + size2 == n) {
          // Swapped the last character in the sorting range, so update
          // `final_ccc`
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

template <DecomposedForm form>
simdutf_really_inline uint16_t lookup_narrow_trie(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint16_t value;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index =
        simdutf::tables::utf8_to_decomposed::nfd::trie_index[shift];
    value = simdutf::tables::utf8_to_decomposed::nfd::trie_data[index + masked];
  } else {
    uint16_t index =
        simdutf::tables::utf8_to_decomposed::nfkd::trie_index[shift];
    value =
        simdutf::tables::utf8_to_decomposed::nfkd::trie_data[index + masked];
  }
  return value;
}

template <DecomposedForm form>
simdutf_really_inline uint32_t lookup_full_trie(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint32_t value;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index =
        simdutf::tables::utf8_to_decomposed::nfd::full_trie_index[shift];
    value = simdutf::tables::utf8_to_decomposed::nfd::full_trie_data[index +
                                                                     masked];
  } else {
    uint16_t index =
        simdutf::tables::utf8_to_decomposed::nfkd::full_trie_index[shift];
    value = simdutf::tables::utf8_to_decomposed::nfkd::full_trie_data[index +
                                                                      masked];
  }
  return value;
}

// Decompose character in BMP
template <DecomposedForm form>
size_t decompose_bmp(uint16_t code_point, char *output, uint8_t *first_ccc,
                     uint8_t *ccc) {
  char *start{output};
  uint32_t value = lookup_full_trie<form>(code_point);
  if (value == 0) {
    return 0;
  }
  *ccc = (value >> 21) & 0xFF;
  uint16_t offset = value & 0x7FFF;
  uint8_t length = (value >> 15) & 0x3F;
  const uint8_t *bytes =
      &simdutf::tables::utf8_to_decomposed::decompositions[offset];
  for (size_t k = 0; k < length; k++) {
    *output++ = bytes[k];
  }
  uint8_t ccc_delta = uint8_t(value >> 29);
  *first_ccc = ccc_delta == 0 ? 0 : *ccc - ccc_delta;
  return output - start;
}

template <DecomposedForm form>
uint64_t lookup_supplementary_code_point(uint32_t code_point) {
  constexpr const uint64_t table_size =
      form == DecomposedForm::NFD
          ? sizeof(simdutf::tables::normalization::nfd::lookup_kv) /
                sizeof(uint64_t)
          : sizeof(simdutf::tables::normalization::nfkd::lookup_kv) /
                sizeof(uint64_t);
  uint32_t salt_hash = normalization::phash(code_point, 0, table_size);
  uint32_t salt;
  if constexpr (form == DecomposedForm::NFD) {
    salt = simdutf::tables::normalization::nfd::lookup_salt[salt_hash];
  } else {
    salt = simdutf::tables::normalization::nfkd::lookup_salt[salt_hash];
  }
  uint32_t key_hash = normalization::phash(code_point, salt, table_size);
  uint64_t kv;
  if constexpr (form == DecomposedForm::NFD) {
    kv = simdutf::tables::normalization::nfd::lookup_kv[key_hash];
  } else {
    kv = simdutf::tables::normalization::nfkd::lookup_kv[key_hash];
  }
  return kv;
}

// Decompose character in supplementary plane
template <DecomposedForm form>
size_t decompose_supplementary(uint32_t code_point, char *output,
                               uint8_t *first_ccc, uint8_t *ccc) {
  char *start{output};
  uint64_t kv = lookup_supplementary_code_point<form>(code_point);
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
      output += utf8::write_code_point(chars[j], output);
    }
    uint8_t last_ccc = (kv >> 37) & 0xFF;
    *ccc = last_ccc;
  } else {
    *ccc = 0;
  }
  // first_ccc doesn't exist for any code points in the supplementary plane, for
  // now
  *first_ccc = 0;
  return output - start;
}

// Hangul syllables can be decomposed algorithmically
size_t decompose_hangul(uint32_t code_point, char *out) {
  uint32_t s_index = code_point - normalization::s_base;
  uint32_t l_index = s_index / normalization::n_count;
  uint32_t v_index =
      (s_index % normalization::n_count) / normalization::t_count;
  uint32_t t_index = s_index % normalization::t_count;

  size_t nwritten = 0;
  utf8::write_3_byte_code_point(uint16_t(normalization::l_base + l_index), out);
  nwritten += 3;
  utf8::write_3_byte_code_point(uint16_t(normalization::v_base + v_index),
                                out + nwritten);
  nwritten += 3;
  if (t_index > 0) {
    utf8::write_3_byte_code_point(uint16_t(normalization::t_base + t_index),
                                  out + nwritten);
    nwritten += 3;
  }
  return nwritten;
}

template <DecomposedForm form, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 size_t normalize_with_context(InputPtr data, size_t len,
                                                  OutputPtr output,
                                                  size_t out_offset,
                                                  uint8_t *last_ccc) {
  size_t pos = 0;
  char *start{output};
  while (pos < len) {
    uint8_t first_ccc = 0;
    uint8_t ccc = 0;
    uint8_t leading_byte = data[pos];
    if (leading_byte < 0b10000000) {
      // ASCII, no need to do a lookup
      *output++ = leading_byte;
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // Two-byte UTF-8
      uint32_t code_point =
          (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      size_t nwritten =
          decompose_bmp<form>(uint16_t(code_point), output, &first_ccc, &ccc);
      if (nwritten == 0) {
        *output++ = leading_byte;
        *output++ = data[pos + 1];
      } else {
        output += nwritten;
      }
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // Three-byte UTF-8
      uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                            (data[pos + 1] & 0b00111111) << 6 |
                            (data[pos + 2] & 0b00111111);
      // Hangul syllables are only possible in 3-byte UTF-8
      if (normalization::is_hangul_syllable(code_point)) {
        output += decompose_hangul(code_point, output);
      } else {
        size_t nwritten =
            decompose_bmp<form>(uint16_t(code_point), output, &first_ccc, &ccc);
        if (nwritten == 0) {
          *output++ = leading_byte;
          *output++ = data[pos + 1];
          *output++ = data[pos + 2];
        } else {
          output += nwritten;
        }
      }
      pos += 3;
    } else {
      // Four-byte UTF-8
      uint32_t code_point = (leading_byte & 0b00000111) << 18 |
                            (data[pos + 1] & 0b00111111) << 12 |
                            (data[pos + 2] & 0b00111111) << 6 |
                            (data[pos + 3] & 0b00111111);
      size_t nwritten =
          decompose_supplementary<form>(code_point, output, &first_ccc, &ccc);
      if (nwritten == 0) {
        *output++ = leading_byte;
        *output++ = data[pos + 1];
        *output++ = data[pos + 2];
        *output++ = data[pos + 3];
      } else {
        output += nwritten;
      }
      pos += 4;
    }
    uint8_t cmp_ccc = first_ccc > 0 ? first_ccc : ccc;
    if (cmp_ccc != 0 && *last_ccc > cmp_ccc) {
      ccc = sort_combining(output, (output - start) + out_offset);
    }
    *last_ccc = ccc;
  }
  return output - start;
}

template <DecomposedForm form, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 size_t normalize(InputPtr data, size_t len,
                                     OutputPtr output) {
  uint8_t last_ccc = 0;
  return normalize_with_context<form>(data, len, output, 0, &last_ccc);
}

template <DecomposedForm form>
simdutf_really_inline uint16_t lookup_check_trie(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint16_t value;
  if constexpr (form == DecomposedForm::NFD) {
    uint16_t index =
        simdutf::tables::utf8_to_decomposed::nfd::check_trie_index[shift];
    value = simdutf::tables::utf8_to_decomposed::nfd::check_trie_data[index +
                                                                      masked];
  } else {
    uint16_t index =
        simdutf::tables::utf8_to_decomposed::nfkd::check_trie_index[shift];
    value = simdutf::tables::utf8_to_decomposed::nfkd::check_trie_data[index +
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
  uint64_t kv = lookup_supplementary_code_point<form>(code_point);
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
      length += utf8::code_point_size(chars[j]);
    }
    *out_length += length;
    *ccc = (kv >> 45) & 0xFF;
    uint8_t qc = (kv >> 55) & 1;
    // Check 0th bit in kv.qc. If this bit is set, then we fail the NF(K)D quick
    // check
    return qc == 0;
  } else {
    *out_length += 4;
    *ccc = 0;
    return true;
  }
}

template <DecomposedForm form, typename InputPtr>
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

template <DecomposedForm form, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr>)
#endif
bool check(InputPtr input, size_t length, size_t *out_length) {
  *out_length = 0;
  uint8_t last_ccc = 0;
  return check_with_context<form>(input, length, out_length, &last_ccc);
}

} // namespace utf8_to_decomposed
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_UTF8_TO_DECOMPOSED_H
