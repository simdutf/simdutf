#ifndef SIMDUTF_BASE64_H
#define SIMDUTF_BASE64_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace simdutf {
namespace scalar {
namespace {
namespace base64 {

simdutf_really_inline simdutf_constexpr23 void
copy_encode_pair(char *dst, const std::array<char, 2> &pair) {
#if SIMDUTF_CPLUSPLUS20
  if (std::is_constant_evaluated()) {
    dst[0] = pair[0];
    dst[1] = pair[1];
    return;
  }
#endif
  std::memcpy(dst, pair.data(), pair.size());
}

// This function is not expected to be fast. Do not use in long loops.
// In most instances you should be using is_ignorable.
template <class char_type> bool is_ascii_white_space(char_type c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

template <class char_type> simdutf_constexpr23 bool is_eight_byte(char_type c) {
  if constexpr (sizeof(char_type) == 1) {
    return true;
  }
  return uint8_t(c) == c;
}

template <class char_type>
simdutf_constexpr23 bool is_ignorable(char_type c,
                                      simdutf::base64_options options) {
  const uint8_t *to_base64 =
      (options & base64_default_or_url)
          ? tables::base64::to_base64_default_or_url_value
          : ((options & base64_url) ? tables::base64::to_base64_url_value
                                    : tables::base64::to_base64_value);
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage) ||
      (options == base64_options::base64_default_or_url_accept_garbage);
  uint8_t code = to_base64[uint8_t(c)];
  if (is_eight_byte(c) && code <= 63) {
    return false;
  }
  if (is_eight_byte(c) && code == 64) {
    return true;
  }
  return ignore_garbage;
}
template <class char_type>
simdutf_constexpr23 bool is_base64(char_type c,
                                   simdutf::base64_options options) {
  const uint8_t *to_base64 =
      (options & base64_default_or_url)
          ? tables::base64::to_base64_default_or_url_value
          : ((options & base64_url) ? tables::base64::to_base64_url_value
                                    : tables::base64::to_base64_value);
  uint8_t code = to_base64[uint8_t(c)];
  if (is_eight_byte(c) && code <= 63) {
    return true;
  }
  return false;
}

template <class char_type>
simdutf_constexpr23 bool is_base64_or_padding(char_type c,
                                              simdutf::base64_options options) {
  const uint8_t *to_base64 =
      (options & base64_default_or_url)
          ? tables::base64::to_base64_default_or_url_value
          : ((options & base64_url) ? tables::base64::to_base64_url_value
                                    : tables::base64::to_base64_value);
  if (c == '=') {
    return true;
  }
  uint8_t code = to_base64[uint8_t(c)];
  if (is_eight_byte(c) && code <= 63) {
    return true;
  }
  return false;
}

template <class char_type>
bool is_ignorable_or_padding(char_type c, simdutf::base64_options options) {
  return is_ignorable(c, options) || c == '=';
}

struct reduced_input {
  size_t equalsigns;    // number of padding characters '=', typically 0, 1, 2.
  size_t equallocation; // location of the first padding character if any
  size_t srclen;        // length of the input buffer before padding
  size_t full_input_length; // length of the input buffer with padding but
                            // without ignorable characters
};

// find the end of the base64 input buffer
// It returns the number of padding characters, the location of the first
// padding character if any, the length of the input buffer before padding
// and the length of the input buffer with padding. The input buffer is not
// modified. The function assumes that there are at most two padding characters.
template <class char_type>
simdutf_constexpr23 reduced_input find_end(const char_type *src, size_t srclen,
                                           simdutf::base64_options options) {
  const uint8_t *to_base64 =
      (options & base64_default_or_url)
          ? tables::base64::to_base64_default_or_url_value
          : ((options & base64_url) ? tables::base64::to_base64_url_value
                                    : tables::base64::to_base64_value);
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage) ||
      (options == base64_options::base64_default_or_url_accept_garbage);

