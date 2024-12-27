// file included directly
/**
 * References and further reading:
 *
 * Wojciech Muła, Daniel Lemire, Base64 encoding and decoding at almost the
 * speed of a memory copy, Software: Practice and Experience 50 (2), 2020.
 * https://arxiv.org/abs/1910.05109
 *
 * Wojciech Muła, Daniel Lemire, Faster Base64 Encoding and Decoding using AVX2
 * Instructions, ACM Transactions on the Web 12 (3), 2018.
 * https://arxiv.org/abs/1704.00605
 *
 * Simon Josefsson. 2006. The Base16, Base32, and Base64 Data Encodings.
 * https://tools.ietf.org/html/rfc4648. (2006). Internet Engineering Task Force,
 * Request for Comments: 4648.
 *
 * Alfred Klomp. 2014a. Fast Base64 encoding/decoding with SSE vectorization.
 * http://www.alfredklomp.com/programming/sse-base64/. (2014).
 *
 * Alfred Klomp. 2014b. Fast Base64 stream encoder/decoder in C99, with SIMD
 * acceleration. https://github.com/aklomp/base64. (2014).
 *
 * Hanson Char. 2014. A Fast and Correct Base 64 Codec. (2014).
 * https://aws.amazon.com/blogs/developer/a-fast-and-correct-base-64-codec/
 *
 * Nick Kopp. 2013. Base64 Encoding on a GPU.
 * https://www.codeproject.com/Articles/276993/Base-Encoding-on-a-GPU. (2013).
 */

struct block64 {
  __m512i chunks[1];
};

template <bool base64_url>
size_t encode_base64(char *dst, const char *src, size_t srclen,
                     base64_options options) {
  // credit: Wojciech Muła
  const uint8_t *input = (const uint8_t *)src;

  uint8_t *out = (uint8_t *)dst;
  static const char *lookup_tbl =
      base64_url
          ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
          : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  const __m512i shuffle_input = _mm512_setr_epi32(
      0x01020001, 0x04050304, 0x07080607, 0x0a0b090a, 0x0d0e0c0d, 0x10110f10,
      0x13141213, 0x16171516, 0x191a1819, 0x1c1d1b1c, 0x1f201e1f, 0x22232122,
      0x25262425, 0x28292728, 0x2b2c2a2b, 0x2e2f2d2e);
  const __m512i lookup =
      _mm512_loadu_si512(reinterpret_cast<const __m512i *>(lookup_tbl));
  const __m512i multi_shifts = _mm512_set1_epi64(UINT64_C(0x3036242a1016040a));
  size_t size = srclen;
  __mmask64 input_mask = 0xffffffffffff; // (1 << 48) - 1
  while (size >= 48) {
    const __m512i v = _mm512_maskz_loadu_epi8(
        input_mask, reinterpret_cast<const __m512i *>(input));
    const __m512i in = _mm512_permutexvar_epi8(shuffle_input, v);
    const __m512i indices = _mm512_multishift_epi64_epi8(multi_shifts, in);
    const __m512i result = _mm512_permutexvar_epi8(indices, lookup);
    _mm512_storeu_si512(reinterpret_cast<__m512i *>(out), result);
    out += 64;
    input += 48;
    size -= 48;
  }
  input_mask = ((__mmask64)1 << size) - 1;
  const __m512i v = _mm512_maskz_loadu_epi8(
      input_mask, reinterpret_cast<const __m512i *>(input));
  const __m512i in = _mm512_permutexvar_epi8(shuffle_input, v);
  const __m512i indices = _mm512_multishift_epi64_epi8(multi_shifts, in);
  bool padding_needed =
      (((options & base64_url) == 0) ^
       ((options & base64_reverse_padding) == base64_reverse_padding));
  size_t padding_amount = ((size % 3) > 0) ? (3 - (size % 3)) : 0;
  size_t output_len = ((size + 2) / 3) * 4;
  size_t non_padded_output_len = output_len - padding_amount;
  if (!padding_needed) {
    output_len = non_padded_output_len;
  }
  __mmask64 output_mask = output_len == 64 ? (__mmask64)UINT64_MAX
                                           : ((__mmask64)1 << output_len) - 1;
  __m512i result = _mm512_mask_permutexvar_epi8(
      _mm512_set1_epi8('='), ((__mmask64)1 << non_padded_output_len) - 1,
      indices, lookup);
  _mm512_mask_storeu_epi8(reinterpret_cast<__m512i *>(out), output_mask,
                          result);
  return (size_t)(out - (uint8_t *)dst) + output_len;
}

