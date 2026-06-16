// NEON (64-bit ARM) reference implementation that removes the four ignorable
// base64 whitespace characters (' ', '\t', '\n', '\r') from the input before
// handing the cleaned-up stream to a whitespace-unaware base64 decoder.
//
// The compaction is done the same way the simdutf NEON base64 decoder removes
// "bad" (ignorable) characters internally: for every 16-byte block we build a
// 16-bit mask of the bytes to drop and shuffle the survivors to the front using
// the `thintable_epi8` / `pshufb_combine_table` tables.  See
// src/arm64/arm_base64.cpp (the `compress` function) and Muła & Lemire, "Base64
// encoding and decoding at almost the speed of a memory copy", SPE 50(2), 2020.
//
// This file is for benchmarking/reference only; production code should use
// simdutf::base64_to_binary, which strips whitespace and decodes in one pass.
#ifndef ARM_BASE64_SPACES_H
#define ARM_BASE64_SPACES_H

#include "simdutf.h" // for SIMDUTF_IS_ARM64

#if SIMDUTF_IS_ARM64

  #include <cstddef>
  #include <cstdint>
  #include <cstring>
  #include <arm_neon.h>

  #ifdef _MSC_VER
    #include <intrin.h> // for _BitScanForward
  #endif

  #include "libbase64.h"

  // Set to 1 to A/B test without the single-whitespace fast path.
  #ifndef ARM_BASE64_SPACES_DISABLE_SINGLE
    #define ARM_BASE64_SPACES_DISABLE_SINGLE 0
  #endif

