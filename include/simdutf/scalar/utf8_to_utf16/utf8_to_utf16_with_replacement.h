#ifndef SIMDUTF_UTF8_TO_UTF16_WITH_REPLACEMENT_H
#define SIMDUTF_UTF8_TO_UTF16_WITH_REPLACEMENT_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_utf16 {

// Pointer-like view so validate_with_counts can run during constant
// evaluation, where reinterpret_cast from char* to uint8_t* is not allowed.
// operator+ exists only for the runtime ASCII probe inside that function.
template <typename InputPtr> struct utf8_byte_pointer {
  InputPtr p;
  simdutf_constexpr23 uint8_t operator*() const noexcept {
    return uint8_t(static_cast<unsigned char>(*p));
  }
  simdutf_constexpr23 uint8_t operator[](size_t i) const noexcept {
    return uint8_t(static_cast<unsigned char>(p[i]));
  }
  const uint8_t *operator+(size_t i) const noexcept {
    return reinterpret_cast<const uint8_t *>(static_cast<const void *>(p + i));
  }
};

// Byte length of one maximal subpart starting at ptr. n must be at least 1.
// The validator reports the first byte of the ill-formed sequence.
template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t maximal_subpart(InputPtr ptr, size_t n) noexcept {
  const uint8_t b = uint8_t(static_cast<unsigned char>(ptr[0]));
  size_t need;
  uint8_t lo = 0x80;
  uint8_t hi = 0xBF;
  if (b < 0xC2) {
    return 1; // ASCII is handled by the caller; continuation, C0, C1
  } else if (b < 0xE0) {
    need = 1;
  } else if (b < 0xF0) {
    need = 2;
    if (b == 0xE0) {
      lo = 0xA0;
    } else if (b == 0xED) {
      hi = 0x9F;
    }
  } else if (b < 0xF5) {
    need = 3;
    if (b == 0xF0) {
      lo = 0x90;
    } else if (b == 0xF4) {
      hi = 0x8F;
    }
  } else {
    return 1; // F5..FF
  }
  if (n < 2 || uint8_t(static_cast<unsigned char>(ptr[1])) < lo ||
      uint8_t(static_cast<unsigned char>(ptr[1])) > hi) {
    return 1;
  }
  size_t i = 2;
  while (i <= need && i < n &&
         (uint8_t(static_cast<unsigned char>(ptr[i])) & 0xC0) == 0x80) {
    i++;
  }
  return i;
}