  size_t equalsigns = 0;
  // We intentionally include trailing spaces in the full input length.
  // See https://github.com/simdutf/simdutf/issues/824
  size_t full_input_length = srclen;
  // skip trailing spaces
  while (!ignore_garbage && srclen > 0 &&
         scalar::base64::is_eight_byte(src[srclen - 1]) &&
         to_base64[uint8_t(src[srclen - 1])] == 64) {
    srclen--;
  }
  size_t equallocation =
      srclen; // location of the first padding character if any
  if (ignore_garbage) {
    // Technically, we don't need to find the first padding character, we can
    // just change our algorithms, but it adds substantial complexity.
    auto it = simdutf::find(src, src + srclen, '=');
    if (it != src + srclen) {
      equallocation = it - src;
      equalsigns = 1;
      srclen = equallocation;
      full_input_length = equallocation + 1;
    }
    return {equalsigns, equallocation, srclen, full_input_length};
  }
  if (!ignore_garbage && srclen > 0 && src[srclen - 1] == '=') {
    // This is the last '=' sign.
    equallocation = srclen - 1;
    srclen--;
    equalsigns = 1;
    // skip trailing spaces
    while (srclen > 0 && scalar::base64::is_eight_byte(src[srclen - 1]) &&
           to_base64[uint8_t(src[srclen - 1])] == 64) {
      srclen--;
    }
    if (srclen > 0 && src[srclen - 1] == '=') {
      // This is the second '=' sign.
      equallocation = srclen - 1;
      srclen--;
      equalsigns = 2;
    }
  }
  return {equalsigns, equallocation, srclen, full_input_length};
}

// Returns true upon success. The destination buffer must be large enough.
// This functions assumes that the padding (=) has been removed.
// if check_capacity is true, it will check that the destination buffer is
// large enough. If it is not, it will return OUTPUT_BUFFER_TOO_SMALL.
template <bool check_capacity, class char_type>
simdutf_constexpr23 full_result base64_tail_decode_impl(
    char *dst, size_t outlen, const char_type *src, size_t length,
    size_t padding_characters, // number of padding characters
                               // '=', typically 0, 1, 2.
    base64_options options, last_chunk_handling_options last_chunk_options) {
  char *dstend = dst + outlen;
  (void)dstend;
  // This looks like 10 branches, but we expect the compiler to resolve this to
  // two branches (easily predicted):
  const uint8_t *to_base64 =
      (options & base64_default_or_url)
          ? tables::base64::to_base64_default_or_url_value
          : ((options & base64_url) ? tables::base64::to_base64_url_value
                                    : tables::base64::to_base64_value);
  const uint32_t *d0 =
      (options & base64_default_or_url)
          ? tables::base64::base64_default_or_url::d0
          : ((options & base64_url) ? tables::base64::base64_url::d0
                                    : tables::base64::base64_default::d0);
  const uint32_t *d1 =
      (options & base64_default_or_url)
          ? tables::base64::base64_default_or_url::d1
          : ((options & base64_url) ? tables::base64::base64_url::d1
                                    : tables::base64::base64_default::d1);
  const uint32_t *d2 =
      (options & base64_default_or_url)
          ? tables::base64::base64_default_or_url::d2
          : ((options & base64_url) ? tables::base64::base64_url::d2
                                    : tables::base64::base64_default::d2);
  const uint32_t *d3 =
      (options & base64_default_or_url)
          ? tables::base64::base64_default_or_url::d3
          : ((options & base64_url) ? tables::base64::base64_url::d3
                                    : tables::base64::base64_default::d3);
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage) ||
      (options == base64_options::base64_default_or_url_accept_garbage);

  const char_type *srcend = src + length;
  const char_type *srcinit = src;
  const char *dstinit = dst;