namespace arm_base64_spaces {

// Maps an 8-bit "drop" mask to a byte shuffle that packs the kept bytes of an
// 8-byte lane to the front. Copied from simdutf (base64_tables.h).
static const uint64_t thintable_epi8[256] = {
    0x0706050403020100, 0x0007060504030201, 0x0007060504030200,
    0x0000070605040302, 0x0007060504030100, 0x0000070605040301,
    0x0000070605040300, 0x0000000706050403, 0x0007060504020100,
    0x0000070605040201, 0x0000070605040200, 0x0000000706050402,
    0x0000070605040100, 0x0000000706050401, 0x0000000706050400,
    0x0000000007060504, 0x0007060503020100, 0x0000070605030201,
    0x0000070605030200, 0x0000000706050302, 0x0000070605030100,
    0x0000000706050301, 0x0000000706050300, 0x0000000007060503,
    0x0000070605020100, 0x0000000706050201, 0x0000000706050200,
    0x0000000007060502, 0x0000000706050100, 0x0000000007060501,
    0x0000000007060500, 0x0000000000070605, 0x0007060403020100,
    0x0000070604030201, 0x0000070604030200, 0x0000000706040302,
    0x0000070604030100, 0x0000000706040301, 0x0000000706040300,
    0x0000000007060403, 0x0000070604020100, 0x0000000706040201,
    0x0000000706040200, 0x0000000007060402, 0x0000000706040100,
    0x0000000007060401, 0x0000000007060400, 0x0000000000070604,
    0x0000070603020100, 0x0000000706030201, 0x0000000706030200,
    0x0000000007060302, 0x0000000706030100, 0x0000000007060301,
    0x0000000007060300, 0x0000000000070603, 0x0000000706020100,
    0x0000000007060201, 0x0000000007060200, 0x0000000000070602,
    0x0000000007060100, 0x0000000000070601, 0x0000000000070600,
    0x0000000000000706, 0x0007050403020100, 0x0000070504030201,
    0x0000070504030200, 0x0000000705040302, 0x0000070504030100,
    0x0000000705040301, 0x0000000705040300, 0x0000000007050403,
    0x0000070504020100, 0x0000000705040201, 0x0000000705040200,
    0x0000000007050402, 0x0000000705040100, 0x0000000007050401,
    0x0000000007050400, 0x0000000000070504, 0x0000070503020100,
    0x0000000705030201, 0x0000000705030200, 0x0000000007050302,
    0x0000000705030100, 0x0000000007050301, 0x0000000007050300,
    0x0000000000070503, 0x0000000705020100, 0x0000000007050201,
    0x0000000007050200, 0x0000000000070502, 0x0000000007050100,
    0x0000000000070501, 0x0000000000070500, 0x0000000000000705,
    0x0000070403020100, 0x0000000704030201, 0x0000000704030200,
    0x0000000007040302, 0x0000000704030100, 0x0000000007040301,
    0x0000000007040300, 0x0000000000070403, 0x0000000704020100,
    0x0000000007040201, 0x0000000007040200, 0x0000000000070402,
    0x0000000007040100, 0x0000000000070401, 0x0000000000070400,
    0x0000000000000704, 0x0000000703020100, 0x0000000007030201,
    0x0000000007030200, 0x0000000000070302, 0x0000000007030100,
    0x0000000000070301, 0x0000000000070300, 0x0000000000000703,
    0x0000000007020100, 0x0000000000070201, 0x0000000000070200,
    0x0000000000000702, 0x0000000000070100, 0x0000000000000701,
    0x0000000000000700, 0x0000000000000007, 0x0006050403020100,
    0x0000060504030201, 0x0000060504030200, 0x0000000605040302,
    0x0000060504030100, 0x0000000605040301, 0x0000000605040300,
    0x0000000006050403, 0x0000060504020100, 0x0000000605040201,
    0x0000000605040200, 0x0000000006050402, 0x0000000605040100,
    0x0000000006050401, 0x0000000006050400, 0x0000000000060504,
    0x0000060503020100, 0x0000000605030201, 0x0000000605030200,
    0x0000000006050302, 0x0000000605030100, 0x0000000006050301,
    0x0000000006050300, 0x0000000000060503, 0x0000000605020100,
    0x0000000006050201, 0x0000000006050200, 0x0000000000060502,
    0x0000000006050100, 0x0000000000060501, 0x0000000000060500,
    0x0000000000000605, 0x0000060403020100, 0x0000000604030201,
    0x0000000604030200, 0x0000000006040302, 0x0000000604030100,
    0x0000000006040301, 0x0000000006040300, 0x0000000000060403,
    0x0000000604020100, 0x0000000006040201, 0x0000000006040200,
    0x0000000000060402, 0x0000000006040100, 0x0000000000060401,
    0x0000000000060400, 0x0000000000000604, 0x0000000603020100,
    0x0000000006030201, 0x0000000006030200, 0x0000000000060302,
    0x0000000006030100, 0x0000000000060301, 0x0000000000060300,
    0x0000000000000603, 0x0000000006020100, 0x0000000000060201,
    0x0000000000060200, 0x0000000000000602, 0x0000000000060100,
    0x0000000000000601, 0x0000000000000600, 0x0000000000000006,
    0x0000050403020100, 0x0000000504030201, 0x0000000504030200,
    0x0000000005040302, 0x0000000504030100, 0x0000000005040301,
    0x0000000005040300, 0x0000000000050403, 0x0000000504020100,
    0x0000000005040201, 0x0000000005040200, 0x0000000000050402,
    0x0000000005040100, 0x0000000000050401, 0x0000000000050400,
    0x0000000000000504, 0x0000000503020100, 0x0000000005030201,
    0x0000000005030200, 0x0000000000050302, 0x0000000005030100,
    0x0000000000050301, 0x0000000000050300, 0x0000000000000503,
    0x0000000005020100, 0x0000000000050201, 0x0000000000050200,
    0x0000000000000502, 0x0000000000050100, 0x0000000000000501,
    0x0000000000000500, 0x0000000000000005, 0x0000000403020100,
    0x0000000004030201, 0x0000000004030200, 0x0000000000040302,
    0x0000000004030100, 0x0000000000040301, 0x0000000000040300,
    0x0000000000000403, 0x0000000004020100, 0x0000000000040201,
    0x0000000000040200, 0x0000000000000402, 0x0000000000040100,
    0x0000000000000401, 0x0000000000000400, 0x0000000000000004,
    0x0000000003020100, 0x0000000000030201, 0x0000000000030200,
    0x0000000000000302, 0x0000000000030100, 0x0000000000000301,
    0x0000000000000300, 0x0000000000000003, 0x0000000000020100,
    0x0000000000000201, 0x0000000000000200, 0x0000000000000002,
    0x0000000000000100, 0x0000000000000001, 0x0000000000000000,
    0x0000000000000000,
};

// Stitches the two 8-byte halves back together after `thintable_epi8`.
static const uint8_t pshufb_combine_table[272] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x01, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

// Twice the number of set bits in an 8-bit value (so it doubles as the byte
// offset into `pshufb_combine_table`).
static const unsigned char BitsSetTable256mul2[256] = {
    0,  2,  2,  4,  2,  4,  4,  6,  2,  4,  4,  6,  4,  6,  6,  8,  2,  4,  4,
    6,  4,  6,  6,  8,  4,  6,  6,  8,  6,  8,  8,  10, 2,  4,  4,  6,  4,  6,
    6,  8,  4,  6,  6,  8,  6,  8,  8,  10, 4,  6,  6,  8,  6,  8,  8,  10, 6,
    8,  8,  10, 8,  10, 10, 12, 2,  4,  4,  6,  4,  6,  6,  8,  4,  6,  6,  8,
    6,  8,  8,  10, 4,  6,  6,  8,  6,  8,  8,  10, 6,  8,  8,  10, 8,  10, 10,
    12, 4,  6,  6,  8,  6,  8,  8,  10, 6,  8,  8,  10, 8,  10, 10, 12, 6,  8,
    8,  10, 8,  10, 10, 12, 8,  10, 10, 12, 10, 12, 12, 14, 2,  4,  4,  6,  4,
    6,  6,  8,  4,  6,  6,  8,  6,  8,  8,  10, 4,  6,  6,  8,  6,  8,  8,  10,
    6,  8,  8,  10, 8,  10, 10, 12, 4,  6,  6,  8,  6,  8,  8,  10, 6,  8,  8,
    10, 8,  10, 10, 12, 6,  8,  8,  10, 8,  10, 10, 12, 8,  10, 10, 12, 10, 12,
    12, 14, 4,  6,  6,  8,  6,  8,  8,  10, 6,  8,  8,  10, 8,  10, 10, 12, 6,
    8,  8,  10, 8,  10, 10, 12, 8,  10, 10, 12, 10, 12, 12, 14, 6,  8,  8,  10,
    8,  10, 10, 12, 8,  10, 10, 12, 10, 12, 12, 14, 8,  10, 10, 12, 10, 12, 12,
    14, 10, 12, 12, 14, 12, 14, 14, 16};

// Build a 16-bit mask whose bit i is set iff byte i is an ignorable base64
// whitespace character (' ', '\t', '\n', '\r').
static inline uint16_t whitespace_mask(uint8x16_t v) {
  uint8x16_t is_ws = vorrq_u8(
      vorrq_u8(vceqq_u8(v, vdupq_n_u8(' ')), vceqq_u8(v, vdupq_n_u8('\t'))),
      vorrq_u8(vceqq_u8(v, vdupq_n_u8('\n')), vceqq_u8(v, vdupq_n_u8('\r'))));
  static const uint8_t bit_table[16] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
                                        0x40, 0x80, 0x01, 0x02, 0x04, 0x08,
                                        0x10, 0x20, 0x40, 0x80};
  uint8x16_t bits = vandq_u8(is_ws, vld1q_u8(bit_table));
  // Horizontally add the low and high halves into the two mask bytes.
  uint8_t lo = vaddv_u8(vget_low_u8(bits));
  uint8_t hi = vaddv_u8(vget_high_u8(bits));
  return uint16_t(lo | (uint16_t(hi) << 8));
}