template <bool base64_url, bool ignore_garbage>
static inline uint64_t to_base64_mask(block64 *b, uint64_t *error,
                                      uint64_t input_mask = UINT64_MAX) {
  __m512i input = b->chunks[0];
  const __m512i ascii_space_tbl = _mm512_set_epi8(
      0, 0, 13, 12, 0, 10, 9, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 13, 12, 0, 10,
      9, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 13, 12, 0, 10, 9, 0, 0, 0, 0, 0, 0,
      0, 0, 32, 0, 0, 13, 12, 0, 10, 9, 0, 0, 0, 0, 0, 0, 0, 0, 32);
  __m512i lookup0;
  if (base64_url) {
    lookup0 = _mm512_set_epi8(
        -128, -128, -128, -128, -128, -128, 61, 60, 59, 58, 57, 56, 55, 54, 53,
        52, -128, -128, 62, -128, -128, -128, -128, -128, -128, -128, -128,
        -128, -128, -128, -128, -1, -128, -128, -128, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -1,
        -128, -128, -1, -1, -128, -128, -128, -128, -128, -128, -128, -128, -1);
  } else {
    lookup0 = _mm512_set_epi8(
        -128, -128, -128, -128, -128, -128, 61, 60, 59, 58, 57, 56, 55, 54, 53,
        52, 63, -128, -128, -128, 62, -128, -128, -128, -128, -128, -128, -128,
        -128, -128, -128, -1, -128, -128, -128, -128, -128, -128, -128, -128,
        -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -1, -128,
        -128, -1, -1, -128, -128, -128, -128, -128, -128, -128, -128, -128);
  }
  __m512i lookup1;
  if (base64_url) {
    lookup1 = _mm512_set_epi8(
        -128, -128, -128, -128, -128, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42,
        41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, -128,
        63, -128, -128, -128, -128, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15,
        14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -128);
  } else {
    lookup1 = _mm512_set_epi8(
        -128, -128, -128, -128, -128, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42,
        41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, -128,
        -128, -128, -128, -128, -128, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -128);
  }

  const __m512i translated = _mm512_permutex2var_epi8(lookup0, input, lookup1);
  const __m512i combined = _mm512_or_si512(translated, input);
  const __mmask64 mask = _mm512_movepi8_mask(combined) & input_mask;
  if (!ignore_garbage && mask) {
    const __mmask64 spaces =
        _mm512_cmpeq_epi8_mask(_mm512_shuffle_epi8(ascii_space_tbl, input),
                               input) &
        input_mask;
    *error = (mask ^ spaces);
  }
  b->chunks[0] = translated;

  return mask | (~input_mask);
}

static inline void copy_block(block64 *b, char *output) {
  _mm512_storeu_si512(reinterpret_cast<__m512i *>(output), b->chunks[0]);
}

static inline uint64_t compress_block(block64 *b, uint64_t mask, char *output) {
  uint64_t nmask = ~mask;
  __m512i c = _mm512_maskz_compress_epi8(nmask, b->chunks[0]);
  _mm512_storeu_si512(reinterpret_cast<__m512i *>(output), c);
  return _mm_popcnt_u64(nmask);
}

// The caller of this function is responsible to ensure that there are 64 bytes
// available from reading at src. The data is read into a block64 structure.
static inline void load_block(block64 *b, const char *src) {
  b->chunks[0] = _mm512_loadu_si512(reinterpret_cast<const __m512i *>(src));
}

