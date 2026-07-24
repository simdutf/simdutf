#ifndef SIMDUTF_NORMALIZATION_H
#define SIMDUTF_NORMALIZATION_H

namespace simdutf {
namespace scalar {
namespace {
namespace normalization {

// Hangul decomposition constants
const uint16_t s_base = 0xAC00;
const uint16_t l_base = 0x1100;
const uint16_t v_base = 0x1161;
const uint16_t t_base = 0x11A7;
const uint16_t l_count = 19;
const uint16_t v_count = 21;
const uint16_t t_count = 28;
const uint16_t n_count = v_count * t_count;
const uint16_t s_count = l_count * n_count;

bool is_hangul_syllable(uint32_t code_point) {
  return code_point >= s_base && code_point < s_base + s_count;
}

// Must be kept in sync with the hash function that generates the corresponding
// tables
uint32_t phash(uint32_t key, uint32_t salt, uint64_t size) {
  uint32_t salt_key = key + salt;
  uint32_t y1 = salt_key * 2654435769u;
  uint32_t y2 = key * 0x31415926u;
  uint32_t y = y1 ^ y2;
  uint64_t mh = (uint64_t)y * size;
  return uint32_t(mh >> 32);
}

// Get combining character class of code point
uint8_t lookup_ccc(uint32_t code_point) {
  if (code_point <= 0xFFFF) {
    uint16_t shift = uint16_t(code_point) >> 6;
    uint16_t masked = code_point & 63;
    uint16_t index = simdutf::tables::normalization::ccc_trie_index[shift];
    uint8_t value =
        simdutf::tables::normalization::ccc_trie_data[index + masked];
    return value;
  }
  constexpr const size_t table_size =
      sizeof(simdutf::tables::normalization::ccc_kv) / sizeof(uint32_t);
  uint32_t salt_hash = phash(code_point, 0, table_size);
  uint32_t salt = simdutf::tables::normalization::ccc_salt[salt_hash];
  uint32_t key_hash = phash(code_point, salt, table_size);
  uint32_t kv = simdutf::tables::normalization::ccc_kv[key_hash];
  uint32_t k = kv & 0x1FFFFF;
  if (k == code_point) {
    uint8_t ccc = uint8_t(kv >> 21);
    return ccc;
  }
  return 0;
}

// Try to compose two BMP code points into a single code point. Returns the
// composed code point if the composition is valid, or zero if the composition
// is not valid.
uint32_t compose_bmp(uint16_t c1, uint16_t c2) {
  if (c1 >= l_base && c1 < l_base + l_count && c2 >= v_base &&
      c2 < v_base + v_count) {
    uint32_t l_index = c1 - l_base;
    uint32_t v_index = c2 - v_base;
    uint32_t lv_index = l_index * n_count + v_index * t_count;
    return s_base + lv_index;
  }
  // Check if we have an LV syllable and a T jamo. Note that we check c2 >
  // UNIDATA_T_BASE, not c2 >= UNIDATA_T_BASE for a good reason: the first
  // valid T jamo is UNIDATA_T_BASE + 1! The spec defines the T base constant
  // to be off by one in order to make the math for algorithmic decomposition
  // cleaner.
  //
  // See:
  // https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G59434
  if (c1 >= s_base && c1 < s_base + s_count && (c1 - s_base) % t_count == 0 &&
      c2 > t_base && c2 < t_base + t_count) {
    return c1 + (c2 - t_base);
  }

  uint32_t wide = c1;
  uint32_t key = (wide << 16) | c2;
  constexpr size_t table_size =
      sizeof(simdutf::tables::normalization::compose_kv) /
      (sizeof(uint32_t) * 2);
  uint32_t salt_hash = phash(key, 0, table_size);
  uint32_t salt = simdutf::tables::normalization::compose_salt[salt_hash];
  uint32_t key_hash = phash(key, salt, table_size);
  uint32_t k = simdutf::tables::normalization::compose_kv[key_hash][1];
  uint32_t comp = simdutf::tables::normalization::compose_kv[key_hash][0];
  if (k == key) {
    // The composition is valid, return the composed code point
    return comp;
  } else {
    return 0;
  }
}

template <DecomposedForm form>
uint64_t lookup_supplementary_code_point(uint32_t code_point) {
  constexpr const uint64_t table_size =
      form == DecomposedForm::NFD
          ? sizeof(simdutf::tables::normalization::nfd::lookup_kv) /
                sizeof(uint64_t)
          : sizeof(simdutf::tables::normalization::nfkd::lookup_kv) /
                sizeof(uint64_t);
  uint32_t salt_hash = phash(code_point, 0, table_size);
  uint32_t salt;
  if constexpr (form == DecomposedForm::NFD) {
    salt = simdutf::tables::normalization::nfd::lookup_salt[salt_hash];
  } else {
    salt = simdutf::tables::normalization::nfkd::lookup_salt[salt_hash];
  }
  uint32_t key_hash = phash(code_point, salt, table_size);
  uint64_t kv;
  if constexpr (form == DecomposedForm::NFD) {
    kv = simdutf::tables::normalization::nfd::lookup_kv[key_hash];
  } else {
    kv = simdutf::tables::normalization::nfkd::lookup_kv[key_hash];
  }
  return kv;
}

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
  uint64_t kv = lookup_supplementary_code_point<dform>(code_point);
  uint32_t k = kv & 0x1FFFFF;
  if (k == code_point) {
    uint8_t qc = uint8_t(kv >> 56);
    return qc != 0;
  }
  return false;
}

// Reverse a subsection of an array.
template <typename T> void reverse(T *array, size_t start, size_t end) {
  while (start < end) {
    T tmp = array[start];
    array[start] = array[end];
    array[end] = tmp;
    start++;
    end--;
  }
}

// Rotate a subsection of an array to the right by k positions.
template <typename T> void rotate(T *array, size_t size, size_t k) {
  reverse(array, 0, size - 1);
  reverse(array, 0, k - 1);
  reverse(array, k, size - 1);
}

template <typename T> void shift_right(T *buf, size_t length, size_t amt) {
  for (T *i = buf + length - 1; i >= buf; i--) {
    *(i + amt) = *i;
  }
}

template <typename T> void shift_left(T *buf, size_t length, size_t amt) {
  for (T *i = buf; i < buf + (length - amt); i++) {
    *i = *(i + amt);
  }
}

struct utf8_normalization_traits {
  using char_type = char;