// Pack the bytes of `data` whose mask bit is 0 to the front of `output`,
// writing a full 16-byte vector (only the leading kept bytes are meaningful).
// Mirrors simdutf's NEON `compress`.
static inline void compress(uint8x16_t data, uint16_t mask, char *output) {
  if (mask == 0) {
    vst1q_u8((uint8_t *)output, data);
    return;
  }
  uint8_t mask1 = uint8_t(mask);      // least significant 8 bits
  uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
  uint64x2_t compactmasku64 = {thintable_epi8[mask1], thintable_epi8[mask2]};
  uint8x16_t compactmask = vreinterpretq_u8_u64(compactmasku64);
  static const uint8_t off_arr[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                      8, 8, 8, 8, 8, 8, 8, 8};
  compactmask = vaddq_u8(compactmask, vld1q_u8(off_arr));
  uint8x16_t pruned = vqtbl1q_u8(data, compactmask);

  int pop1 = BitsSetTable256mul2[mask1];
  // Load the mask that writes the first pop1/2 kept bytes from the low half and
  // then fills in the kept bytes from the high half right after them.
  compactmask = vld1q_u8(pshufb_combine_table + pop1 * 8);
  uint8x16_t answer = vqtbl1q_u8(pruned, compactmask);
  vst1q_u8((uint8_t *)output, answer);
}