  uint32_t x;
  size_t idx;
  uint8_t buffer[4];
  while (true) {
    // Decode three clean quartets together. On exceptional input, leave the
    // complete block to the policy-aware path below.
    while (srcend - src >= 12 && (!check_capacity || dstend - dst >= 9) &&
           is_eight_byte(src[0]) && is_eight_byte(src[1]) &&
           is_eight_byte(src[2]) && is_eight_byte(src[3]) &&
           is_eight_byte(src[4]) && is_eight_byte(src[5]) &&
           is_eight_byte(src[6]) && is_eight_byte(src[7]) &&
           is_eight_byte(src[8]) && is_eight_byte(src[9]) &&
           is_eight_byte(src[10]) && is_eight_byte(src[11])) {
      const uint32_t x0 = d0[uint8_t(src[0])] | d1[uint8_t(src[1])] |
                          d2[uint8_t(src[2])] | d3[uint8_t(src[3])];
      const uint32_t x1 = d0[uint8_t(src[4])] | d1[uint8_t(src[5])] |
                          d2[uint8_t(src[6])] | d3[uint8_t(src[7])];
      const uint32_t x2 = d0[uint8_t(src[8])] | d1[uint8_t(src[9])] |
                          d2[uint8_t(src[10])] | d3[uint8_t(src[11])];
      if ((x0 | x1 | x2) >= 0x01FFFFFF) {
        break;
      }
#if SIMDUTF_IS_BIG_ENDIAN
      dst[0] = static_cast<char>(x0);
      dst[1] = static_cast<char>(x0 >> 8);
      dst[2] = static_cast<char>(x0 >> 16);
      dst[3] = static_cast<char>(x1);
      dst[4] = static_cast<char>(x1 >> 8);
      dst[5] = static_cast<char>(x1 >> 16);
      dst[6] = static_cast<char>(x2);
      dst[7] = static_cast<char>(x2 >> 8);
      dst[8] = static_cast<char>(x2 >> 16);
#else
      const uint64_t word = uint64_t(x0 & 0x00FFFFFF) |
                            (uint64_t(x1 & 0x00FFFFFF) << 24) |
                            (uint64_t(x2 & 0xFFFF) << 48);
  #if SIMDUTF_CPLUSPLUS20
      if (std::is_constant_evaluated()) {
        dst[0] = static_cast<char>(x0);
        dst[1] = static_cast<char>(x0 >> 8);
        dst[2] = static_cast<char>(x0 >> 16);
        dst[3] = static_cast<char>(x1);
        dst[4] = static_cast<char>(x1 >> 8);
        dst[5] = static_cast<char>(x1 >> 16);
        dst[6] = static_cast<char>(x2);
        dst[7] = static_cast<char>(x2 >> 8);
      } else
  #endif
      {
        std::memcpy(dst, &word, sizeof(word));
      }
      dst[8] = static_cast<char>(x2 >> 16);
#endif
      src += 12;
      dst += 9;
    }
    while (srcend - src >= 4 && is_eight_byte(src[0]) &&
           is_eight_byte(src[1]) && is_eight_byte(src[2]) &&
           is_eight_byte(src[3]) &&
           (x = d0[uint8_t(src[0])] | d1[uint8_t(src[1])] |
                d2[uint8_t(src[2])] | d3[uint8_t(src[3])]) < 0x01FFFFFF) {
      if (check_capacity && dstend - dst < 3) {
        return {OUTPUT_BUFFER_TOO_SMALL, size_t(src - srcinit),
                size_t(dst - dstinit)};
      }
      *dst++ = static_cast<char>(x & 0xFF);
      *dst++ = static_cast<char>((x >> 8) & 0xFF);
      *dst++ = static_cast<char>((x >> 16) & 0xFF);
      src += 4;
    }
    const char_type *srccur = src;
    idx = 0;
    // we need at least four characters.
#ifdef __clang__
    // If possible, we read four characters at a time. (It is an optimization.)
    if (ignore_garbage && src + 4 <= srcend) {
      char_type c0 = src[0];
      char_type c1 = src[1];
      char_type c2 = src[2];
      char_type c3 = src[3];

      uint8_t code0 = to_base64[uint8_t(c0)];
      uint8_t code1 = to_base64[uint8_t(c1)];
      uint8_t code2 = to_base64[uint8_t(c2)];
      uint8_t code3 = to_base64[uint8_t(c3)];

      buffer[idx] = code0;
      idx += (is_eight_byte(c0) && code0 <= 63);
      buffer[idx] = code1;
      idx += (is_eight_byte(c1) && code1 <= 63);
      buffer[idx] = code2;
      idx += (is_eight_byte(c2) && code2 <= 63);
      buffer[idx] = code3;
      idx += (is_eight_byte(c3) && code3 <= 63);
      src += 4;
    }
#endif
    while ((idx < 4) && (src < srcend)) {
      char_type c = *src;

      uint8_t code = to_base64[uint8_t(c)];
      buffer[idx] = uint8_t(code);
      if (is_eight_byte(c) && code <= 63) {
        idx++;
      } else if (!ignore_garbage &&
                 (code > 64 || !scalar::base64::is_eight_byte(c))) {
        return {INVALID_BASE64_CHARACTER, size_t(src - srcinit),
                size_t(dst - dstinit)};
      } else {
        // We have a space or a newline or garbage. We ignore it.
      }
      src++;
    }
    if (idx != 4) {
      simdutf_log_assert(idx < 4, "idx should be less than 4");
      // We never should have that the number of base64 characters + the
      // number of padding characters is more than 4.
      if (!ignore_garbage && (idx + padding_characters > 4)) {
        return {INVALID_BASE64_CHARACTER, size_t(src - srcinit),
                size_t(dst - dstinit), true};
      }

      // The idea here is that in loose mode,
      // if there is padding at all, it must be used
      // to form 4-wise chunk. However, in loose mode,
      // we do accept no padding at all.
      if (!ignore_garbage &&
          last_chunk_options == last_chunk_handling_options::loose &&
          (idx >= 2) && padding_characters > 0 &&
          ((idx + padding_characters) & 3) != 0) {
        return {INVALID_BASE64_CHARACTER, size_t(src - srcinit),
                size_t(dst - dstinit), true};
      } else

        // The idea here is that in strict mode, we do not want to accept
        // incomplete base64 chunks. So if the chunk was otherwise valid, we
        // return BASE64_INPUT_REMAINDER.
        if (!ignore_garbage &&
            last_chunk_options == last_chunk_handling_options::strict &&
            (idx >= 2) && ((idx + padding_characters) & 3) != 0) {
          // The partial chunk was at src - idx
          return {BASE64_INPUT_REMAINDER, size_t(src - srcinit),
                  size_t(dst - dstinit), true};
        } else
          // If there is a partial chunk with insufficient padding, with
          // stop_before_partial, we need to just ignore it. In "only full"
          // mode, skip the minute there are padding characters.
          if ((last_chunk_options ==
                   last_chunk_handling_options::stop_before_partial &&
               (padding_characters + idx < 4) && (idx != 0) &&
               (idx >= 2 || padding_characters == 0)) ||
              (last_chunk_options ==
                   last_chunk_handling_options::only_full_chunks &&
               (idx >= 2 || padding_characters == 0))) {
            // partial means that we are *not* going to consume the read
            // characters. We need to rewind the src pointer.
            src = srccur;
            return {SUCCESS, size_t(src - srcinit), size_t(dst - dstinit)};
          } else {
            if (idx == 2) {
              uint32_t triple = (uint32_t(buffer[0]) << 3 * 6) +
                                (uint32_t(buffer[1]) << 2 * 6);
              if (!ignore_garbage &&
                  (last_chunk_options == last_chunk_handling_options::strict) &&
                  (triple & 0xffff)) {
                return {BASE64_EXTRA_BITS, size_t(src - srcinit),
                        size_t(dst - dstinit)};
              }
              if (check_capacity && dstend - dst < 1) {
                return {OUTPUT_BUFFER_TOO_SMALL, size_t(srccur - srcinit),
                        size_t(dst - dstinit)};
              }
              *dst++ = static_cast<char>((triple >> 16) & 0xFF);
            } else if (idx == 3) {
              uint32_t triple = (uint32_t(buffer[0]) << 3 * 6) +
                                (uint32_t(buffer[1]) << 2 * 6) +
                                (uint32_t(buffer[2]) << 1 * 6);
              if (!ignore_garbage &&
                  (last_chunk_options == last_chunk_handling_options::strict) &&
                  (triple & 0xff)) {
                return {BASE64_EXTRA_BITS, size_t(src - srcinit),
                        size_t(dst - dstinit)};
              }
              if (check_capacity && dstend - dst < 2) {
                return {OUTPUT_BUFFER_TOO_SMALL, size_t(srccur - srcinit),
                        size_t(dst - dstinit)};
              }
              *dst++ = static_cast<char>((triple >> 16) & 0xFF);
              *dst++ = static_cast<char>((triple >> 8) & 0xFF);
            } else if (!ignore_garbage && idx == 1 &&
                       (!is_partial(last_chunk_options) ||
                        (is_partial(last_chunk_options) &&
                         padding_characters > 0))) {
              return {BASE64_INPUT_REMAINDER, size_t(src - srcinit),
                      size_t(dst - dstinit)};
            } else if (!ignore_garbage && idx == 0 && padding_characters > 0) {
              return {INVALID_BASE64_CHARACTER, size_t(src - srcinit),
                      size_t(dst - dstinit), true};
            }
            return {SUCCESS, size_t(src - srcinit), size_t(dst - dstinit)};
          }
    }
    if (check_capacity && dstend - dst < 3) {
      return {OUTPUT_BUFFER_TOO_SMALL, size_t(srccur - srcinit),
              size_t(dst - dstinit)};
    }
    uint32_t triple =
        (uint32_t(buffer[0]) << 3 * 6) + (uint32_t(buffer[1]) << 2 * 6) +
        (uint32_t(buffer[2]) << 1 * 6) + (uint32_t(buffer[3]) << 0 * 6);
    *dst++ = static_cast<char>((triple >> 16) & 0xFF);
    *dst++ = static_cast<char>((triple >> 8) & 0xFF);
    *dst++ = static_cast<char>(triple & 0xFF);
  }
}

template <class char_type>
simdutf_constexpr23 full_result base64_tail_decode(
    char *dst, const char_type *src, size_t length,
    size_t padding_characters, // number of padding characters
                               // '=', typically 0, 1, 2.
    base64_options options, last_chunk_handling_options last_chunk_options) {
  return base64_tail_decode_impl<false>(dst, 0, src, length, padding_characters,
                                        options, last_chunk_options);
}

// like base64_tail_decode, but it will not write past the end of the output
// buffer. The outlen parameter is modified to reflect the number of bytes
// written. This functions assumes that the padding (=) has been removed.
//
template <class char_type>
simdutf_constexpr23 full_result base64_tail_decode_safe(
    char *dst, size_t outlen, const char_type *src, size_t length,
    size_t padding_characters, // number of padding characters
                               // '=', typically 0, 1, 2.
    base64_options options, last_chunk_handling_options last_chunk_options) {
  return base64_tail_decode_impl<true>(dst, outlen, src, length,
                                       padding_characters, options,
                                       last_chunk_options);
}

inline simdutf_constexpr23 full_result
patch_tail_result(full_result r, size_t previous_input, size_t previous_output,
                  size_t equallocation, size_t full_input_length,
                  last_chunk_handling_options last_chunk_options) {
  r.input_count += previous_input;
  r.output_count += previous_output;
  if (r.padding_error) {
    r.input_count = equallocation;
  }

  if (r.error == error_code::SUCCESS) {
    if (!is_partial(last_chunk_options)) {
      // A success when we are not in stop_before_partial mode.
      // means that we have consumed the whole input buffer.
      r.input_count = full_input_length;
    } else if (r.output_count % 3 != 0) {
      r.input_count = full_input_length;
    }
  }
  return r;
}

simdutf_really_inline simdutf_constexpr23 void
tail_encode_base64_quantum(char *dst, const char *src,
                           const std::array<char, 2> *pairs) {
  const uint32_t value = (uint32_t(uint8_t(src[0])) << 16) |
                         (uint32_t(uint8_t(src[1])) << 8) | uint8_t(src[2]);
  copy_encode_pair(dst, pairs[value >> 12]);
  copy_encode_pair(dst + 2, pairs[value & 0xFFF]);
}

simdutf_really_inline simdutf_constexpr23 size_t tail_encode_base64_short_impl(
    char *dst, const char *src, size_t srclen, base64_options options) {
  const auto *pairs = (options & base64_url)
                          ? tables::base64::base64_url::encode_pairs.data()
                          : tables::base64::base64_default::encode_pairs.data();
  const bool use_padding =
      ((options & base64_url) == 0) ^
      ((options & base64_reverse_padding) == base64_reverse_padding);

#define SIMDUTF_BASE64_ENCODE_QUANTUM(index)                                   \
  tail_encode_base64_quantum(dst + (index) * 4, src + (index) * 3, pairs)
#define SIMDUTF_BASE64_ENCODE_TAIL1(index)                                     \
  do {                                                                         \
    const uint32_t value = uint32_t(uint8_t(src[(index) * 3])) << 16;          \
    copy_encode_pair(dst + (index) * 4, pairs[value >> 12]);                   \
    if (use_padding) {                                                         \
      dst[(index) * 4 + 2] = '=';                                              \
      dst[(index) * 4 + 3] = '=';                                              \
    }                                                                          \
  } while (false)
#define SIMDUTF_BASE64_ENCODE_TAIL2(index)                                     \
  do {                                                                         \
    const uint32_t value = (uint32_t(uint8_t(src[(index) * 3])) << 16) |       \
                           (uint32_t(uint8_t(src[(index) * 3 + 1])) << 8);     \
    copy_encode_pair(dst + (index) * 4, pairs[value >> 12]);                   \
    dst[(index) * 4 + 2] = pairs[value & 0xFFF][0];                            \
    if (use_padding) {                                                         \
      dst[(index) * 4 + 3] = '=';                                              \
    }                                                                          \
  } while (false)

  switch (srclen) {
  case 11:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    SIMDUTF_BASE64_ENCODE_QUANTUM(1);
    SIMDUTF_BASE64_ENCODE_QUANTUM(2);
    SIMDUTF_BASE64_ENCODE_TAIL2(3);
    return 15 + size_t(use_padding);
  case 10:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    SIMDUTF_BASE64_ENCODE_QUANTUM(1);
    SIMDUTF_BASE64_ENCODE_QUANTUM(2);
    SIMDUTF_BASE64_ENCODE_TAIL1(3);
    return 14 + 2 * size_t(use_padding);
  case 9:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    SIMDUTF_BASE64_ENCODE_QUANTUM(1);
    SIMDUTF_BASE64_ENCODE_QUANTUM(2);
    return 12;
  case 8:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    SIMDUTF_BASE64_ENCODE_QUANTUM(1);
    SIMDUTF_BASE64_ENCODE_TAIL2(2);
    return 11 + size_t(use_padding);
  case 7:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    SIMDUTF_BASE64_ENCODE_QUANTUM(1);
    SIMDUTF_BASE64_ENCODE_TAIL1(2);
    return 10 + 2 * size_t(use_padding);
  case 6:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    SIMDUTF_BASE64_ENCODE_QUANTUM(1);
    return 8;
  case 5:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    SIMDUTF_BASE64_ENCODE_TAIL2(1);
    return 7 + size_t(use_padding);
  case 4:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    SIMDUTF_BASE64_ENCODE_TAIL1(1);
    return 6 + 2 * size_t(use_padding);
  case 3:
    SIMDUTF_BASE64_ENCODE_QUANTUM(0);
    return 4;
  case 2:
    SIMDUTF_BASE64_ENCODE_TAIL2(0);
    return 3 + size_t(use_padding);
  case 1:
    SIMDUTF_BASE64_ENCODE_TAIL1(0);
    return 2 + 2 * size_t(use_padding);
  default:
    return 0;
  }

#undef SIMDUTF_BASE64_ENCODE_TAIL2
#undef SIMDUTF_BASE64_ENCODE_TAIL1
#undef SIMDUTF_BASE64_ENCODE_QUANTUM
}

simdutf_constexpr23 size_t tail_encode_base64_long_impl(char *dst,
                                                        const char *src,
                                                        size_t srclen,
                                                        base64_options options);

// Returns the number of bytes written. The destination buffer must be large
// enough. It will add padding (=) if needed.
template <bool use_lines = false>
simdutf_constexpr23 size_t tail_encode_base64_impl(
    char *dst, const char *src, size_t srclen, base64_options options,
    size_t line_length = simdutf::default_line_length, size_t line_offset = 0) {
  if constexpr (use_lines) {
    // sanitize line_length and starting_line_offset.
    // line_length must be greater than 3.
    if (line_length < 4) {
      line_length = 4;
    }
    simdutf_log_assert(line_offset <= line_length,
                       "line_offset should be less than line_length");
  }
  // By default, we use padding if we are not using the URL variant.
  // This is check with ((options & base64_url) == 0) which returns true if we
  // are not using the URL variant. However, we also allow 'inversion' of the
  // convention with the base64_reverse_padding option. If the
  // base64_reverse_padding option is set, we use padding if we are using the
  // URL variant, and we omit it if we are not using the URL variant. This is
  // checked with
  // ((options & base64_reverse_padding) == base64_reverse_padding).
  bool use_padding =
      ((options & base64_url) == 0) ^
      ((options & base64_reverse_padding) == base64_reverse_padding);
  // This looks like 3 branches, but we expect the compiler to resolve this to
  // a single branch:
  const char *e0 = (options & base64_url) ? tables::base64::base64_url::e0
                                          : tables::base64::base64_default::e0;
  const char *e1 = (options & base64_url) ? tables::base64::base64_url::e1
                                          : tables::base64::base64_default::e1;
  const char *e2 = (options & base64_url) ? tables::base64::base64_url::e2
                                          : tables::base64::base64_default::e2;
  char *out = dst;
  size_t i = 0;
  uint8_t t1, t2, t3;
  if constexpr (!use_lines) {
    if (srclen < 12) {
      return tail_encode_base64_short_impl(dst, src, srclen, options);
    }
    i = srclen - srclen % 12;
    out += tail_encode_base64_long_impl(dst, src, i, options);
    return size_t(out - dst) +
           tail_encode_base64_short_impl(out, src + i, srclen - i, options);
  }
  for (; i + 2 < srclen; i += 3) {
    t1 = uint8_t(src[i]);
    t2 = uint8_t(src[i + 1]);
    t3 = uint8_t(src[i + 2]);
    if constexpr (use_lines) {
      if (line_offset + 3 >= line_length) {
        if (line_offset == line_length) {
          *out++ = '\n';
          *out++ = e0[t1];
          *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
          *out++ = e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
          *out++ = e2[t3];
          line_offset = 4;
        } else if (line_offset + 1 == line_length) {
          *out++ = e0[t1];
          *out++ = '\n';
          *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
          *out++ = e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
          *out++ = e2[t3];
          line_offset = 3;
        } else if (line_offset + 2 == line_length) {
          *out++ = e0[t1];
          *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
          *out++ = '\n';
          *out++ = e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
          *out++ = e2[t3];
          line_offset = 2;
        } else if (line_offset + 3 == line_length) {
          *out++ = e0[t1];
          *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
          *out++ = e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
          *out++ = '\n';
          *out++ = e2[t3];
          line_offset = 1;
        }
      } else {
        *out++ = e0[t1];
        *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
        *out++ = e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
        *out++ = e2[t3];
        line_offset += 4;
      }
    } else {
      *out++ = e0[t1];
      *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
      *out++ = e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
      *out++ = e2[t3];
    }
  }
  switch (srclen - i) {
  case 0:
    break;
  case 1:
    t1 = uint8_t(src[i]);
    if constexpr (use_lines) {
      if (use_padding) {
        if (line_offset + 3 >= line_length) {
          if (line_offset == line_length) {
            *out++ = '\n';
            *out++ = e0[t1];
            *out++ = e1[(t1 & 0x03) << 4];
            *out++ = '=';
            *out++ = '=';
          } else if (line_offset + 1 == line_length) {
            *out++ = e0[t1];
            *out++ = '\n';
            *out++ = e1[(t1 & 0x03) << 4];
            *out++ = '=';
            *out++ = '=';
          } else if (line_offset + 2 == line_length) {
            *out++ = e0[t1];
            *out++ = e1[(t1 & 0x03) << 4];
            *out++ = '\n';
            *out++ = '=';
            *out++ = '=';
          } else if (line_offset + 3 == line_length) {
            *out++ = e0[t1];
            *out++ = e1[(t1 & 0x03) << 4];
            *out++ = '=';
            *out++ = '\n';
            *out++ = '=';
          }
        } else {
          *out++ = e0[t1];
          *out++ = e1[(t1 & 0x03) << 4];
          *out++ = '=';
          *out++ = '=';
        }
      } else {
        if (line_offset + 2 >= line_length) {
          if (line_offset == line_length) {
            *out++ = '\n';
            *out++ = e0[uint8_t(src[i])];
            *out++ = e1[(uint8_t(src[i]) & 0x03) << 4];
          } else if (line_offset + 1 == line_length) {
            *out++ = e0[uint8_t(src[i])];
            *out++ = '\n';
            *out++ = e1[(uint8_t(src[i]) & 0x03) << 4];
          } else {
            *out++ = e0[uint8_t(src[i])];
            *out++ = e1[(uint8_t(src[i]) & 0x03) << 4];
            // *out++ = '\n'; ==> no newline at the end of the output
          }
        } else {
          *out++ = e0[uint8_t(src[i])];
          *out++ = e1[(uint8_t(src[i]) & 0x03) << 4];
        }
      }
    } else {
      *out++ = e0[t1];
      *out++ = e1[(t1 & 0x03) << 4];
      if (use_padding) {
        *out++ = '=';
        *out++ = '=';
      }
    }
    break;
  default: /* case 2 */
    t1 = uint8_t(src[i]);
    t2 = uint8_t(src[i + 1]);
    if constexpr (use_lines) {
      if (use_padding) {
        if (line_offset + 3 >= line_length) {
          if (line_offset == line_length) {
            *out++ = '\n';
            *out++ = e0[t1];
            *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *out++ = e2[(t2 & 0x0F) << 2];
            *out++ = '=';
          } else if (line_offset + 1 == line_length) {
            *out++ = e0[t1];
            *out++ = '\n';
            *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *out++ = e2[(t2 & 0x0F) << 2];
            *out++ = '=';
          } else if (line_offset + 2 == line_length) {
            *out++ = e0[t1];
            *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *out++ = '\n';
            *out++ = e2[(t2 & 0x0F) << 2];
            *out++ = '=';
          } else if (line_offset + 3 == line_length) {
            *out++ = e0[t1];
            *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *out++ = e2[(t2 & 0x0F) << 2];
            *out++ = '\n';
            *out++ = '=';
          }
        } else {
          *out++ = e0[t1];
          *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
          *out++ = e2[(t2 & 0x0F) << 2];
          *out++ = '=';
        }
      } else {
        if (line_offset + 3 >= line_length) {
          if (line_offset == line_length) {
            *out++ = '\n';
            *out++ = e0[t1];
            *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *out++ = e2[(t2 & 0x0F) << 2];
          } else if (line_offset + 1 == line_length) {
            *out++ = e0[t1];
            *out++ = '\n';
            *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *out++ = e2[(t2 & 0x0F) << 2];
          } else if (line_offset + 2 == line_length) {
            *out++ = e0[t1];
            *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *out++ = '\n';
            *out++ = e2[(t2 & 0x0F) << 2];
          } else {
            *out++ = e0[t1];
            *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
            *out++ = e2[(t2 & 0x0F) << 2];
            // *out++ = '\n'; ==> no newline at the end of the output
          }
        } else {
          *out++ = e0[t1];
          *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
          *out++ = e2[(t2 & 0x0F) << 2];
        }
      }
    } else {
      *out++ = e0[t1];
      *out++ = e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
      *out++ = e2[(t2 & 0x0F) << 2];
      if (use_padding) {
        *out++ = '=';
      }
    }
  }
  return (size_t)(out - dst);
}

simdutf_never_inline simdutf_constexpr23 size_t tail_encode_base64_long_impl(
    char *dst, const char *src, size_t srclen, base64_options options) {
  // Keep this loop out of tail_encode_base64_impl so short tails retain the
  // smaller register set and stack frame. srclen is a nonzero multiple of 12.
  // Two 12-bit lookups encode each three-byte quantum. Processing four
  // independent quanta per iteration reduces lookup and loop overhead.
  const auto *pairs = (options & base64_url)
                          ? tables::base64::base64_url::encode_pairs.data()
                          : tables::base64::base64_default::encode_pairs.data();
  char *out = dst;
  size_t i = 0;
  for (; i + 11 < srclen; i += 12) {
    const uint32_t v0 = (uint32_t(uint8_t(src[i])) << 16) |
                        (uint32_t(uint8_t(src[i + 1])) << 8) |
                        uint8_t(src[i + 2]);
    const uint32_t v1 = (uint32_t(uint8_t(src[i + 3])) << 16) |
                        (uint32_t(uint8_t(src[i + 4])) << 8) |
                        uint8_t(src[i + 5]);
    const uint32_t v2 = (uint32_t(uint8_t(src[i + 6])) << 16) |
                        (uint32_t(uint8_t(src[i + 7])) << 8) |
                        uint8_t(src[i + 8]);
    const uint32_t v3 = (uint32_t(uint8_t(src[i + 9])) << 16) |
                        (uint32_t(uint8_t(src[i + 10])) << 8) |
                        uint8_t(src[i + 11]);
    copy_encode_pair(out, pairs[v0 >> 12]);
    copy_encode_pair(out + 2, pairs[v0 & 0xFFF]);
    copy_encode_pair(out + 4, pairs[v1 >> 12]);
    copy_encode_pair(out + 6, pairs[v1 & 0xFFF]);
    copy_encode_pair(out + 8, pairs[v2 >> 12]);
    copy_encode_pair(out + 10, pairs[v2 & 0xFFF]);
    copy_encode_pair(out + 12, pairs[v3 >> 12]);
    copy_encode_pair(out + 14, pairs[v3 & 0xFFF]);
    out += 16;
  }
  return size_t(out - dst);
}

// Returns the number of bytes written. The destination buffer must be large
// enough. It will add padding (=) if needed.
simdutf_unused inline simdutf_constexpr23 size_t tail_encode_base64(
    char *dst, const char *src, size_t srclen, base64_options options) {
  return tail_encode_base64_impl(dst, src, srclen, options);
}

template <class InputPtr>
simdutf_warn_unused simdutf_constexpr23 size_t
maximal_binary_length_from_base64(InputPtr input, size_t length) noexcept {
  // We process the padding characters ('=') at the end to make sure
  // that we return an exact result when the input has no ignorable characters
  // (e.g., spaces).
  size_t padding = 0;
  if (length > 0) {
    if (input[length - 1] == '=') {
      padding++;
      if (length > 1 && input[length - 2] == '=') {
        padding++;
      }
    }
  }
  // The input is not otherwise processed for ignorable characters or
  // validation, so that the function runs in constant time (very fast). In
  // practice, base64 inputs without ignorable characters are common and the
  // common case are line separated inputs with relatively long lines (e.g., 76
  // characters) which leads this function to a slight (1%) overestimation of
  // the output size.
  //
  // Of course, some inputs might contain an arbitrary number of spaces or
  // newlines, which would make this function return a very pessimistic output
  // size but systems that produce base64 outputs typically do not do that and
  // if they do, they do not care much about minimizing memory usage.
  //
  // In specialized applications, users may know that their input is line
  // separated, which can be checked very quickly by by iterating (e.g., over 76
  // character chunks, looking for the linefeed characters only). We could
  // provide a specialized function for that, but it is not clear that the added
  // complexity is worth it for us.
  //
  size_t actual_length = length - padding;
  if (actual_length % 4 <= 1) {
    return actual_length / 4 * 3;
  }
  // if we have a valid input, then the remainder must be 2 or 3 adding one or
  // two extra bytes.
  return actual_length / 4 * 3 + (actual_length % 4) - 1;
}

// This function computes the binary length by iterating through the input
// and counting non-whitespace characters (excluding padding characters).
// We use a simple check (c > ' ') which is easy to parallelize and matches
// SIMD behavior. Only the last few characters are checked for padding '='.
template <class char_type>
simdutf_warn_unused simdutf_constexpr23 size_t
binary_length_from_base64(const char_type *input, size_t length) noexcept {
  // Count non-whitespace characters (c > ' ') with loop unrolling
  size_t count = 0;
  for (size_t i = 0; i < length; i++) {
    count += (input[i] > ' ');
  }

  // Check for padding '=' at the end (at most 2 padding characters)
  // Scan backwards, skipping whitespace, to find padding
  size_t padding = 0;
  size_t pos = length;
  // Skip trailing whitespace
  while (pos > 0 && padding < 2) {
    char_type c = input[--pos];
    if (c == '=') {
      padding++;
    } else if (c > ' ') {
      break;
    }
  }
  return ((count - padding) * 3) / 4;
}

template <typename char_type>
simdutf_warn_unused simdutf_constexpr23 full_result
base64_to_binary_details_impl(
    const char_type *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) noexcept {
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage) ||
      (options == base64_options::base64_default_or_url_accept_garbage);
  auto ri = simdutf::scalar::base64::find_end(input, length, options);
  size_t equallocation = ri.equallocation;
  size_t equalsigns = ri.equalsigns;
  length = ri.srclen;
  size_t full_input_length = ri.full_input_length;
  if (length == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      return {INVALID_BASE64_CHARACTER, equallocation, 0, true};
    }
    return {SUCCESS, full_input_length, 0};
  }
  full_result r = scalar::base64::base64_tail_decode(
      output, input, length, equalsigns, options, last_chunk_options);
  r = scalar::base64::patch_tail_result(r, 0, 0, equallocation,
                                        full_input_length, last_chunk_options);
  if (!is_partial(last_chunk_options) && r.error == error_code::SUCCESS &&
      equalsigns > 0 && !ignore_garbage) {
    // additional checks
    if ((r.output_count % 3 == 0) ||
        ((r.output_count % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation, r.output_count, true};
    }
  }
  // When is_partial(last_chunk_options) is true, we must either end with
  // the end of the stream (beyond whitespace) or right after a non-ignorable
  // character or at the very beginning of the stream.
  // See https://tc39.es/proposal-arraybuffer-base64/spec/#sec-frombase64
  if (is_partial(last_chunk_options) && r.error == error_code::SUCCESS &&
      r.input_count < full_input_length) {
    // First check if we can extend the input to the end of the stream
    while (r.input_count < full_input_length &&
           base64_ignorable(*(input + r.input_count), options)) {
      r.input_count++;
    }
    // If we are still not at the end of the stream, then we must backtrack
    // to the last non-ignorable character.
    if (r.input_count < full_input_length) {
      while (r.input_count > 0 &&
             base64_ignorable(*(input + r.input_count - 1), options)) {
        r.input_count--;
      }
    }
  }
  return r;
}