static inline void load_block_partial(block64 *b, const char *src,
                                      __mmask64 input_mask) {
  b->chunks[0] = _mm512_maskz_loadu_epi8(
      input_mask, reinterpret_cast<const __m512i *>(src));
}

// The caller of this function is responsible to ensure that there are 128 bytes
// available from reading at src. The data is read into a block64 structure.
static inline void load_block(block64 *b, const char16_t *src) {
  __m512i m1 = _mm512_loadu_si512(reinterpret_cast<const __m512i *>(src));
  __m512i m2 = _mm512_loadu_si512(reinterpret_cast<const __m512i *>(src + 32));
  __m512i p = _mm512_packus_epi16(m1, m2);
  b->chunks[0] =
      _mm512_permutexvar_epi64(_mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7), p);
}

static inline void load_block_partial(block64 *b, const char16_t *src,
                                      __mmask64 input_mask) {
  __m512i m1 = _mm512_maskz_loadu_epi16((__mmask32)input_mask,
                                        reinterpret_cast<const __m512i *>(src));
  __m512i m2 =
      _mm512_maskz_loadu_epi16((__mmask32)(input_mask >> 32),
                               reinterpret_cast<const __m512i *>(src + 32));
  __m512i p = _mm512_packus_epi16(m1, m2);
  b->chunks[0] =
      _mm512_permutexvar_epi64(_mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7), p);
}

static inline void base64_decode(char *out, __m512i str) {
  const __m512i merge_ab_and_bc =
      _mm512_maddubs_epi16(str, _mm512_set1_epi32(0x01400140));
  const __m512i merged =
      _mm512_madd_epi16(merge_ab_and_bc, _mm512_set1_epi32(0x00011000));
  const __m512i pack = _mm512_set_epi8(
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 60, 61, 62, 56, 57, 58,
      52, 53, 54, 48, 49, 50, 44, 45, 46, 40, 41, 42, 36, 37, 38, 32, 33, 34,
      28, 29, 30, 24, 25, 26, 20, 21, 22, 16, 17, 18, 12, 13, 14, 8, 9, 10, 4,
      5, 6, 0, 1, 2);
  const __m512i shuffled = _mm512_permutexvar_epi8(pack, merged);
  _mm512_mask_storeu_epi8(
      (__m512i *)out, 0xffffffffffff,
      shuffled); // mask would be 0xffffffffffff since we write 48 bytes.
}
// decode 64 bytes and output 48 bytes
static inline void base64_decode_block(char *out, const char *src) {
  base64_decode(out,
                _mm512_loadu_si512(reinterpret_cast<const __m512i *>(src)));
}
static inline void base64_decode_block(char *out, block64 *b) {
  base64_decode(out, b->chunks[0]);
}

