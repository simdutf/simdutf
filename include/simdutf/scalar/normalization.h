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

} // namespace normalization
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_NORMALIZATION_H