template <typename char_type>
simdutf_constexpr23 simdutf_warn_unused full_result
base64_to_binary_details_safe_impl(
    const char_type *input, size_t length, char *output, size_t outlen,
    base64_options options,
    last_chunk_handling_options last_chunk_options) noexcept {
  const bool ignore_garbage =
      (options == base64_options::base64_url_accept_garbage) ||
      (options == base64_options::base64_default_accept_garbage) ||
      (options == base64_options::base64_default_or_url_accept_garbage);
  auto ri = simdutf::scalar::base64::find_end(input, length, options);
  size_t equallocation = ri.equallocation;
  size_t equalsigns = ri.equalsigns;
  length = ri.srclen;
  size_t full_input_length = ri.full_input_length;
  if (length == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      return {INVALID_BASE64_CHARACTER, equallocation, 0};
    }
    return {SUCCESS, full_input_length, 0};
  }
  full_result r = scalar::base64::base64_tail_decode_safe(
      output, outlen, input, length, equalsigns, options, last_chunk_options);
  r = scalar::base64::patch_tail_result(r, 0, 0, equallocation,
                                        full_input_length, last_chunk_options);
  if (!is_partial(last_chunk_options) && r.error == error_code::SUCCESS &&
      equalsigns > 0 && !ignore_garbage) {
    // additional checks
    if ((r.output_count % 3 == 0) ||
        ((r.output_count % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation, r.output_count};
    }
  }

  // When is_partial(last_chunk_options) is true, we must either end with
  // the end of the stream (beyond whitespace) or right after a non-ignorable
  // character or at the very beginning of the stream.
  // See https://tc39.es/proposal-arraybuffer-base64/spec/#sec-frombase64
  if (is_partial(last_chunk_options) && r.error == error_code::SUCCESS &&
      r.input_count < full_input_length) {
    // First check if we can extend the input to the end of the stream
    while (r.input_count < full_input_length &&
           base64_ignorable(*(input + r.input_count), options)) {
      r.input_count++;
    }
    // If we are still not at the end of the stream, then we must backtrack
    // to the last non-ignorable character.
    if (r.input_count < full_input_length) {
      while (r.input_count > 0 &&
             base64_ignorable(*(input + r.input_count - 1), options)) {
        r.input_count--;
      }
    }
  }
  return r;
}

