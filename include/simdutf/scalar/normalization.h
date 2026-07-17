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

} // namespace normalization
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_NORMALIZATION_H