template <bool base64_url, bool ignore_garbage, typename chartype>
full_result
compress_decode_base64(char *dst, const chartype *src, size_t srclen,
                       base64_options options,
                       last_chunk_handling_options last_chunk_options) {
  (void)options;
  const uint8_t *to_base64 = base64_url ? tables::base64::to_base64_url_value
                                        : tables::base64::to_base64_value;
  size_t equallocation =
      srclen; // location of the first padding character if any
  size_t equalsigns = 0;
  // skip trailing spaces
  while (!ignore_garbage && srclen > 0 &&
         scalar::base64::is_eight_byte(src[srclen - 1]) &&
         to_base64[uint8_t(src[srclen - 1])] == 64) {
    srclen--;
  }
  if (!ignore_garbage && srclen > 0 && src[srclen - 1] == '=') {
    equallocation = srclen - 1;
    srclen--;
    equalsigns = 1;
    // skip trailing spaces
    while (srclen > 0 && scalar::base64::is_eight_byte(src[srclen - 1]) &&
           to_base64[uint8_t(src[srclen - 1])] == 64) {
      srclen--;
    }
    if (srclen > 0 && src[srclen - 1] == '=') {
      equallocation = srclen - 1;
      srclen--;
      equalsigns = 2;
    }
  }
  if (srclen == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      if (last_chunk_options == last_chunk_handling_options::strict) {
        return {BASE64_INPUT_REMAINDER, 0, 0};
      } else if (last_chunk_options ==
                 last_chunk_handling_options::stop_before_partial) {
        return {SUCCESS, 0, 0};
      }
      return {INVALID_BASE64_CHARACTER, equallocation, 0};
    }
    return {SUCCESS, 0, 0};
  }
  const chartype *const srcinit = src;
  const char *const dstinit = dst;
  const chartype *const srcend = src + srclen;

  // figure out why block_size == 2 is sometimes best???
  constexpr size_t block_size = 6;
  char buffer[block_size * 64];
  char *bufferptr = buffer;
  if (srclen >= 64) {
    const chartype *const srcend64 = src + srclen - 64;
    while (src <= srcend64) {
      block64 b;
      load_block(&b, src);
      src += 64;
      uint64_t error = 0;
      uint64_t badcharmask =
          to_base64_mask<base64_url, ignore_garbage>(&b, &error);
      if (!ignore_garbage && error) {
        src -= 64;
        size_t error_offset = _tzcnt_u64(error);
        return {error_code::INVALID_BASE64_CHARACTER,
                size_t(src - srcinit + error_offset), size_t(dst - dstinit)};
      }
      if (badcharmask != 0) {
        // optimization opportunity: check for simple masks like those made of
        // continuous 1s followed by continuous 0s. And masks containing a
        // single bad character.
        bufferptr += compress_block(&b, badcharmask, bufferptr);
      } else if (bufferptr != buffer) {
        copy_block(&b, bufferptr);
        bufferptr += 64;
      } else {
        base64_decode_block(dst, &b);
        dst += 48;
      }
      if (bufferptr >= (block_size - 1) * 64 + buffer) {
        for (size_t i = 0; i < (block_size - 1); i++) {
          base64_decode_block(dst, buffer + i * 64);
          dst += 48;
        }
        std::memcpy(buffer, buffer + (block_size - 1) * 64,
                    64); // 64 might be too much
        bufferptr -= (block_size - 1) * 64;
      }
    }
  }

  int last_block_len = (int)(srcend - src);
  if (last_block_len != 0) {
    __mmask64 input_mask = ((__mmask64)1 << last_block_len) - 1;
    block64 b;
    load_block_partial(&b, src, input_mask);
    uint64_t error = 0;
    uint64_t badcharmask =
        to_base64_mask<base64_url, ignore_garbage>(&b, &error, input_mask);
    if (!ignore_garbage && error) {
      size_t error_offset = _tzcnt_u64(error);
      return {error_code::INVALID_BASE64_CHARACTER,
              size_t(src - srcinit + error_offset), size_t(dst - dstinit)};
    }
    src += last_block_len;
    bufferptr += compress_block(&b, badcharmask, bufferptr);
  }

  char *buffer_start = buffer;
  for (; buffer_start + 64 <= bufferptr; buffer_start += 64) {
    base64_decode_block(dst, buffer_start);
    dst += 48;
  }

  if ((bufferptr - buffer_start) != 0) {
    size_t rem = (bufferptr - buffer_start);
    int idx = rem % 4;
    __mmask64 mask = ((__mmask64)1 << rem) - 1;
    __m512i input = _mm512_maskz_loadu_epi8(mask, buffer_start);
    size_t output_len = (rem / 4) * 3;
    __mmask64 output_mask = mask >> (rem - output_len);
    const __m512i merge_ab_and_bc =
        _mm512_maddubs_epi16(input, _mm512_set1_epi32(0x01400140));
    const __m512i merged =
        _mm512_madd_epi16(merge_ab_and_bc, _mm512_set1_epi32(0x00011000));
    const __m512i pack = _mm512_set_epi8(
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 60, 61, 62, 56, 57, 58,
        52, 53, 54, 48, 49, 50, 44, 45, 46, 40, 41, 42, 36, 37, 38, 32, 33, 34,
        28, 29, 30, 24, 25, 26, 20, 21, 22, 16, 17, 18, 12, 13, 14, 8, 9, 10, 4,
        5, 6, 0, 1, 2);
    const __m512i shuffled = _mm512_permutexvar_epi8(pack, merged);

    if (!ignore_garbage &&
        last_chunk_options == last_chunk_handling_options::strict &&
        (idx != 1) && ((idx + equalsigns) & 3) != 0) {
      // The partial chunk was at src - idx
      _mm512_mask_storeu_epi8((__m512i *)dst, output_mask, shuffled);
      dst += output_len;
      return {BASE64_INPUT_REMAINDER, size_t(src - srcinit),
              size_t(dst - dstinit)};
    } else if (!ignore_garbage &&
               last_chunk_options ==
                   last_chunk_handling_options::stop_before_partial &&
               (idx != 1) && ((idx + equalsigns) & 3) != 0) {
      // Rewind src to before partial chunk
      _mm512_mask_storeu_epi8((__m512i *)dst, output_mask, shuffled);
      dst += output_len;
      src -= idx;
    } else {
      if (idx == 2) {
        if (!ignore_garbage &&
            last_chunk_options == last_chunk_handling_options::strict) {
          uint32_t triple = (uint32_t(bufferptr[-2]) << 3 * 6) +
                            (uint32_t(bufferptr[-1]) << 2 * 6);
          if (triple & 0xffff) {
            _mm512_mask_storeu_epi8((__m512i *)dst, output_mask, shuffled);
            dst += output_len;
            return {BASE64_EXTRA_BITS, size_t(src - srcinit),
                    size_t(dst - dstinit)};
          }
        }
        output_mask = (output_mask << 1) | 1;
        output_len += 1;
        _mm512_mask_storeu_epi8((__m512i *)dst, output_mask, shuffled);
        dst += output_len;
      } else if (idx == 3) {
        if (!ignore_garbage &&
            last_chunk_options == last_chunk_handling_options::strict) {
          uint32_t triple = (uint32_t(bufferptr[-3]) << 3 * 6) +
                            (uint32_t(bufferptr[-2]) << 2 * 6) +
                            (uint32_t(bufferptr[-1]) << 1 * 6);
          if (triple & 0xff) {
            _mm512_mask_storeu_epi8((__m512i *)dst, output_mask, shuffled);
            dst += output_len;
            return {BASE64_EXTRA_BITS, size_t(src - srcinit),
                    size_t(dst - dstinit)};
          }
        }
        output_mask = (output_mask << 2) | 3;
        output_len += 2;
        _mm512_mask_storeu_epi8((__m512i *)dst, output_mask, shuffled);
        dst += output_len;
      } else if (!ignore_garbage && idx == 1) {
        _mm512_mask_storeu_epi8((__m512i *)dst, output_mask, shuffled);
        dst += output_len;
        return {BASE64_INPUT_REMAINDER, size_t(src - srcinit),
                size_t(dst - dstinit)};
      } else {
        _mm512_mask_storeu_epi8((__m512i *)dst, output_mask, shuffled);
        dst += output_len;
      }
    }

    if (!ignore_garbage && last_chunk_options != stop_before_partial &&
        equalsigns > 0) {
      size_t output_count = size_t(dst - dstinit);
      if ((output_count % 3 == 0) ||
          ((output_count % 3) + 1 + equalsigns != 4)) {
        return {INVALID_BASE64_CHARACTER, equallocation, output_count};
      }
    }

    return {SUCCESS, srclen, size_t(dst - dstinit)};
  }

  if (!ignore_garbage && equalsigns > 0) {
    if (last_chunk_options == last_chunk_handling_options::strict) {
      return {BASE64_INPUT_REMAINDER, size_t(src - srcinit),
              size_t(dst - dstinit)};
    }
    if (last_chunk_options ==
        last_chunk_handling_options::stop_before_partial) {
      return {SUCCESS, size_t(src - srcinit), size_t(dst - dstinit)};
    }
    if ((size_t(dst - dstinit) % 3 == 0) ||
        ((size_t(dst - dstinit) % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation, size_t(dst - dstinit)};
    }
  }
  return {SUCCESS, srclen, size_t(dst - dstinit)};
}