  static uint32_t parse_code_point(const char_type *input, uint8_t *size) {
    return utf8::parse_code_point(input, size);
  }

  static size_t write_code_point(uint32_t code_point, char_type *output) {
    return utf8::write_code_point(code_point, output);
  }

  static size_t code_point_size(uint32_t code_point) {
    return utf8::code_point_size(code_point);
  }

  static bool is_continuation(char_type unit) {
    return (unit & 0b11000000) == 0b10000000;
  }
};

template <endianness big_endian> struct utf16_normalization_traits {
  using char_type = char16_t;

  static uint32_t parse_code_point(const char_type *input, uint8_t *size) {
    return utf16::parse_code_point<big_endian>(input, size);
  }

  static size_t write_code_point(uint32_t code_point, char_type *output) {
    return utf16::write_code_point<big_endian>(code_point, output);
  }

  static size_t code_point_size(uint32_t code_point) {
    return utf16::code_point_size(code_point);
  }

  static bool is_continuation(char_type unit) {
    return utf16::is_low_surrogate<big_endian>(unit);
  }
};

// Walk backwards from a pointer to the last code unit of a code point,
// returning a pointer to its first code unit.
template <typename Traits, typename CharPtr>
CharPtr find_code_point_start_reverse(CharPtr ptr) {
  while (Traits::is_continuation(*ptr)) {
    ptr--;
  }
  return ptr;
}

// Get the code unit position of the nth code point going backwards from buf.
template <typename Traits>
simdutf_really_inline size_t
get_code_point_pos_reverse(const typename Traits::char_type *buf, size_t n) {
  if (n == 0) {
    return 0;
  }
  size_t count = n;
  size_t p = 0;
  while (true) {
    while (Traits::is_continuation(*(buf - p - 1))) {
      p++;
    }
    count--;
    if (count == 0) {
      return p + 1;
    }
    p++;
  }
}

template <typename Traits>
std::pair<size_t, bool> rfind_starter(const typename Traits::char_type *input,
                                      size_t length) {
  size_t p = 0;
  while (p < length) {
    auto start = find_code_point_start_reverse<Traits>(input + length - p - 1);
    uint8_t size;
    uint32_t c = Traits::parse_code_point(start, &size);
    uint8_t ccc = lookup_ccc(c);
    // If we found a starter, then we're done
    if (ccc == 0) {
      return std::make_pair(size_t(start - input), true);
    }
    p += size;
  }
  return std::make_pair(0, false);
}

template <typename Traits, ComposedForm form>
std::pair<size_t, bool>
find_first_stable(const typename Traits::char_type *input, size_t length) {
  size_t p = 0;
  while (p < length) {
    uint8_t size;
    uint32_t c = Traits::parse_code_point(input + p, &size);
    uint8_t ccc = lookup_ccc(c);
    if (ccc == 0 && !is_relevant<form>(c)) {
      return std::make_pair(p, true);
    }
    p += size;
  }
  return std::make_pair(0, false);
}

template <typename Traits, ComposedForm form>
std::pair<size_t, bool>
find_last_stable(const typename Traits::char_type *input, size_t length) {
  size_t cutoff = length;
  while (cutoff > 0) {
    auto result = rfind_starter<Traits>(input, cutoff);
    if (!result.second) {
      return std::make_pair(0, false);
    }
    cutoff = result.first;
    uint8_t size;
    uint32_t c = Traits::parse_code_point(input + cutoff, &size);
    if (!is_relevant<form>(c)) {
      return std::make_pair(cutoff, true);
    }
  }
  return std::make_pair(0, false);
}

// In-place canonical ordering as defined by the specification.
template <typename Traits>
uint8_t sort_combining(typename Traits::char_type *output, size_t len) {
  using char_type = typename Traits::char_type;
  if (len == 0) {
    return 0;
  }

  char_type *start{output};
  // Tracks the ccc of the final character in the sorting range.
  uint8_t final_ccc = 255;

  // We need to walk backwards until we find a starter character.
  uint8_t last_ccc = 255;
  bool needs_sort = false;
  output--;
  while (true) {
    output = find_code_point_start_reverse<Traits>(output);
    uint8_t size;
    uint32_t code_point = Traits::parse_code_point(output, &size);
    uint8_t ccc = lookup_ccc(code_point);
    if (final_ccc == 255) {
      final_ccc = ccc;
    }
    if (last_ccc < ccc) {
      needs_sort = true;
    }
    // If we found a starter or reached the start of the buffer, then we're
    // done
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

  // We do bubble sort on starting at the starter code point, up until the
  // next starter. The implementation supports sorting any number of
  // combining characters with no memory allocation. Sorting is thus done
  // entirely in-place and still while all code points are in their encoded
  // form. In practice, n will be small.
  size_t n = start - output;
  // This loop will run until we detect no more swaps, in which case we will
  // have sorted the buffer.
  while (true) {
    bool did_swap = false;
    uint8_t last_size;
    for (size_t j = 0; j < n; j += last_size) {
      uint8_t size1;
      uint8_t size2;
      uint32_t c1 = Traits::parse_code_point(output + j, &size1);
      // Going past the buffer is also a stop condition
      if (j + size1 >= n) {
        break;
      }
      uint32_t c2 = Traits::parse_code_point(output + j + size1, &size2);
      uint8_t ccc1 = lookup_ccc(c1);
      uint8_t ccc2 = lookup_ccc(c2);
      last_size = size1;
      if (ccc1 > ccc2) {
        // Swapping two adjacent, variably sized encoded code points can be
        // done with a right rotation by the size of the right code point.
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

// Canonically compose buf in place: repeatedly find, for each
// composition-relevant code point, its preceding starter and try to combine the
// two into a single code point. This closely follows the specification. See:
// https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G49614
template <typename Traits, ComposedForm form>
size_t compose_canonical(typename Traits::char_type *buf, size_t length) {
  size_t pos = 0;
  uint8_t last_ccc = 255;
  while (pos < length) {
    uint8_t size;
    uint32_t c = Traits::parse_code_point(buf + pos, &size);
    uint8_t ccc = lookup_ccc(c);

    // Find the preceding starter. It should be composition irrelevant
    // NOTE: we can cache this if it shows up in a profile
    auto starter_pos_result = rfind_starter<Traits>(buf, pos);
    size_t starter_pos = starter_pos_result.first;
    // Skip if we don't have a starter before this
    if (!starter_pos_result.second) {
      pos += size;
      last_ccc = ccc;
      continue;
    }

    uint8_t starter_size;
    uint32_t starter =
        Traits::parse_code_point(buf + starter_pos, &starter_size);
    // Skip if we're blocked from the starter
    if (ccc <= last_ccc && starter_pos + starter_size != pos) {
      pos += size;
      last_ccc = ccc;
      continue;
    }

    uint32_t composed;
    if (starter <= 0xFFFF && c <= 0xFFFF) {
      composed = compose_bmp(uint16_t(starter), uint16_t(c));
    } else {
      composed =
          simdutf::tables::normalization::compose_supplementary(starter, c);
    }
    // Skip if no composed character
    if (composed == 0) {
      pos += size;
      last_ccc = ccc;
      continue;
    }
    size_t composed_size = Traits::code_point_size(composed);

    // Shift left to delete the combining character
    shift_left(buf + pos, length - pos, size);
    // Account for combining character deletion
    length -= size;

    // Shift everything right to make room for new composed code point
    shift_right(buf + starter_pos + starter_size,
                length - starter_pos - starter_size,
                composed_size - starter_size);
    // Overwrite the starter with the new composed code point
    (void)Traits::write_code_point(composed, buf + starter_pos);
    length += composed_size - starter_size;
    pos += composed_size - starter_size;
  }
  return length;
}

} // namespace normalization
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_NORMALIZATION_H
