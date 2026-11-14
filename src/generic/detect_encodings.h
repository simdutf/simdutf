namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace encoding_detection {

struct checkers {
  utf8_checker utf8{};
  utf16le_checker utf16{};
  utf32le_checker utf32{};

  template <bool check_utf8, bool check_utf16, bool check_utf32>
  simdutf_really_inline void check_block(const simd::simd8x64<uint8_t> &in) {
    if constexpr (check_utf8) {
      const bool ascii = utf8.check_next_input(in);
      if constexpr (check_utf16) {
        if (ascii) {
          utf16.check_ascii();
        } else {
          utf16.check(in);
        }
      }
    } else if constexpr (check_utf16) {
      utf16.check(in);
    }
    if constexpr (check_utf32) {
      utf32.check(in);
    }
  }

  // Returns the subset of 'active' whose checks have not failed so far.
  simdutf_really_inline int still_valid(int active) const {
    if ((active & encoding_type::UTF8) && utf8.errors()) {
      active &= ~encoding_type::UTF8;
    }
    if ((active & encoding_type::UTF16_LE) && utf16.errors()) {
      active &= ~encoding_type::UTF16_LE;
    }
    if ((active & encoding_type::UTF32_LE) && utf32.errors()) {
      active &= ~encoding_type::UTF32_LE;
    }
    return active;
  }
};

// Copies 1 to 63 bytes with overlapping fixed-size copies, which compilers
// inline, instead of a call to memcpy.
simdutf_really_inline void copy_short(uint8_t *dst, const uint8_t *src,
                                      size_t n) {
  if (n >= 32) {
    std::memcpy(dst, src, 32);
    std::memcpy(dst + n - 32, src + n - 32, 32);
  } else if (n >= 16) {
    std::memcpy(dst, src, 16);
    std::memcpy(dst + n - 16, src + n - 16, 16);
  } else if (n >= 8) {
    std::memcpy(dst, src, 8);
    std::memcpy(dst + n - 8, src + n - 8, 8);
  } else if (n >= 4) {
    std::memcpy(dst, src, 4);
    std::memcpy(dst + n - 4, src + n - 4, 4);
  } else {
    for (size_t i = 0; i < n; i++) {
      dst[i] = src[i];
    }
  }
}

// How often check_blocks looks for encodings that have failed.
constexpr size_t stride = 16 * 64;

// Checks the full 64-byte blocks starting before 'end', from 'idx' on, for the
// encodings in 'active' (the template parameters may include more). Every
// 1024 bytes and at the end, it removes from 'active' the encodings that have
// failed, and returns if there are any, so that the caller stops checking
// them. Returns the index of the first block that was not checked.
template <bool check_utf8, bool check_utf16, bool check_utf32>
size_t check_blocks(checkers &c, const uint8_t *input, size_t idx, size_t end,
                    int &active) {
  while (true) {
    const size_t stop = (end - idx > stride) ? idx + stride : end;
    for (; idx < stop; idx += 64) {
      const simd::simd8x64<uint8_t> in(input + idx);
      c.check_block<check_utf8, check_utf16, check_utf32>(in);
    }
    const int valid = c.still_valid(active);
    if (idx >= end || valid != active) {
      active = valid;
      return idx;
    }
  }
}

int detect_encodings(const char *input, size_t length) {
  if (length % 2 == 1 && length < 1024) {
    // Only UTF-8 is possible, and the input is too short for stopping early
    // to pay off.
    return utf8_validation::generic_validate_utf8(input, length)
               ? encoding_type::UTF8
               : 0;
  }
  // We check UTF-8, UTF-16LE and UTF-32LE in a single pass over the input.
  // An encoding that has failed is no longer checked, and we stop as soon as
  // all of them have failed.
  int active = encoding_type::UTF8;
  if (length % 2 == 0) {
    active |= encoding_type::UTF16_LE;
  }
  if (length % 4 == 0) {
    active |= encoding_type::UTF32_LE;
  }
  constexpr int utf8 = encoding_type::UTF8;
  constexpr int utf16 = encoding_type::UTF16_LE;
  constexpr int utf32 = encoding_type::UTF32_LE;

  checkers c{};
  const uint8_t *buf = reinterpret_cast<const uint8_t *>(input);
  // Like buf_block_reader, we leave 1 to 64 bytes for the final block, unless
  // the input is empty.
  const size_t end = length < 64 ? 0 : length - 64;
  size_t idx = 0;
  while (idx < end) {
    switch (active) {
    case utf8 | utf16 | utf32:
    // Valid UTF-8 that is also valid UTF-32LE is always valid UTF-16LE: a
    // high byte 0xD8..0xDF in a UTF-16 code unit would be a UTF-8 lead byte
    // followed by a continuation byte, and the UTF-32 value would then exceed
    // 0x10FFFF. So we do not specialize for UTF-8 and UTF-32 without UTF-16.
    case utf8 | utf32:
      idx = check_blocks<true, true, true>(c, buf, idx, end, active);
      break;
    case utf8 | utf16:
      idx = check_blocks<true, true, false>(c, buf, idx, end, active);
      break;
    case utf8:
      if (idx != 0) {
        // Only UTF-8 is left, without an error so far, and validate_utf8 is
        // faster than our loop. The bytes before idx are valid UTF-8 except
        // that they may end with an incomplete character (or a byte that
        // cannot start one): validate_utf8 starts with that character.
        const size_t start = scalar::utf8::trim_partial_utf8(input, idx);
        return utf8_validation::generic_validate_utf8(input + start,
                                                      length - start)
                   ? utf8
                   : 0;
      }
      // Odd length: we check the first 1024 bytes here, so that we stop early
      // if the input is not UTF-8.
      idx = check_blocks<true, false, false>(c, buf, idx,
                                             detail::min(end, stride), active);
      break;
    case utf16 | utf32:
      idx = check_blocks<false, true, true>(c, buf, idx, end, active);
      break;
    case utf16:
      idx = check_blocks<false, true, false>(c, buf, idx, end, active);
      break;
    case utf32:
      idx = check_blocks<false, false, true>(c, buf, idx, end, active);
      break;
    default:
      // All three encodings have failed.
      return 0;
    }
  }

  // The last 1 to 64 bytes.
  if (length < 64) {
    // Short input: copy it to a block padded with zeros. A zero byte is valid
    // in all three encodings, and a high surrogate followed by the padding is
    // caught.
    if (length != 0) {
      uint8_t block[64]{};
      copy_short(block, buf, length);
      const simd::simd8x64<uint8_t> in(block);
      c.check_block<true, true, true>(in);
    }
    c.utf8.check_eof();
  } else {
    // The last 64 bytes. They start at an even index if UTF-16LE is still
    // active, and at a multiple of four if UTF-32LE is.
    const size_t start = length - 64;
    const simd::simd8x64<uint8_t> in(buf + start);
    if (idx == start) {
      // They form a full block that has not been checked.
      if (active & utf8) {
        c.utf8.check_next_input(in);
        c.utf8.check_eof();
      }
      if (active & utf16) {
        c.utf16.check(in);
      }
    } else {
      // They overlap the bytes already checked.
      if (active & utf8) {
        // Same as validate_utf8, it also calls check_eof().
        utf8_validation::check_remainder(c.utf8, buf, length, idx);
      }
      if (active & utf16) {
        c.utf16.check<true>(in);
      }
    }
    if (active & utf32) {
      // Checking values again does not change the result.
      c.utf32.check(in);
    }
  }

  int out = 0;
  if ((active & utf8) && !c.utf8.errors()) {
    out |= utf8;
  }
  if ((active & utf16) && !c.utf16.errors_at_eof()) {
    out |= utf16;
  }
  if ((active & utf32) && !c.utf32.errors()) {
    out |= utf32;
  }
  return out;
}

} // namespace encoding_detection
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