// WHATWG / Unicode substitution of maximal subparts. One U+FFFD per subpart.
// `write == false` counts UTF-16 code units and does not store.
template <endianness endian, bool write, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t transcode_with_replacement(
    InputPtr data, size_t len, char16_t *utf16_output) noexcept {
  size_t i = 0;
  size_t written = 0;
  uint32_t cp = 0;
  size_t needed = 0;
  size_t seen = 0;
  uint8_t lo = 0x80;
  uint8_t hi = 0xBF;
  auto store = [&](uint32_t c) {
    if (c >= 0x10000) {
      c -= 0x10000;
      // The mask is a no-op (c < 0x100000). It stops clang 18's SLP
      // vectorizer on RISC-V V from narrowing c to 16 bits before
      // the shift, which drops bits 16-19 of the code point.
      const uint16_t high = uint16_t(0xD800 + ((c >> 10) & 0x3FF));
      const uint16_t low = uint16_t(0xDC00 + (c & 0x3FF));
      if constexpr (write) {
        utf16_output[written] =
            char16_t(scalar::utf16::swap_if_needed<endian>(high));
        utf16_output[written + 1] =
            char16_t(scalar::utf16::swap_if_needed<endian>(low));
      }
      written += 2;
    } else {
      if constexpr (write) {
        utf16_output[written] =
            char16_t(scalar::utf16::swap_if_needed<endian>(uint16_t(c)));
      }
      written += 1;
    }
  };
  while (i < len) {
    if (needed == 0 && i + 16 <= len) {
#if SIMDUTF_CPLUSPLUS23
      if !consteval
#endif
      {
        uint64_t v1;
        std::memcpy(&v1, data + i, sizeof(v1));
        uint64_t v2;
        std::memcpy(&v2, data + i + sizeof(v1), sizeof(v2));
        if (((v1 | v2) & 0x8080808080808080ULL) == 0) {
          for (size_t k = 0; k < 16; k++) {
            store(uint8_t(static_cast<unsigned char>(data[i + k])));
          }
          i += 16;
          continue;
        }
      }
    }
    const uint8_t b = uint8_t(static_cast<unsigned char>(data[i]));
    if (needed == 0) {
      i++;
      lo = 0x80;
      hi = 0xBF;
      if (b < 0x80) {
        store(b);
      } else if (b <= 0xDF) {
        if (b < 0xC2) {
          store(0xFFFD);
        } else {
          needed = 1;
          cp = b & 0x1F;
        }
      } else if (b <= 0xEF) {
        if (b == 0xE0) {
          lo = 0xA0;
        } else if (b == 0xED) {
          hi = 0x9F;
        }
        needed = 2;
        cp = b & 0x0F;
      } else if (b <= 0xF4) {
        if (b == 0xF0) {
          lo = 0x90;
        } else if (b == 0xF4) {
          hi = 0x8F;
        }
        needed = 3;
        cp = b & 0x07;
      } else {
        store(0xFFFD);
      }
      continue;
    }
    if (b < lo || b > hi) {
      needed = 0;
      seen = 0;
      cp = 0;
      lo = 0x80;
      hi = 0xBF;
      store(0xFFFD);
      continue;
    }
    i++;
    lo = 0x80;
    hi = 0xBF;
    cp = (cp << 6) | uint32_t(b & 0x3F);
    seen++;
    if (seen == needed) {
      store(cp);
      needed = 0;
      seen = 0;
      cp = 0;
    }
  }
  if (needed != 0) {
    store(0xFFFD);
  }
  return written;
}

template <endianness endian, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t convert_with_replacement(
    InputPtr data, size_t len, char16_t *utf16_output) noexcept {
  return transcode_with_replacement<endian, true>(data, len, utf16_output);
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t count_with_replacement(InputPtr data,
                                                  size_t len) noexcept {
  return transcode_with_replacement<endianness::LITTLE, false>(data, len,
                                                               nullptr);
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 utf8_to_utf16_result
utf16_length_from_utf8_with_replacement(InputPtr data, size_t len) noexcept {
  utf8_to_utf16_result out;
  size_t pos = 0;
  while (pos < len) {
    utf8_result validation;
#if SIMDUTF_CPLUSPLUS23
    if consteval {
      validation = scalar::utf8::validate_with_counts(
          utf8_byte_pointer<InputPtr>{data + pos}, len - pos);
    } else
#endif
    {
      validation = scalar::utf8::validate_with_counts(
          reinterpret_cast<const uint8_t *>(data + pos), len - pos);
    }
    if (validation.error == error_code::SUCCESS) {
      out.count += validation.utf16_length();
      return out;
    }
    if (validation.input_count >= len - pos) {
      out.count += count_with_replacement(data + pos, len - pos);
      out.more_errors = true;
      if (out.error == error_code::SUCCESS) {
        out.error = validation.error;
      }
      return out;
    }
    out.count += validation.utf16_length() + 1;
    if (out.error == error_code::SUCCESS) {
      out.error = validation.error;
    }
    const size_t err = pos + validation.input_count;
    const size_t skip = maximal_subpart(data + err, len - err);
    if (out.error_count < utf8_to_utf16_result::max_errors) {
      out.error_offset[out.error_count] = err;
      out.error_count += 1;
    } else {
      out.more_errors = true;
    }
    pos = err + skip;
  }
  return out;
}

} // namespace utf8_to_utf16
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif // SIMDUTF_UTF8_TO_UTF16_WITH_REPLACEMENT_H
