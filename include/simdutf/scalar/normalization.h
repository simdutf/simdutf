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
  uint32_t supplementary = code_point - 0x10000;
  uint16_t index1 =
      simdutf::tables::normalization::ccc_trie_index1[supplementary >> 11];
  uint16_t index2 = simdutf::tables::normalization::ccc_trie_index2
      [index1 + ((supplementary >> 6) & 31)];
  return simdutf::tables::normalization::ccc_trie_data[index2 +
                                                       (code_point & 63)];
}

template <ComposedForm form>
simdutf_really_inline uint16_t lookup_comp_trie_bmp(uint16_t code_point) {
  uint16_t shift = code_point >> 6;
  uint16_t masked = code_point & 63;
  uint16_t index;
  uint16_t value;
  if constexpr (form == ComposedForm::NFC) {
    index = simdutf::tables::normalization::nfc::trie_index[shift];
    value = simdutf::tables::normalization::trie_data[index + masked];
  } else {
    index = simdutf::tables::normalization::nfkc::trie_index[shift];
    value = simdutf::tables::normalization::trie_data[index + masked];
  }
  return value;
}

template <ComposedForm form>
simdutf_really_inline uint16_t
lookup_comp_trie_supplementary(uint32_t code_point) {
  uint32_t supplementary = code_point - 0x10000;
  if constexpr (form == ComposedForm::NFC) {
    uint16_t index1 =
        simdutf::tables::normalization::nfc::trie_index1[supplementary >> 11];
    uint16_t index2 = simdutf::tables::normalization::nfc::trie_index2
        [index1 + ((supplementary >> 6) & 31)];
    return simdutf::tables::normalization::trie_data[index2 +
                                                     (code_point & 63)];
  } else {
    uint16_t index1 =
        simdutf::tables::normalization::nfkc::trie_index1[supplementary >> 11];
    uint16_t index2 = simdutf::tables::normalization::nfkc::trie_index2
        [index1 + ((supplementary >> 6) & 31)];
    return simdutf::tables::normalization::trie_data[index2 +
                                                     (code_point & 63)];
  }
}

template <ComposedForm form>
simdutf_really_inline uint16_t lookup_comp_trie(uint32_t code_point) {
  if (code_point <= 0xFFFF) {
    return lookup_comp_trie_bmp<form>(uint16_t(code_point));
  } else {
    return lookup_comp_trie_supplementary<form>(code_point);
  }
}

template <ComposedForm form>
simdutf_really_inline bool is_stable(uint32_t code_point) {
  return (lookup_comp_trie<form>(code_point) & 0b11) == 0;
}

template <ComposedForm form>
constexpr uint16_t min_relevant_cp =
    form == ComposedForm::NFC ? 0x0300 : 0x00A0;

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

// Find the first stable code point boundary at or after `input`. A position
// is stable for `form` if the code point there has a canonical combining
// class of zero and is not itself relevant to NF(K)C composition. Returns
// `input + length` (the end of the scanned range) if no such position
// exists.
template <typename Traits, ComposedForm form>
const typename Traits::char_type *
find_first_stable(const typename Traits::char_type *input, size_t length) {
  size_t p = 0;
  while (p < length) {
    uint8_t size;
    uint32_t c = Traits::parse_code_point(input + p, &size);
    if (is_stable<form>(c)) {
      return input + p;
    }
    p += size;
  }
  return input + length;
}

