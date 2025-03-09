#pragma once

#include "simdutf/encoding_types.h"

template <typename = void>
char16_t to_utf16(simdutf::endianness byte_order, char16_t chr) {
  if (!match_system(byte_order)) {
    return char16_t((uint16_t(chr) << 8) | (uint16_t(chr) >> 8));
  } else {
    return chr;
  }
}

template <typename = void> char16_t to_utf16be(char16_t chr) {
  return to_utf16(simdutf::endianness::BIG, chr);
}

template <typename = void> char16_t to_utf16le(char16_t chr) {
  return to_utf16(simdutf::endianness::LITTLE, chr);
}

template <typename = void>
void to_utf16le_inplace(char16_t *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    data[i] = to_utf16le(data[i]);
  }
}