simdutf_warn_unused simdutf_constexpr23 size_t
base64_length_from_binary(size_t length, base64_options options) noexcept {
  // By default, we use padding if we are not using the URL variant.
  // This is check with ((options & base64_url) == 0) which returns true if we
  // are not using the URL variant. However, we also allow 'inversion' of the
  // convention with the base64_reverse_padding option. If the
  // base64_reverse_padding option is set, we use padding if we are using the
  // URL variant, and we omit it if we are not using the URL variant. This is
  // checked with
  // ((options & base64_reverse_padding) == base64_reverse_padding).
  bool use_padding =
      ((options & base64_url) == 0) ^
      ((options & base64_reverse_padding) == base64_reverse_padding);
  if (!use_padding) {
    return length / 3 * 4 + ((length % 3) ? (length % 3) + 1 : 0);
  }
  return (length + 2) / 3 *
         4; // We use padding to make the length a multiple of 4.
}

simdutf_warn_unused simdutf_constexpr23 size_t
base64_length_from_binary_with_lines(size_t length, base64_options options,
                                     size_t line_length) noexcept {
  if (length == 0) {
    return 0;
  }
  size_t base64_length =
      scalar::base64::base64_length_from_binary(length, options);
  if (line_length < 4) {
    line_length = 4;
  }
  size_t lines =
      (base64_length + line_length - 1) / line_length; // number of lines
  return base64_length + lines - 1;
}

// Return the length of the prefix that contains count base64 characters.
// Thus, if count is 3, the function returns the length of the prefix
// that contains 3 base64 characters.
// The function returns (size_t)-1 if there is not enough base64 characters in
// the input.
template <typename char_type>
simdutf_warn_unused size_t prefix_length(size_t count,
                                         simdutf::base64_options options,
                                         const char_type *input,
                                         size_t length) noexcept {
  size_t i = 0;
  while (i < length && is_ignorable(input[i], options)) {
    i++;
  }
  if (count == 0) {
    return i; // duh!
  }
  for (; i < length; i++) {
    if (is_ignorable(input[i], options)) {
      continue;
    }
    // We have a base64 character or a padding character.
    count--;
    if (count == 0) {
      return i + 1;
    }
  }
  simdutf_log_assert(false, "You never get here");

  return -1; // should never happen
}

} // namespace base64
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