// Count trailing zeroes. Undefined when mask is zero.
static inline int ctz16(uint16_t mask) {
  #ifdef _MSC_VER
  unsigned long ret;
  _BitScanForward(&ret, (unsigned long)mask);
  return (int)ret;
  #else
  return __builtin_ctz(mask);
  #endif
}

// Fast path for a block with exactly one byte to drop (at index `pos`): keep
// bytes [0, pos) in place and shift bytes (pos, 16) left by one with a single
// table lookup. Writes a full 16-byte vector; only the leading 15 bytes are
// meaningful. Mirrors simdutf's NEON `compress_block_single`.
static inline void compress_single(uint8x16_t data, int pos, char *output) {
  static const uint8_t iota_arr[16] = {0, 1, 2,  3,  4,  5,  6,  7,
                                       8, 9, 10, 11, 12, 13, 14, 15};
  const uint8x16_t iota = vld1q_u8(iota_arr);
  // ge is 0xFF for lanes i >= pos; subtracting it adds one to those indices,
  // so the byte at `pos` is skipped and everything after it slides left.
  const uint8x16_t ge = vcgeq_u8(iota, vdupq_n_u8(uint8_t(pos)));
  const uint8x16_t sh = vsubq_u8(iota, ge);
  vst1q_u8((uint8_t *)output, vqtbl1q_u8(data, sh));
}

// Remove all ignorable whitespace from [src, src+srclen) into `dst`, returning
// the number of bytes written. Each 16-byte block writes a full vector even
// when some bytes are dropped, but a block starting at offset i only ever
// stores up to offset i+16 <= srclen, so `dst` only needs srclen bytes.
static inline size_t remove_spaces(const char *src, size_t srclen, char *dst) {
  char *const dstinit = dst;
  size_t i = 0;
  for (; i + 16 <= srclen; i += 16) {
    uint8x16_t v = vld1q_u8(reinterpret_cast<const uint8_t *>(src) + i);
    uint16_t mask = whitespace_mask(v);
    if (mask == 0) {
      // No whitespace: keep the whole block.
      vst1q_u8(reinterpret_cast<uint8_t *>(dst), v);
      dst += 16;
    } else if (!ARM_BASE64_SPACES_DISABLE_SINGLE &&
               (mask & uint16_t(mask - 1)) == 0) {
      // Exactly one whitespace byte: cheap single-shift compaction.
      compress_single(v, ctz16(mask), dst);
      dst += 15;
    } else {
      compress(v, mask, dst);
      // popcount(mask) == (BitsSetTable256mul2[lo] + BitsSetTable256mul2[hi])/2
      int dropped = (BitsSetTable256mul2[uint8_t(mask)] +
                     BitsSetTable256mul2[uint8_t(mask >> 8)]) /
                    2;
      dst += 16 - dropped;
    }
  }
  // Scalar tail for the final partial block.
  for (; i < srclen; i++) {
    char c = src[i];
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      *dst++ = c;
    }
  }
  return size_t(dst - dstinit);
}

// Strip whitespace with NEON, then decode the cleaned base64 with a
// whitespace-unaware decoder (libbase64). `scratch` must hold at least
// srclen + 16 bytes; `out` receives the decoded binary.
static inline bool space_decode(const char *src, size_t srclen, char *scratch,
                                char *out, size_t *outlen) {
  size_t cleaned = remove_spaces(src, srclen, scratch);
  return base64_decode(scratch, cleaned, out, outlen, 0) == 1;
}

// Same as space_decode, but compacts whitespace in-place: src is copied into
// `buf` (size >= srclen) and then compacted within `buf`, avoiding a separate
// scratch buffer. `out` receives the decoded binary.
static inline bool space_decode_inplace(const char *src, size_t srclen,
                                        char *buf, char *out, size_t *outlen) {
  memcpy(buf, src, srclen);
  size_t cleaned = remove_spaces(buf, srclen, buf);
  return base64_decode(buf, cleaned, out, outlen, 0) == 1;
}

} // namespace arm_base64_spaces

#endif // SIMDUTF_IS_ARM64
#endif // ARM_BASE64_SPACES_H
