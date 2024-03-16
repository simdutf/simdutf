#ifndef SIMDUTF_BASE64_H
#define SIMDUTF_BASE64_H

#include <cstddef>
#include <cstdint>
#include <cstring>
namespace simdutf {
namespace scalar {
namespace {
namespace base64 {

// Returns true upon success. The destination buffer must be large enough and is
// incremented by the number of bytes written and src is incremented by the number of bytes read.
// This functions assumes that the padding (=) has been removed.
result base64_tail_decode(char *dst, const char *src, size_t length) {
  const char *srcend = src + length;
  const char *srcinit = src;
  const char *dstinit = dst;

  uint32_t x;
  size_t idx;
  uint8_t buffer[4];
  while (true) {
    while (src + 4 <= srcend &&
           (x = tables::base64::d0[uint8_t(src[0])] | tables::base64::d1[uint8_t(src[1])] |
                tables::base64::d2[uint8_t(src[2])] | tables::base64::d3[uint8_t(src[3])]) < 0x01FFFFFF) {
      if(match_system(endianness::BIG)) {
        x = scalar::utf32::swap_bytes(x);
      }
      std::memcpy(dst, &x, 3); // optimization opportunity: copy 4 bytes
      dst += 3;
      src += 4;
    }
    idx = 0;
    // we need at least four characters.
    while (idx < 4 && src < srcend) {
      char c = *src;
      uint8_t code = tables::base64::to_base64_value[uint8_t(c)];
      buffer[idx] = uint8_t(code);
      if (code <= 63) {
        idx++;
      } else if (code > 64) {
        return {INVALID_BASE64_CHARACTER, size_t(src - srcinit)};
      }
      src++;
    }
    if (idx != 4) {
      if (idx == 2) {
        uint32_t triple =
            (uint32_t(buffer[0]) << 3 * 6) + (uint32_t(buffer[1]) << 2 * 6);
        if(match_system(endianness::BIG)) {
          triple <<= 8;
          std::memcpy(dst, &triple, 1);
        } else {
          triple = scalar::utf32::swap_bytes(triple);
          triple >>= 8;
          std::memcpy(dst, &triple, 1);
        }
        dst += 1;

      } else if (idx == 3) {
        uint32_t triple = (uint32_t(buffer[0]) << 3 * 6) +
                          (uint32_t(buffer[1]) << 2 * 6) +
                          (uint32_t(buffer[2]) << 1 * 6);
        if(match_system(endianness::BIG)) {
          triple <<= 8;
          std::memcpy(dst, &triple, 2);
        } else {
          triple = scalar::utf32::swap_bytes(triple);
          triple >>= 8;
          std::memcpy(dst, &triple, 2);
        }
        dst += 2;
      } else if (idx == 1) {
        return {BASE64_INPUT_REMAINDER, size_t(dst - dstinit)};
      }
      return {SUCCESS, size_t(dst - dstinit)};
    }

    uint32_t triple =
        (uint32_t(buffer[0]) << 3 * 6) + (uint32_t(buffer[1]) << 2 * 6) +
        (uint32_t(buffer[2]) << 1 * 6) + (uint32_t(buffer[3]) << 0 * 6);
    if(match_system(endianness::BIG)) {
      triple <<= 8;
      std::memcpy(dst, &triple, 3);
    } else {
      triple = scalar::utf32::swap_bytes(triple);
      triple >>= 8;
      std::memcpy(dst, &triple, 3);
    }
    dst += 3;
  }
}

// Returns the number of bytes written. The destination buffer must be large
// enough. It will add padding (=) if needed.
size_t tail_encode_base64(char *dst, const char *src, size_t srclen) {
  char *out = dst;
  size_t i = 0;
  uint8_t t1, t2, t3;
  for (; i + 2 < srclen; i += 3) {
    t1 = (uint8_t)src[i];
    t2 = (uint8_t)src[i + 1];
    t3 = (uint8_t)src[i + 2];
    *out++ = tables::base64::e0[t1];
    *out++ = tables::base64::e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
    *out++ = tables::base64::e1[((t2 & 0x0F) << 2) | ((t3 >> 6) & 0x03)];
    *out++ = tables::base64::e2[t3];
  }
  switch (srclen - i) {
  case 0:
    break;
  case 1:
    t1 = (uint8_t)src[i];
    *out++ = tables::base64::e0[t1];
    *out++ = tables::base64::e1[(t1 & 0x03) << 4];
    *out++ = '=';
    *out++ = '=';
    break;
  default: /* case 2 */
    t1 = (uint8_t)src[i];
    t2 = (uint8_t)src[i + 1];
    *out++ = tables::base64::e0[t1];
    *out++ = tables::base64::e1[((t1 & 0x03) << 4) | ((t2 >> 4) & 0x0F)];
    *out++ = tables::base64::e2[(t2 & 0x0F) << 2];
    *out++ = '=';
  }
  return (size_t)(out - dst);
}

simdutf_warn_unused size_t maximal_binary_length_from_base64(const char * input, size_t length) noexcept {
  // We follow https://infra.spec.whatwg.org/#forgiving-base64-decode
  size_t padding = 0;
  if(length > 0) {
    if(input[length - 1] == '=') {
      padding++;
      if(length > 1 && input[length - 2] == '=') {
        padding++;
      }
    }
  }
  size_t actual_length = length - padding;
  if(actual_length % 4 == 0) {
    return actual_length / 4 * 3;
  }
  // if we have a valid input, then the remainder must be 2 or 3 adding one or two extra bytes.
  return  actual_length / 4 * 3 + (actual_length %4)  - 1;
}

simdutf_warn_unused simdutf_really_inline result base64_to_binary(const char * input, size_t length, char* output) noexcept {
  if(length > 0 && input[length - 1] == '=') {
    length -= 1;
    if(length > 0 && input[length - 1] == '=') {
      length -= 1;
    }
  }
  if(length == 0) {
    return {SUCCESS, 0};
  }
  return base64_tail_decode(output, input, length);
}

simdutf_warn_unused size_t base64_length_from_binary(size_t length) noexcept {
  return (length + 2)/3 * 4; // We use padding to make the length a multiple of 4.
}

simdutf_really_inline size_t binary_to_base64(const char * input, size_t length, char* output) noexcept {
  return tail_encode_base64(output, input, length);
}
} // namespace base64
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
