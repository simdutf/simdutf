// Icelake (AVX-512) UTF-8 normalization: per-window masked kernels.
//
// These functions plug into the portable driver in
// generic/utf8_to_{decomposed,composed}/. The driver provides the vectorized
// 63-byte all-ASCII fast path; each masked kernel additionally skips a leading
// run of ASCII bytes eagerly (the dominant case for Latin-script text, where
// decomposable code points are sparse). Non-ASCII code points are decoded and
// (de)composed with the shared, already-validated scalar routines.
//
// NOTE(perf): the non-ASCII inner work is currently scalar. The vectorized
// decomposition writers (mirroring the arm64 kernel, using vpermb) are a
// follow-up; the plumbing and the ASCII fast paths land first.

namespace internal {
// Copy 64 bytes. The caller must guarantee 64 bytes of headroom in both
// operands (the driver's SAFETY_MARGIN and the 128-byte input lookahead do).
simdutf_really_inline void icelake_copy64(uint8_t *dst, const uint8_t *src) {
  _mm512_storeu_si512(reinterpret_cast<void *>(dst),
                      _mm512_loadu_si512(reinterpret_cast<const void *>(src)));
}
} // namespace internal

// Decompose (NF(K)D) up to a 12-byte window of UTF-8. Returns bytes consumed.
template <DecomposedForm form>
size_t normalize_masked_utf8_to_decomposed(const uint8_t *input, uint64_t mask,
                                           uint8_t **out, size_t out_length,
                                           uint8_t *last_ccc) {
  // Eagerly skip a leading ASCII run. `~mask` trailing zeros == number of
  // leading ASCII bytes. Matches the arm64 kernel's threshold.
  int t1 = int(_tzcnt_u64(~mask));
  if (t1 > 2) {
    size_t min = t1 > 52 ? 52 : size_t(t1);
    internal::icelake_copy64(*out, input);
    *out += min;
    *last_ccc = 0;
    return min;
  }

  const char *data = reinterpret_cast<const char *>(input);
  char *output = reinterpret_cast<char *>(*out);
  char *ostart = output;
  size_t pos = 0;
  do {
    uint8_t first_ccc = 0;
    uint8_t ccc = 0;
    uint8_t leading = uint8_t(data[pos]);
    if (leading < 0x80) {
      *output++ = char(leading);
      pos++;
    } else if ((leading & 0xE0) == 0xC0) {
      uint32_t code_point =
          (uint32_t(leading & 0x1F) << 6) | (uint8_t(data[pos + 1]) & 0x3F);
      size_t nwritten = scalar::utf8_to_decomposed::decompose_bmp<form>(
          uint16_t(code_point), output, &first_ccc, &ccc);
      if (nwritten == 0) {
        *output++ = char(leading);
        *output++ = data[pos + 1];
      } else {
        output += nwritten;
      }
      pos += 2;
    } else if ((leading & 0xF0) == 0xE0) {
      uint32_t code_point = (uint32_t(leading & 0x0F) << 12) |
                            (uint32_t(uint8_t(data[pos + 1]) & 0x3F) << 6) |
                            (uint8_t(data[pos + 2]) & 0x3F);
      // Precomposed Hangul syllables only occur in 3-byte UTF-8.
      if (scalar::normalization::is_hangul_syllable(code_point)) {
        output +=
            scalar::utf8_to_decomposed::decompose_hangul(code_point, output);
      } else {
        size_t nwritten = scalar::utf8_to_decomposed::decompose_bmp<form>(
            uint16_t(code_point), output, &first_ccc, &ccc);
        if (nwritten == 0) {
          *output++ = char(leading);
          *output++ = data[pos + 1];
          *output++ = data[pos + 2];
        } else {
          output += nwritten;
        }
      }
      pos += 3;
    } else {
      uint32_t code_point = (uint32_t(leading & 0x07) << 18) |
                            (uint32_t(uint8_t(data[pos + 1]) & 0x3F) << 12) |
                            (uint32_t(uint8_t(data[pos + 2]) & 0x3F) << 6) |
                            (uint8_t(data[pos + 3]) & 0x3F);
      size_t nwritten =
          scalar::utf8_to_decomposed::decompose_supplementary<form>(
              code_point, output, &first_ccc, &ccc);
      if (nwritten == 0) {
        *output++ = char(leading);
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
      ccc = scalar::normalization::sort_combining<
          scalar::normalization::utf8_normalization_traits>(
          output, (output - ostart) + out_length);
    }
    *last_ccc = ccc;
  } while (pos < 12);

  *out = reinterpret_cast<uint8_t *>(output);
  return pos;
}

// NF(K)D quick check + output-length estimate for a 12-byte window.
template <DecomposedForm form>
size_t normalize_masked_utf8_to_decomposed_check(const uint8_t *input,
                                                 uint64_t mask,
                                                 size_t *out_length, bool *is_qc,
                                                 uint8_t *last_ccc) {
  int t1 = int(_tzcnt_u64(~mask));
  if (t1 > 0) {
    size_t min = t1 > 52 ? 52 : size_t(t1);
    *out_length += min;
    *last_ccc = 0;
    return min;
  }

  const char *data = reinterpret_cast<const char *>(input);
  size_t pos = 0;
  do {
    uint8_t ccc = 0;
    uint8_t leading = uint8_t(data[pos]);
    if (leading < 0x80) {
      (*out_length)++;
      pos++;
    } else if ((leading & 0xE0) == 0xC0) {
      uint32_t code_point =
          (uint32_t(leading & 0x1F) << 6) | (uint8_t(data[pos + 1]) & 0x3F);
      *is_qc &= scalar::utf8_to_decomposed::check_code_point_bmp<form>(
          uint16_t(code_point), out_length, &ccc);
      pos += 2;
    } else if ((leading & 0xF0) == 0xE0) {
      uint32_t code_point = (uint32_t(leading & 0x0F) << 12) |
                            (uint32_t(uint8_t(data[pos + 1]) & 0x3F) << 6) |
                            (uint8_t(data[pos + 2]) & 0x3F);
      *is_qc &= scalar::utf8_to_decomposed::check_code_point_bmp<form>(
          uint16_t(code_point), out_length, &ccc);
      pos += 3;
    } else {
      uint32_t code_point = (uint32_t(leading & 0x07) << 18) |
                            (uint32_t(uint8_t(data[pos + 1]) & 0x3F) << 12) |
                            (uint32_t(uint8_t(data[pos + 2]) & 0x3F) << 6) |
                            (uint8_t(data[pos + 3]) & 0x3F);
      *is_qc &= scalar::utf8_to_decomposed::check_code_point_supplementary<form>(
          code_point, out_length, &ccc);
      pos += 4;
    }
    if (*last_ccc > ccc && ccc != 0) {
      *is_qc = false;
    }
    *last_ccc = ccc;
  } while (pos < 12);

  return pos;
}

// Compose (NF(K)C) up to a 12-byte window of UTF-8. Returns bytes consumed.
// Stable (composition-irrelevant, canonically-ordered) code points are copied
// verbatim; on the first relevant/combining code point we delegate the enclosing
// canonical region to the shared scalar composer, which expands to the true
// stable-starter boundaries and returns how many input bytes it consumed.
template <ComposedForm form>
size_t normalize_masked_utf8_to_composed(const uint8_t *input,
                                         const uint8_t *input_base,
                                         size_t input_length, uint64_t mask,
                                         uint8_t **out, size_t out_length,
                                         uint8_t *last_ccc) {
  (void)out_length;
  int t1 = int(_tzcnt_u64(~mask));
  if (t1 > 2) {
    size_t min = t1 > 52 ? 52 : size_t(t1);
    internal::icelake_copy64(*out, input);
    *out += min;
    *last_ccc = 0;
    return min;
  }

  const char *data = reinterpret_cast<const char *>(input);
  char *output = reinterpret_cast<char *>(*out);
  size_t pos = 0;
  do {
    uint8_t leading = uint8_t(data[pos]);
    if (leading < 0x80) {
      *output++ = char(leading);
      pos++;
      *last_ccc = 0;
      continue;
    }
    // leading >= 0x80 here; valid UTF-8 leading byte encodes its length.
    uint8_t size = leading < 0xE0 ? 2 : (leading < 0xF0 ? 3 : 4);
    uint32_t c;
    uint8_t ccc;
    bool is_relevant;
    if (size == 2) {
      c = (uint32_t(leading & 0x1F) << 6) | (uint8_t(data[pos + 1]) & 0x3F);
    } else if (size == 3) {
      c = (uint32_t(leading & 0x0F) << 12) |
          (uint32_t(uint8_t(data[pos + 1]) & 0x3F) << 6) |
          (uint8_t(data[pos + 2]) & 0x3F);
    } else {
      c = (uint32_t(leading & 0x07) << 18) |
          (uint32_t(uint8_t(data[pos + 1]) & 0x3F) << 12) |
          (uint32_t(uint8_t(data[pos + 2]) & 0x3F) << 6) |
          (uint8_t(data[pos + 3]) & 0x3F);
    }
    if (c <= 0xFFFF) {
      uint16_t value = scalar::normalization::lookup_comp_trie<form>(uint16_t(c));
      ccc = uint8_t(value >> 2);
      is_relevant = (value & 0b11) > 0;
    } else {
      constexpr auto dform = to_decomposed_form(form);
      uint64_t kv =
          scalar::normalization::lookup_supplementary_code_point<dform>(c);
      uint32_t k = kv & 0x1FFFFF;
      is_relevant = false;
      ccc = 0;
      if (k == c) {
        uint8_t qc = uint8_t(kv >> 56);
        is_relevant = qc != 0;
        ccc = (kv >> 45) & 0xFF;
      }
    }

    if (ccc <= *last_ccc && !is_relevant) {
      for (uint8_t i = 0; i < size; i++) {
        *output++ = data[pos + i];
      }
      pos += size;
      *last_ccc = ccc;
      continue;
    }

    // Relevant/combining code point: hand the enclosing canonical region to the
    // scalar composer. It reaches back to the previous stable starter and
    // forward to the next one, so `output` must be positioned at the current
    // input offset (it is), and it returns bytes consumed from `data + pos`.
    *out = reinterpret_cast<uint8_t *>(output);
    size_t consumed = scalar::utf8_to_composed::normalize_with_context<form>(
        data + pos, reinterpret_cast<const char *>(input_base), input_length,
        reinterpret_cast<char **>(out), size);
    *last_ccc = 0;
    return pos + consumed;
  } while (pos < 12);

  *out = reinterpret_cast<uint8_t *>(output);
  return pos;
}

// NF(K)C quick check + output-length estimate for a 12-byte window.
template <ComposedForm form>
size_t normalize_masked_utf8_to_composed_check(const uint8_t *input,
                                               uint64_t mask, size_t *out_length,
                                               bool *is_qc, uint8_t *last_ccc) {
  int t1 = int(_tzcnt_u64(~mask));
  if (t1 > 0) {
    size_t min = t1 > 52 ? 52 : size_t(t1);
    *out_length += min;
    *last_ccc = 0;
    return min;
  }

  const char *data = reinterpret_cast<const char *>(input);
  size_t pos = 0;
  do {
    uint8_t ccc;
    uint8_t leading = uint8_t(data[pos]);
    if (leading < 0x80) {
      (*out_length)++;
      pos++;
      ccc = 0;
    } else if ((leading & 0xE0) == 0xC0) {
      uint32_t code_point =
          (uint32_t(leading & 0x1F) << 6) | (uint8_t(data[pos + 1]) & 0x3F);
      *is_qc &= scalar::utf8_to_composed::check_code_point_bmp<form>(
          uint16_t(code_point), out_length, &ccc);
      pos += 2;
    } else if ((leading & 0xF0) == 0xE0) {
      uint32_t code_point = (uint32_t(leading & 0x0F) << 12) |
                            (uint32_t(uint8_t(data[pos + 1]) & 0x3F) << 6) |
                            (uint8_t(data[pos + 2]) & 0x3F);
      *is_qc &= scalar::utf8_to_composed::check_code_point_bmp<form>(
          uint16_t(code_point), out_length, &ccc);
      pos += 3;
    } else {
      uint32_t code_point = (uint32_t(leading & 0x07) << 18) |
                            (uint32_t(uint8_t(data[pos + 1]) & 0x3F) << 12) |
                            (uint32_t(uint8_t(data[pos + 2]) & 0x3F) << 6) |
                            (uint8_t(data[pos + 3]) & 0x3F);
      *is_qc &= scalar::utf8_to_composed::check_code_point_supplementary<form>(
          code_point, out_length, &ccc);
      pos += 4;
    }
    if (*last_ccc > ccc && ccc != 0) {
      *is_qc = false;
    }
    *last_ccc = ccc;
  } while (pos < 12);

  return pos;
}

namespace internal {
// End-of-code-point mask for a 64-byte block: bit i set means byte i is the
// last byte of a code point (matches the generic driver's `mask`). Continuation
// bytes are 0x80..0xBF, i.e. signed int8 < -64.
simdutf_really_inline uint64_t icelake_codepoint_mask(const uint8_t *in) {
  __m512i v = _mm512_loadu_si512(reinterpret_cast<const void *>(in));
  __mmask64 cont = _mm512_cmplt_epi8_mask(v, _mm512_set1_epi8(-64));
  uint64_t leading = ~static_cast<uint64_t>(cont);
  return leading >> 1;
}
} // namespace internal

// Icelake outer driver for NF(K)D. Structure mirrors the portable driver in
// generic/utf8_to_decomposed/, but uses native AVX-512 (icelake has no
// simd8x64 abstraction).
template <DecomposedForm form>
size_t icelake_normalize_utf8_to_decomposed(const char *in, size_t length,
                                            char *out) {
  char *start{out};
  char **out_ptr = &out;
  constexpr const size_t SAFETY_MARGIN = 64;
  uint8_t last_ccc = 0;
  size_t p = 0;
  while (p + 64 + SAFETY_MARGIN <= length) {
    uint64_t mask = internal::icelake_codepoint_mask(
        reinterpret_cast<const uint8_t *>(in) + p);
    // All-ASCII fast path (63 bytes; bit 63 excluded so a code point straddling
    // the block boundary is handled by the next block).
    if (mask == 0x7FFFFFFFFFFFFFFF) {
      _mm512_storeu_si512(
          reinterpret_cast<void *>(*out_ptr),
          _mm512_loadu_si512(reinterpret_cast<const void *>(in + p)));
      p += 63;
      *out_ptr += 63;
      last_ccc = 0;
      continue;
    }
    size_t pmax = (p + 64) - 12;
    while (p < pmax) {
      size_t consumed = normalize_masked_utf8_to_decomposed<form>(
          reinterpret_cast<const uint8_t *>(in + p), mask,
          reinterpret_cast<uint8_t **>(out_ptr), *out_ptr - start, &last_ccc);
      p += consumed;
      mask >>= consumed;
    }
  }
  if (p < length) {
    *out_ptr += scalar::utf8_to_decomposed::normalize_with_context<form>(
        in + p, length - p, *out_ptr, *out_ptr - start, &last_ccc);
  }
  return *out_ptr - start;
}

template <DecomposedForm form>
bool icelake_normalize_utf8_to_decomposed_check(const char *in, size_t length,
                                                size_t *output_length) {
  *output_length = 0;
  bool is_qc = true;
  constexpr const size_t SAFETY_MARGIN = 64;
  uint8_t last_ccc = 0;
  size_t p = 0;
  while (p + 64 + SAFETY_MARGIN <= length) {
    uint64_t mask = internal::icelake_codepoint_mask(
        reinterpret_cast<const uint8_t *>(in) + p);
    if (mask == 0x7FFFFFFFFFFFFFFF) {
      p += 63;
      *output_length += 63;
      last_ccc = 0;
      continue;
    }
    size_t pmax = (p + 64) - 12;
    while (p < pmax) {
      size_t consumed = normalize_masked_utf8_to_decomposed_check<form>(
          reinterpret_cast<const uint8_t *>(in + p), mask, output_length, &is_qc,
          &last_ccc);
      p += consumed;
      mask >>= consumed;
    }
  }
  if (p < length) {
    is_qc &= scalar::utf8_to_decomposed::check_with_context<form>(
        in + p, length - p, output_length, &last_ccc);
  }
  return is_qc;
}

// Icelake outer driver for NF(K)C.
template <ComposedForm form>
size_t icelake_normalize_utf8_to_composed(const char *in, size_t length,
                                          char *out) {
  char *start{out};
  char **out_ptr = &out;
  constexpr const size_t SAFETY_MARGIN = 64;
  uint8_t last_ccc = 0;
  size_t p = 0;
  while (p + 64 + SAFETY_MARGIN <= length) {
    uint64_t mask = internal::icelake_codepoint_mask(
        reinterpret_cast<const uint8_t *>(in) + p);
    if (mask == 0x7FFFFFFFFFFFFFFF) {
      _mm512_storeu_si512(
          reinterpret_cast<void *>(*out_ptr),
          _mm512_loadu_si512(reinterpret_cast<const void *>(in + p)));
      p += 63;
      *out_ptr += 63;
      last_ccc = 0;
      continue;
    }
    size_t pmax = (p + 64) - 12;
    while (p < pmax) {
      size_t consumed = normalize_masked_utf8_to_composed<form>(
          reinterpret_cast<const uint8_t *>(in + p),
          reinterpret_cast<const uint8_t *>(in), length, mask,
          reinterpret_cast<uint8_t **>(out_ptr), *out_ptr - start, &last_ccc);
      p += consumed;
      mask >>= consumed;
    }
  }
  if (p < length) {
    (void)scalar::utf8_to_composed::normalize_with_context<form>(
        in + p, in, length, out_ptr, length - p);
  }
  return *out_ptr - start;
}

template <ComposedForm form>
bool icelake_normalize_utf8_to_composed_check(const char *in, size_t length,
                                              size_t *output_length) {
  *output_length = 0;
  bool is_qc = true;
  constexpr const size_t SAFETY_MARGIN = 64;
  uint8_t last_ccc = 0;
  size_t p = 0;
  while (p + 64 + SAFETY_MARGIN <= length) {
    uint64_t mask = internal::icelake_codepoint_mask(
        reinterpret_cast<const uint8_t *>(in) + p);
    if (mask == 0x7FFFFFFFFFFFFFFF) {
      p += 63;
      *output_length += 63;
      last_ccc = 0;
      continue;
    }
    size_t pmax = (p + 64) - 12;
    while (p < pmax) {
      size_t consumed = normalize_masked_utf8_to_composed_check<form>(
          reinterpret_cast<const uint8_t *>(in + p), mask, output_length, &is_qc,
          &last_ccc);
      p += consumed;
      mask >>= consumed;
    }
  }
  if (p < length) {
    is_qc &= scalar::utf8_to_composed::check_with_context<form>(
        in + p, length - p, output_length, &last_ccc);
  }
  return is_qc;
}