// Find the last stable code point boundary at or before `input + length`.
// Returns `input + length` if no such position exists.
template <typename Traits, ComposedForm form>
const typename Traits::char_type *
find_last_stable(const typename Traits::char_type *input, size_t length) {
  size_t p = length;
  while (p > 0) {
    auto start = find_code_point_start_reverse<Traits>(input + p - 1);
    uint8_t size;
    uint32_t c = Traits::parse_code_point(start, &size);
    if (is_stable<form>(c)) {
      return start;
    }
    p = size_t(start - input);
  }
  return input + length;
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

// Get the composition list of a starter that combines forward, or null if it
// has none
template <ComposedForm form>
simdutf_really_inline const uint64_t *forward_compositions(uint16_t value) {
  if ((value & 0x8000) == 0) {
    return nullptr;
  }
  uint16_t offset = (value >> 2) & 0x3FF;
  if constexpr (form == ComposedForm::NFC) {
    return &simdutf::tables::normalization::compositions[offset];
  } else {
    return &simdutf::tables::normalization::compositions[offset];
  }
}

// Search a starter's composition list for `trail`, returning the composed code
// point or zero if the pair does not compose. `*combines_forward` is set when
// the composite may itself compose with a following character.
//
// Entries for one starter are contiguous and sorted by ascending trailing code
// point, and the run is terminated by a flag bit, so the scan can stop as soon
// as it overshoots.
uint32_t combine(const uint64_t *list, uint32_t trail, bool *combines_forward) {
  for (;;) {
    uint64_t entry = *list;
    uint32_t entry_trail = uint32_t(entry & 0x1FFFFF);
    if (entry_trail == trail) {
      *combines_forward = ((entry >> 42) & 1) != 0;
      return uint32_t((entry >> 21) & 0x1FFFFF);
    }
    if (entry_trail > trail || ((entry >> 43) & 1) != 0) {
      return 0;
    }
    list++;
  }
}

// Canonically compose `buf` in place. The buffer must already be in NF(K)D.
template <typename Traits, ComposedForm form>
size_t recompose(typename Traits::char_type *buf, size_t length) {
  using char_type = typename Traits::char_type;

  char_type *p = buf;
  // Non-null once we have seen a starter that may compose forward.
  char_type *starter = nullptr;
  uint8_t starter_size = 0;
  // Null when that starter has no composition list. A Hangul L jamo composes
  // forward arithmetically, so it sets `starter` but leaves this null.
  const uint64_t *compositions = nullptr;
  uint8_t prev_ccc = 0;

  while (p < buf + length) {
    uint8_t size;
    uint32_t c = Traits::parse_code_point(p, &size);
    char_type *c_start = p;
    p += size;

    uint16_t value = lookup_comp_trie<form>(c);
    uint8_t ccc = (value & 0x8000) > 0 ? 0 : uint8_t(value >> 2);

    // Compose if:
    // 1. The starter composes forward
    // 2. The code point composes backwar
    // 3. The code point is not blocked
    if (starter != nullptr && (value & 0b11) == 3 &&
        (prev_ccc < ccc || prev_ccc == 0)) {
      uint32_t composite = 0;
      bool combines_forward = false;

      if (c >= v_base && c < v_base + v_count) {
        // Hangul composes arithmetically and has no table entry. In NF(K)D a
        // syllable appears as L + V (+ T), so a Jamo V composes with the
        // preceding Jamo L and absorbs a following Jamo T in the same step.
        // That leaves no LV syllable for a lone Jamo T to combine with, which
        // is why there is no Jamo T case here.
        uint8_t l_size;
        uint32_t l = Traits::parse_code_point(starter, &l_size) - l_base;
        if (l < l_count) {
          composite = s_base + (l * v_count + (c - v_base)) * t_count;
          if (p < buf + length) {
            uint8_t t_size;
            uint32_t t = Traits::parse_code_point(p, &t_size);
            // Note that this checks `t > t_base`, not `t >= t_base`, for a
            // good reason: the first valid T jamo is `t_base + 1`. The spec
            // defines the T base constant to be off by one in order to make
            // the math for algorithmic decomposition cleaner. See:
            // https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G59434
            if (t > t_base && t < t_base + t_count) {
              composite += t - t_base;
              p += t_size;
            }
          }
        }
      } else if (compositions != nullptr) {
        composite = combine(compositions, c, &combines_forward);
      }

      if (composite != 0) {
        // Delete everything consumed after the starter.
        size_t consumed = size_t(p - c_start);
        shift_left(c_start, size_t((buf + length) - c_start), consumed);
        length -= consumed;

        // Resize the starter in place so it can hold the composite. Note that
        // this can never overflow the buffer: the composite is at most one
        // code unit longer than the starter, and we just removed a code point
        // of at least that size.
        size_t composed_size = Traits::code_point_size(composite);
        size_t tail_length = size_t((buf + length) - (starter + starter_size));
        if (composed_size > starter_size) {
          size_t grow = composed_size - starter_size;
          shift_right(starter + starter_size, tail_length, grow);
        } else if (composed_size < starter_size) {
          size_t shrink = starter_size - composed_size;
          shift_left(starter + composed_size, tail_length + shrink, shrink);
        }
        (void)Traits::write_code_point(composite, starter);
        length = length - starter_size + composed_size;

        // Resume after the starter's following text, which the resize moved.
        p = c_start + (ptrdiff_t(composed_size) - ptrdiff_t(starter_size));
        starter_size = uint8_t(composed_size);
        if (combines_forward) {
          compositions =
              forward_compositions<form>(lookup_comp_trie<form>(composite));
        } else {
          starter = nullptr;
          compositions = nullptr;
        }
        continue;
      }
    }

    prev_ccc = ccc;
    if (ccc == 0) {
      // A new starter. Check if it can combine forward
      if ((value & 0x8000) > 0) {
        compositions = forward_compositions<form>(value);
        starter = c_start;
        starter_size = size;
      } else if (c >= l_base && c < l_base + l_count) {
        compositions = nullptr;
        starter = c_start;
        starter_size = size;
      } else {
        starter = nullptr;
        compositions = nullptr;
      }
    }
  }

  return length;
}

} // namespace normalization
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_NORMALIZATION_H
