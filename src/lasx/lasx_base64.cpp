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

// Translate 6-bit values (0..63) to the base64 alphabet using two 32-entry
// (per 128-bit lane) shuffles and a select. xvshuf.b only uses the low five
// bits of each index, so values 32..63 pick the right entry from
// (tbl2, tbl3) without any adjustment.
simdutf_really_inline __m256i lookup_base64(__m256i indices, __m256i tbl0,
                                            __m256i tbl1, __m256i tbl2,
                                            __m256i tbl3) {
  const __m256i lo = __lasx_xvshuf_b(tbl1, tbl0, indices);
  const __m256i hi = __lasx_xvshuf_b(tbl3, tbl2, indices);
  const __m256i is_lo = __lasx_xvslei_bu(indices, 31);
  return __lasx_xvbitsel_v(hi, lo, is_lo);
}

// Returns input with a line feed inserted at position K (0..15); the last
// byte of the input is dropped and must be stored separately.
simdutf_really_inline __m128i insert_line_feed16(__m128i input, size_t K) {
  static const uint8_t shuffle_masks[16][16] = {
      {15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
      {0, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
      {0, 1, 15, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
      {0, 1, 2, 15, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 15, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 15, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 15, 6, 7, 8, 9, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 15, 7, 8, 9, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 15, 8, 9, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 15, 9, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 10, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 11, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 15, 12, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 13, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 14},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
  input = __lsx_vinsgr2vr_b(input, '\n', 15);
  const __m128i mask = __lsx_vld(shuffle_masks[K], 0);
  return __lsx_vshuf_b(input, input, mask);
}

// Stores the 32 bytes of `data` at `out` with a line feed inserted at
// position K (0..31), writing 33 bytes in total.
simdutf_really_inline void store_with_line_feed32(uint8_t *out, __m256i data,
                                                  size_t K) {
  // out[1..32] receives data[0..31]; we then overwrite the 128-bit half
  // that contains the line feed (and, when the line feed is in the upper
  // half, restore the lower half).
  __lasx_xvst(data, out, 1);
  const __m128i lo = lasx_extracti128_lo(data);
  if (K < 16) {
    __lsx_vst(insert_line_feed16(lo, K), out, 0);
  } else {
    const __m128i hi = lasx_extracti128_hi(data);
    __lsx_vst(lo, out, 0);
    __lsx_vst(insert_line_feed16(hi, K - 16), out, 16);
  }
}

template <bool isbase64url, bool use_lines>
size_t encode_base64_impl(char *dst, const char *src, size_t srclen,
                          base64_options options,
                          size_t line_length = simdutf::default_line_length) {
  size_t offset = 0;
  if (line_length < 4) {
    line_length = 4; // We do not support line_length less than 4
  }
  // credit: Wojciech Muła
  // SSE (lookup: pshufb improved unrolled)
  const uint8_t *input = (const uint8_t *)src;
  static const char *lookup_tbl =
      isbase64url
          ? "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
          : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  uint8_t *out = (uint8_t *)dst;

  v32u8 shuf, shuf_overlap;
  __m256i v_fc0fc00, v_3f03f0, shift_r, shift_l, base64_tbl0, base64_tbl1,
      base64_tbl2, base64_tbl3;
  if (srclen >= 28) {
    shuf = v32u8{1, 0, 2, 1, 4, 3, 5, 4, 7, 6, 8, 7, 10, 9, 11, 10,
                 1, 0, 2, 1, 4, 3, 5, 4, 7, 6, 8, 7, 10, 9, 11, 10};
    // Same as shuf, but for a 32-byte block loaded 4 bytes before the
    // start of a 24-byte group: the low 128-bit lane then holds bytes
    // 0..11 of the group at positions 4..15 and the high lane holds bytes
    // 12..23 at positions 0..11.
    shuf_overlap =
        v32u8{5, 4, 6, 5, 8, 7, 9, 8, 11, 10, 12, 11, 14, 13, 15, 14,
              1, 0, 2, 1, 4, 3, 5, 4, 7,  6,  8,  7,  10, 9,  11, 10};

    v_fc0fc00 = __lasx_xvreplgr2vr_w(uint32_t(0x0fc0fc00));
    v_3f03f0 = __lasx_xvreplgr2vr_w(uint32_t(0x003f03f0));
    shift_r = __lasx_xvreplgr2vr_w(uint32_t(0x0006000a));
    shift_l = __lasx_xvreplgr2vr_w(uint32_t(0x00080004));
    base64_tbl0 = ____m256i(__lsx_vld(lookup_tbl, 0));
    base64_tbl1 = ____m256i(__lsx_vld(lookup_tbl, 16));
    base64_tbl2 = ____m256i(__lsx_vld(lookup_tbl, 32));
    base64_tbl3 = ____m256i(__lsx_vld(lookup_tbl, 48));
  }
  size_t i = 0;
  for (; i + 100 <= srclen; i += 96) {
    // The first 24-byte group cannot be loaded 4 bytes early (it might
    // read before the start of the buffer), so it uses two 16-byte loads.
    // The next three groups use a single overlapping 32-byte load each; the
    // last one ends at input + i + 100 <= input + srclen.
    __m128i in0_lo =
        __lsx_vld(reinterpret_cast<const __m128i *>(input + i), 4 * 3 * 0);
    __m128i in0_hi =
        __lsx_vld(reinterpret_cast<const __m128i *>(input + i), 4 * 3 * 1);
    __m256i in0 = lasx_set_q(in0_hi, in0_lo);
    __m256i in1 =
        __lasx_xvld(reinterpret_cast<const __m256i *>(input + i), 24 - 4);
    __m256i in2 =
        __lasx_xvld(reinterpret_cast<const __m256i *>(input + i), 48 - 4);
    __m256i in3 =
        __lasx_xvld(reinterpret_cast<const __m256i *>(input + i), 72 - 4);

    in0 = __lasx_xvshuf_b(in0, in0, (__m256i)shuf);
    in1 = __lasx_xvshuf_b(in1, in1, (__m256i)shuf_overlap);
    in2 = __lasx_xvshuf_b(in2, in2, (__m256i)shuf_overlap);
    in3 = __lasx_xvshuf_b(in3, in3, (__m256i)shuf_overlap);

    __m256i t0_0 = __lasx_xvand_v(in0, v_fc0fc00);
    __m256i t0_1 = __lasx_xvand_v(in1, v_fc0fc00);
    __m256i t0_2 = __lasx_xvand_v(in2, v_fc0fc00);
    __m256i t0_3 = __lasx_xvand_v(in3, v_fc0fc00);

    __m256i t1_0 = __lasx_xvsrl_h(t0_0, shift_r);
    __m256i t1_1 = __lasx_xvsrl_h(t0_1, shift_r);
    __m256i t1_2 = __lasx_xvsrl_h(t0_2, shift_r);
    __m256i t1_3 = __lasx_xvsrl_h(t0_3, shift_r);

    __m256i t2_0 = __lasx_xvand_v(in0, v_3f03f0);
    __m256i t2_1 = __lasx_xvand_v(in1, v_3f03f0);
    __m256i t2_2 = __lasx_xvand_v(in2, v_3f03f0);
    __m256i t2_3 = __lasx_xvand_v(in3, v_3f03f0);

    __m256i t3_0 = __lasx_xvsll_h(t2_0, shift_l);
    __m256i t3_1 = __lasx_xvsll_h(t2_1, shift_l);
    __m256i t3_2 = __lasx_xvsll_h(t2_2, shift_l);
    __m256i t3_3 = __lasx_xvsll_h(t2_3, shift_l);

    __m256i input0 = __lasx_xvor_v(t1_0, t3_0);
    __m256i input1 = __lasx_xvor_v(t1_1, t3_1);
    __m256i input2 = __lasx_xvor_v(t1_2, t3_2);
    __m256i input3 = __lasx_xvor_v(t1_3, t3_3);

    const __m256i r0 = lookup_base64(input0, base64_tbl0, base64_tbl1,
                                     base64_tbl2, base64_tbl3);
    const __m256i r1 = lookup_base64(input1, base64_tbl0, base64_tbl1,
                                     base64_tbl2, base64_tbl3);
    const __m256i r2 = lookup_base64(input2, base64_tbl0, base64_tbl1,
                                     base64_tbl2, base64_tbl3);
    const __m256i r3 = lookup_base64(input3, base64_tbl0, base64_tbl1,
                                     base64_tbl2, base64_tbl3);

    if (use_lines) {
      if (line_length >= 32) { // fast path
        if (offset + 32 > line_length) {
          size_t location_end = line_length - offset;
          store_with_line_feed32(out, r0, location_end);
          offset = 32 - location_end;
          out += 32 + 1;
        } else {
          __lasx_xvst(r0, out, 0);
          offset += 32;
          out += 32;
        }
        if (offset + 32 > line_length) {
          size_t location_end = line_length - offset;
          store_with_line_feed32(out, r1, location_end);
          offset = 32 - location_end;
          out += 32 + 1;
        } else {
          __lasx_xvst(r1, out, 0);
          offset += 32;
          out += 32;
        }
        if (offset + 32 > line_length) {
          size_t location_end = line_length - offset;
          store_with_line_feed32(out, r2, location_end);
          offset = 32 - location_end;
          out += 32 + 1;
        } else {
          __lasx_xvst(r2, out, 0);
          offset += 32;
          out += 32;
        }
        if (offset + 32 > line_length) {
          size_t location_end = line_length - offset;
          store_with_line_feed32(out, r3, location_end);
          offset = 32 - location_end;
          out += 32 + 1;
        } else {
          __lasx_xvst(r3, out, 0);
          offset += 32;
          out += 32;
        }
      } else { // slow path
        // could be optimized
        alignas(32) uint8_t buffer[128];
        __lasx_xvst(r0, buffer, 0);
        __lasx_xvst(r1, buffer, 32);
        __lasx_xvst(r2, buffer, 64);
        __lasx_xvst(r3, buffer, 96);
        size_t out_pos = 0;
        size_t local_offset = offset;
        for (size_t j = 0; j < 128;) {
          if (local_offset == line_length) {
            out[out_pos++] = '\n';
            local_offset = 0;
          }
          out[out_pos++] = buffer[j++];
          local_offset++;
        }
        offset = local_offset;
        out += out_pos;
      }
    } else {
      __lasx_xvst(r0, out, 0);
      __lasx_xvst(r1, out, 32);
      __lasx_xvst(r2, out, 64);
      __lasx_xvst(r3, out, 96);
      out += 128;
    }
  }
  for (; i + 28 <= srclen; i += 24) {

    __m128i in_lo = __lsx_vld(reinterpret_cast<const __m128i *>(input + i), 0);
    __m128i in_hi =
        __lsx_vld(reinterpret_cast<const __m128i *>(input + i), 4 * 3 * 1);

    __m256i in = lasx_set_q(in_hi, in_lo);

    // bytes from groups A, B and C are needed in separate 32-bit lanes
    // in = [DDDD|CCCC|BBBB|AAAA]
    //
    //      an input triplet has layout
    //      [????????|ccdddddd|bbbbcccc|aaaaaabb]
    //        byte 3   byte 2   byte 1   byte 0    -- byte 3 comes from the next
    //        triplet
    //
    //      shuffling changes the order of bytes: 1, 0, 2, 1
    //      [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]
    //           ^^^^ ^^^^^^^^ ^^^^^^^^ ^^^^
    //                  processed bits
    in = __lasx_xvshuf_b(in, in, (__m256i)shuf);

    // unpacking
    // t0    = [0000cccc|cc000000|aaaaaa00|00000000]
    __m256i t0 = __lasx_xvand_v(in, v_fc0fc00);
    // t1    = [00000000|00cccccc|00000000|00aaaaaa]
    //          ((c >> 6),  (a >> 10))
    __m256i t1 = __lasx_xvsrl_h(t0, shift_r);

    // t2    = [00000000|00dddddd|000000bb|bbbb0000]
    __m256i t2 = __lasx_xvand_v(in, v_3f03f0);
    // t3    = [00dddddd|00000000|00bbbbbb|00000000]
    //          ((d << 8), (b << 4))
    __m256i t3 = __lasx_xvsll_h(t2, shift_l);

    // res   = [00dddddd|00cccccc|00bbbbbb|00aaaaaa] = t1 | t3
    __m256i indices = __lasx_xvor_v(t1, t3);

    const __m256i result = lookup_base64(indices, base64_tbl0, base64_tbl1,
                                         base64_tbl2, base64_tbl3);

    if (use_lines) {
      if (line_length >= 32) { // fast path
        if (offset + 32 > line_length) {
          size_t location_end = line_length - offset;
          store_with_line_feed32(out, result, location_end);
          offset = 32 - location_end;
          out += 32 + 1;
        } else {
          __lasx_xvst(result, out, 0);
          offset += 32;
          out += 32;
        }
      } else { // slow path
        // could be optimized
        alignas(32) uint8_t buffer[32];
        __lasx_xvst(result, buffer, 0);
        size_t out_pos = 0;
        size_t local_offset = offset;
        for (size_t j = 0; j < 32;) {
          if (local_offset == line_length) {
            out[out_pos++] = '\n';
            local_offset = 0;
          }
          out[out_pos++] = buffer[j++];
          local_offset++;
        }
        offset = local_offset;
        out += out_pos;
      }
    } else {
      __lasx_xvst(result, out, 0);
      out += 32;
    }
  }

  return ((char *)out - (char *)dst) +
         scalar::base64::tail_encode_base64_impl<use_lines>(
             (char *)out, src + i, srclen - i, options, line_length, offset);
}

template <bool isbase64url>
size_t encode_base64(char *dst, const char *src, size_t srclen,
                     base64_options options) {
  return encode_base64_impl<isbase64url, false>(dst, src, srclen, options);
}

static inline void compress(__m128i data, uint16_t mask, char *output) {
  if (mask == 0) {
    __lsx_vst(data, reinterpret_cast<__m128i *>(output), 0);
    return;
  }
  // this particular implementation was inspired by work done by @animetosho
  // we do it in two steps, first 8 bytes and then second 8 bytes
  uint8_t mask1 = uint8_t(mask);      // least significant 8 bits
  uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
  // next line just loads the 64-bit values thintable_epi8[mask1] and
  // thintable_epi8[mask2] into a 128-bit register, using only
  // two instructions on most compilers.

  v2u64 shufmask = {tables::base64::thintable_epi8[mask1],
                    tables::base64::thintable_epi8[mask2]};

  // we increment by 0x08 the second half of the mask
  const v4u32 hi = {0, 0, 0x08080808, 0x08080808};
  __m128i shufmask1 = __lsx_vadd_b((__m128i)shufmask, (__m128i)hi);

  // this is the version "nearly pruned"
  __m128i pruned = __lsx_vshuf_b(data, data, shufmask1);
  // we still need to put the two halves together.
  // we compute the popcount of the first half:
  int pop1 = tables::base64::BitsSetTable256mul2[mask1];
  // then load the corresponding mask, what it does is to write
  // only the first pop1 bytes from the first 8 bytes, and then
  // it fills in with the bytes from the second 8 bytes + some filling
  // at the end.
  __m128i compactmask =
      __lsx_vld(reinterpret_cast<const __m128i *>(
                    tables::base64::pshufb_combine_table + pop1 * 8),
                0);
  __m128i answer = __lsx_vshuf_b(pruned, pruned, compactmask);

  __lsx_vst(answer, reinterpret_cast<__m128i *>(output), 0);
}

struct block64 {
  __m256i chunks[2];
};

template <bool base64_url, bool default_or_url>
static inline uint32_t to_base64_mask(__m256i *src, bool *error) {
  __m256i ascii_space_tbl =
      ____m256i((__m128i)v16u8{0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                               0x9, 0xa, 0x0, 0xc, 0xd, 0x0, 0x0});
  // credit: aqrit
  __m256i delta_asso;
  if (default_or_url) {
    delta_asso =
        ____m256i((__m128i)v16u8{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0,
                                 0x0, 0x0, 0x0, 0x0, 0x11, 0x0, 0x16});
  } else {
    delta_asso =
        ____m256i((__m128i)v16u8{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0,
                                 0x0, 0x0, 0x0, 0x0, 0xF, 0x0, 0xF});
  }
  __m256i delta_values;
  if (default_or_url) {
    delta_values = ____m256i(
        (__m128i)v16i8{int8_t(0xBF), int8_t(0xE0), int8_t(0xB9), int8_t(0x13),
                       int8_t(0x04), int8_t(0xBF), int8_t(0xBF), int8_t(0xB9),
                       int8_t(0xB9), int8_t(0x00), int8_t(0xFF), int8_t(0x11),
                       int8_t(0xFF), int8_t(0xBF), int8_t(0x10), int8_t(0xB9)});
  } else if (base64_url) {
    delta_values = ____m256i(
        (__m128i)v16i8{int8_t(0x00), int8_t(0x00), int8_t(0x00), int8_t(0x13),
                       int8_t(0x04), int8_t(0xBF), int8_t(0xBF), int8_t(0xB9),
                       int8_t(0xB9), int8_t(0x00), int8_t(0x11), int8_t(0xC3),
                       int8_t(0xBF), int8_t(0xE0), int8_t(0xB9), int8_t(0xB9)});
  } else {
    delta_values = ____m256i(
        (__m128i)v16i8{int8_t(0x00), int8_t(0x00), int8_t(0x00), int8_t(0x13),
                       int8_t(0x04), int8_t(0xBF), int8_t(0xBF), int8_t(0xB9),
                       int8_t(0xB9), int8_t(0x00), int8_t(0x10), int8_t(0xC3),
                       int8_t(0xBF), int8_t(0xBF), int8_t(0xB9), int8_t(0xB9)});
  }

  __m256i check_asso;
  if (default_or_url) {
    check_asso = ____m256i((__m128i)v16u8{0x0D, 0x01, 0x01, 0x01, 0x01, 0x01,
                                          0x01, 0x01, 0x01, 0x01, 0x03, 0x07,
                                          0x0B, 0x0E, 0x0B, 0x06});

  } else if (base64_url) {
    check_asso = ____m256i((__m128i)v16u8{0x0D, 0x01, 0x01, 0x01, 0x01, 0x01,
                                          0x01, 0x01, 0x01, 0x01, 0x03, 0x07,
                                          0x0B, 0x06, 0x0B, 0x12});
  } else {
    check_asso = ____m256i((__m128i)v16u8{0x0D, 0x01, 0x01, 0x01, 0x01, 0x01,
                                          0x01, 0x01, 0x01, 0x01, 0x03, 0x07,
                                          0x0B, 0x0B, 0x0B, 0x0F});
  }

  __m256i check_values;
  if (default_or_url) {

    check_values = ____m256i(
        (__m128i)v16i8{int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0x80),
                       int8_t(0xCF), int8_t(0xBF), int8_t(0xD5), int8_t(0xA6),
                       int8_t(0xB5), int8_t(0xA1), int8_t(0x00), int8_t(0x80),
                       int8_t(0x00), int8_t(0x80), int8_t(0x00), int8_t(0x80)});
  } else if (base64_url) {
    check_values = ____m256i(
        (__m128i)v16i8{int8_t(0x0), int8_t(0x80), int8_t(0x80), int8_t(0x80),
                       int8_t(0xCF), int8_t(0xBF), int8_t(0xD3), int8_t(0xA6),
                       int8_t(0xB5), int8_t(0x86), int8_t(0xD0), int8_t(0x80),
                       int8_t(0xB0), int8_t(0x80), int8_t(0x0), int8_t(0x0)});
  } else {
    check_values = ____m256i(
        (__m128i)v16i8{int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0x80),
                       int8_t(0xCF), int8_t(0xBF), int8_t(0xD5), int8_t(0xA6),
                       int8_t(0xB5), int8_t(0x86), int8_t(0xD1), int8_t(0x80),
                       int8_t(0xB1), int8_t(0x80), int8_t(0x91), int8_t(0x80)});
  }

  __m256i shifted = __lasx_xvsrli_b(*src, 3);
  __m256i asso_index = __lasx_xvand_v(*src, __lasx_xvldi(0xF));
  __m256i delta_hash = __lasx_xvavgr_bu(
      __lasx_xvshuf_b(delta_asso, delta_asso, asso_index), shifted);
  __m256i check_hash = __lasx_xvavgr_bu(
      __lasx_xvshuf_b(check_asso, check_asso, asso_index), shifted);

  __m256i out = __lasx_xvsadd_b(
      __lasx_xvshuf_b(delta_values, delta_values, delta_hash), *src);
  __m256i chk = __lasx_xvsadd_b(
      __lasx_xvshuf_b(check_values, check_values, check_hash), *src);
  __m256i chk_ltz = __lasx_xvmskltz_b(chk);
  unsigned int mask = __lasx_xvpickve2gr_wu(chk_ltz, 0);
  mask = mask | (__lsx_vpickve2gr_hu(lasx_extracti128_hi(chk_ltz), 0) << 16);
  if (mask) {
    __m256i ascii_space = __lasx_xvseq_b(
        __lasx_xvshuf_b(ascii_space_tbl, ascii_space_tbl, asso_index), *src);
    __m256i ascii_space_ltz = __lasx_xvmskltz_b(ascii_space);
    unsigned int ascii_space_mask = __lasx_xvpickve2gr_wu(ascii_space_ltz, 0);
    ascii_space_mask =
        ascii_space_mask |
        (__lsx_vpickve2gr_hu(lasx_extracti128_hi(ascii_space_ltz), 0) << 16);
    *error |= (mask != ascii_space_mask);
  }

  *src = out;
  return (uint32_t)mask;
}

template <bool base64_url, bool default_or_url>
static inline uint64_t to_base64_mask(block64 *b, bool *error) {
  *error = 0;
  uint64_t m0 =
      to_base64_mask<base64_url, default_or_url>(&b->chunks[0], error);
  uint64_t m1 =
      to_base64_mask<base64_url, default_or_url>(&b->chunks[1], error);
  return m0 | (m1 << 32);
}

static inline void copy_block(block64 *b, char *output) {
  __lasx_xvst(b->chunks[0], reinterpret_cast<__m256i *>(output), 0);
  __lasx_xvst(b->chunks[1], reinterpret_cast<__m256i *>(output), 32);
}

static inline uint64_t compress_block(block64 *b, uint64_t mask, char *output) {
  uint64_t nmask = ~mask;
  uint64_t count =
      __lsx_vpickve2gr_d(__lsx_vpcnt_h(__lsx_vreplgr2vr_d(nmask)), 0);
  uint16_t *count_ptr = (uint16_t *)&count;
  compress(lasx_extracti128_lo(b->chunks[0]), uint16_t(mask), output);
  compress(lasx_extracti128_hi(b->chunks[0]), uint16_t(mask >> 16),
           output + count_ptr[0]);
  compress(lasx_extracti128_lo(b->chunks[1]), uint16_t(mask >> 32),
           output + count_ptr[0] + count_ptr[1]);
  compress(lasx_extracti128_hi(b->chunks[1]), uint16_t(mask >> 48),
           output + count_ptr[0] + count_ptr[1] + count_ptr[2]);
  return count_ones(nmask);
}

template <typename T> bool is_power_of_two(T x) { return (x & (x - 1)) == 0; }

inline size_t compress_block_single(block64 *b, uint64_t mask, char *output) {
  const size_t pos64 = trailing_zeroes(mask);
  const int8_t pos = pos64 & 0xf;

  // Predefine the index vector
  const v16u8 v1 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

  switch (pos64 >> 4) {
  case 0b00: {
    const __m128i lane0 = lasx_extracti128_lo(b->chunks[0]);
    const __m128i lane1 = lasx_extracti128_hi(b->chunks[0]);

    const __m128i v0 = __lsx_vreplgr2vr_b((uint8_t)(pos - 1));
    const __m128i v2 = __lsx_vslt_b(v0, (__m128i)v1); // v1 > v0
    const __m128i sh = __lsx_vsub_b((__m128i)v1, v2);
    const __m128i compressed = __lsx_vshuf_b(lane0, lane0, sh);

    __lsx_vst(compressed, reinterpret_cast<__m128i *>(output + 0 * 16), 0);
    __lsx_vst(lane1, reinterpret_cast<__m128i *>(output + 1 * 16 - 1), 0);
    __lasx_xvst(b->chunks[1], reinterpret_cast<__m256i *>(output + 2 * 16 - 1),
                0);
  } break;
  case 0b01: {
    const __m128i lane0 = lasx_extracti128_lo(b->chunks[0]);
    const __m128i lane1 = lasx_extracti128_hi(b->chunks[0]);
    __lsx_vst(lane0, reinterpret_cast<__m128i *>(output + 0 * 16), 0);

    const __m128i v0 = __lsx_vreplgr2vr_b((uint8_t)(pos - 1));
    const __m128i v2 = __lsx_vslt_b(v0, (__m128i)v1);
    const __m128i sh = __lsx_vsub_b((__m128i)v1, v2);
    const __m128i compressed = __lsx_vshuf_b(lane1, lane1, sh);

    __lsx_vst(compressed, reinterpret_cast<__m128i *>(output + 1 * 16), 0);
    __lasx_xvst(b->chunks[1], reinterpret_cast<__m256i *>(output + 2 * 16 - 1),
                0);
  } break;
  case 0b10: {
    __lasx_xvst(b->chunks[0], reinterpret_cast<__m256i *>(output + 0 * 16), 0);

    const __m128i lane2 = lasx_extracti128_lo(b->chunks[1]);
    const __m128i lane3 = lasx_extracti128_hi(b->chunks[1]);

    const __m128i v0 = __lsx_vreplgr2vr_b((uint8_t)(pos - 1));
    const __m128i v2 = __lsx_vslt_b(v0, (__m128i)v1);
    const __m128i sh = __lsx_vsub_b((__m128i)v1, v2);
    const __m128i compressed = __lsx_vshuf_b(lane2, lane2, sh);

    __lsx_vst(compressed, reinterpret_cast<__m128i *>(output + 2 * 16), 0);
    __lsx_vst(lane3, reinterpret_cast<__m128i *>(output + 3 * 16 - 1), 0);
  } break;
  case 0b11: {
    __lasx_xvst(b->chunks[0], reinterpret_cast<__m256i *>(output + 0 * 16), 0);
    __lsx_vst(lasx_extracti128_lo(b->chunks[1]),
              reinterpret_cast<__m128i *>(output + 2 * 16), 0);

    const __m128i lane3 = lasx_extracti128_hi(b->chunks[1]);

    const __m128i v0 = __lsx_vreplgr2vr_b((uint8_t)(pos - 1));
    const __m128i v2 = __lsx_vslt_b(v0, (__m128i)v1);
    const __m128i sh = __lsx_vsub_b((__m128i)v1, v2);
    const __m128i compressed = __lsx_vshuf_b(lane3, lane3, sh);

    __lsx_vst(compressed, reinterpret_cast<__m128i *>(output + 3 * 16), 0);
  } break;
  }
  return 63;
}

// The caller of this function is responsible to ensure that there are 64 bytes
// available from reading at src. The data is read into a block64 structure.
static inline void load_block(block64 *b, const char *src) {
  b->chunks[0] = __lasx_xvld(reinterpret_cast<const __m256i *>(src), 0);
  b->chunks[1] = __lasx_xvld(reinterpret_cast<const __m256i *>(src), 32);
}

// The caller of this function is responsible to ensure that there are 128 bytes
// available from reading at src. The data is read into a block64 structure.
static inline void load_block(block64 *b, const char16_t *src) {
  __m256i m1 = __lasx_xvld(reinterpret_cast<const __m256i *>(src), 0);
  __m256i m2 = __lasx_xvld(reinterpret_cast<const __m256i *>(src), 32);
  __m256i m3 = __lasx_xvld(reinterpret_cast<const __m256i *>(src), 64);
  __m256i m4 = __lasx_xvld(reinterpret_cast<const __m256i *>(src), 96);
  b->chunks[0] = __lasx_xvpermi_d(__lasx_xvssrlni_bu_h(m2, m1, 0), 0b11011000);
  b->chunks[1] = __lasx_xvpermi_d(__lasx_xvssrlni_bu_h(m4, m3, 0), 0b11011000);
}

static inline void base64_decode(char *out, __m256i str) {
  __m256i t0 = __lasx_xvor_v(
      __lasx_xvslli_w(str, 26),
      __lasx_xvslli_w(__lasx_xvand_v(str, lasx_splat_u32(0x0000ff00)), 12));
  __m256i t1 =
      __lasx_xvsrli_w(__lasx_xvand_v(str, lasx_splat_u32(0x003f0000)), 2);
  __m256i t2 = __lasx_xvor_v(t0, t1);
  __m256i t3 = __lasx_xvor_v(t2, __lasx_xvsrli_w(str, 16));
  __m256i pack_shuffle = ____m256i(
      (__m128i)v16u8{3, 2, 1, 7, 6, 5, 11, 10, 9, 15, 14, 13, 0, 0, 0, 0});
  t3 = __lasx_xvshuf_b(t3, t3, (__m256i)pack_shuffle);
  t3 = __lasx_xvinsgr2vr_w(t3, 0, 7);

  // Two 16-byte stores write 28 bytes: the 24 bytes of output followed by four
  // zero bytes. Callers that cannot spare those four bytes must go through
  // base64_decode_block_safe. The bulk loop can: it only takes this path while
  // dst is below end_of_safe_64byte_zone, which leaves 63 bytes of room.
  __lsx_vst(lasx_extracti128_lo(t3), out, 0);
  __lsx_vst(lasx_extracti128_hi(t3), out, 12);
}
// decode 64 bytes and output 48 bytes
static inline void base64_decode_block(char *out, const char *src) {
  base64_decode(out, __lasx_xvld(reinterpret_cast<const __m256i *>(src), 0));
  base64_decode(out + 24,
                __lasx_xvld(reinterpret_cast<const __m256i *>(src), 32));
}

static inline void base64_decode_block_safe(char *out, const char *src) {
  alignas(32) char buffer[64];
  base64_decode_block(buffer, src);
  std::memcpy(out, buffer, 48);
}

static inline void base64_decode_block(char *out, block64 *b) {
  base64_decode(out, b->chunks[0]);
  base64_decode(out + 24, b->chunks[1]);
}
static inline void base64_decode_block_safe(char *out, block64 *b) {
  alignas(32) char buffer[64];
  base64_decode_block(buffer, b);
  std::memcpy(out, buffer, 48);
}

template <bool base64_url, bool ignore_garbage, bool default_or_url,
          typename chartype>
full_result
compress_decode_base64(char *dst, const chartype *src, size_t srclen,
                       base64_options options,
                       last_chunk_handling_options last_chunk_options) {
  const uint8_t *to_base64 =
      default_or_url ? tables::base64::to_base64_default_or_url_value
                     : (base64_url ? tables::base64::to_base64_url_value
                                   : tables::base64::to_base64_value);
  auto ri = simdutf::scalar::base64::find_end(src, srclen, options);
  size_t equallocation = ri.equallocation;
  size_t equalsigns = ri.equalsigns;
  srclen = ri.srclen;
  size_t full_input_length = ri.full_input_length;
  if (srclen == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      return {INVALID_BASE64_CHARACTER, equallocation, 0, true};
    }
    return {SUCCESS, full_input_length, 0};
  }
  char *end_of_safe_64byte_zone =
      (srclen + 3) / 4 * 3 >= 63 ? dst + (srclen + 3) / 4 * 3 - 63 : dst;

  const chartype *const srcinit = src;
  const char *const dstinit = dst;
  const chartype *const srcend = src + srclen;

  constexpr size_t block_size = 6;
  static_assert(block_size >= 2, "block_size must be at least two");
  char buffer[block_size * 64];
  char *bufferptr = buffer;
  if (srclen >= 64) {
    const chartype *const srcend64 = src + srclen - 64;
    while (src <= srcend64) {
      block64 b;
      load_block(&b, src);
      src += 64;
      bool error = false;
      uint64_t badcharmask =
          to_base64_mask<base64_url, default_or_url>(&b, &error);
      if (error && !ignore_garbage) {
        src -= 64;
        while (src < srcend && scalar::base64::is_eight_byte(*src) &&
               to_base64[uint8_t(*src)] <= 64) {
          src++;
        }
        return {error_code::INVALID_BASE64_CHARACTER, size_t(src - srcinit),
                size_t(dst - dstinit)};
      }
      if (badcharmask != 0) {
        if (is_power_of_two(badcharmask)) {
          bufferptr += compress_block_single(&b, badcharmask, bufferptr);
        } else {
          bufferptr += compress_block(&b, badcharmask, bufferptr);
        }
      } else if (bufferptr != buffer) {
        copy_block(&b, bufferptr);
        bufferptr += 64;
      } else {
        if (dst >= end_of_safe_64byte_zone) {
          base64_decode_block_safe(dst, &b);
        } else {
          base64_decode_block(dst, &b);
        }
        dst += 48;
      }
      if (bufferptr >= (block_size - 1) * 64 + buffer) {
        for (size_t i = 0; i < (block_size - 2); i++) {
          base64_decode_block(dst, buffer + i * 64);
          dst += 48;
        }
        if (dst >= end_of_safe_64byte_zone) {
          base64_decode_block_safe(dst, buffer + (block_size - 2) * 64);
        } else {
          base64_decode_block(dst, buffer + (block_size - 2) * 64);
        }
        dst += 48;
        std::memcpy(buffer, buffer + (block_size - 1) * 64,
                    64); // 64 might be too much
        bufferptr -= (block_size - 1) * 64;
      }
    }
  }

  char *buffer_start = buffer;
  // Optimization note: if this is almost full, then it is worth our
  // time, otherwise, we should just decode directly.
  int last_block = (int)((bufferptr - buffer_start) % 64);
  if (last_block != 0 && srcend - src + last_block >= 64) {

    while ((bufferptr - buffer_start) % 64 != 0 && src < srcend) {
      uint8_t val = to_base64[uint8_t(*src)];
      *bufferptr = char(val);
      if ((!scalar::base64::is_eight_byte(*src) || val > 64) &&
          !ignore_garbage) {
        return {error_code::INVALID_BASE64_CHARACTER, size_t(src - srcinit),
                size_t(dst - dstinit)};
      }
      bufferptr += (val <= 63);
      src++;
    }
  }

  for (; buffer_start + 64 <= bufferptr; buffer_start += 64) {
    if (dst >= end_of_safe_64byte_zone) {
      base64_decode_block_safe(dst, buffer_start);
    } else {
      base64_decode_block(dst, buffer_start);
    }
    dst += 48;
  }
  if ((bufferptr - buffer_start) % 64 != 0) {
    while (buffer_start + 4 < bufferptr) {
      uint32_t triple = ((uint32_t(uint8_t(buffer_start[0])) << 3 * 6) +
                         (uint32_t(uint8_t(buffer_start[1])) << 2 * 6) +
                         (uint32_t(uint8_t(buffer_start[2])) << 1 * 6) +
                         (uint32_t(uint8_t(buffer_start[3])) << 0 * 6))
                        << 8;
      // lasx is little-endian
      triple = scalar::u32_swap_bytes(triple);
      std::memcpy(dst, &triple, 4);

      dst += 3;
      buffer_start += 4;
    }
    if (buffer_start + 4 <= bufferptr) {
      uint32_t triple = ((uint32_t(uint8_t(buffer_start[0])) << 3 * 6) +
                         (uint32_t(uint8_t(buffer_start[1])) << 2 * 6) +
                         (uint32_t(uint8_t(buffer_start[2])) << 1 * 6) +
                         (uint32_t(uint8_t(buffer_start[3])) << 0 * 6))
                        << 8;
      // lasx is little-endian
      triple = scalar::u32_swap_bytes(triple);
      std::memcpy(dst, &triple, 3);

      dst += 3;
      buffer_start += 4;
    }
    // we may have 1, 2 or 3 bytes left and we need to decode them so let us
    // backtrack
    int leftover = int(bufferptr - buffer_start);
    while (leftover > 0) {
      if (!ignore_garbage) {
        while (to_base64[uint8_t(*(src - 1))] == 64) {
          src--;
        }
      } else {
        while (to_base64[uint8_t(*(src - 1))] >= 64) {
          src--;
        }
      }
      src--;
      leftover--;
    }
  }
  if (src < srcend + equalsigns) {
    full_result r = scalar::base64::base64_tail_decode(
        dst, src, srcend - src, equalsigns, options, last_chunk_options);
    r = scalar::base64::patch_tail_result(
        r, size_t(src - srcinit), size_t(dst - dstinit), equallocation,
        full_input_length, last_chunk_options);
    // When is_partial(last_chunk_options) is true, we must either end with
    // the end of the stream (beyond whitespace) or right after a non-ignorable
    // character or at the very beginning of the stream.
    // See https://tc39.es/proposal-arraybuffer-base64/spec/#sec-frombase64
    if (is_partial(last_chunk_options) && r.error == error_code::SUCCESS &&
        r.input_count < full_input_length) {
      // First check if we can extend the input to the end of the stream
      while (r.input_count < full_input_length &&
             base64_ignorable(*(srcinit + r.input_count), options)) {
        r.input_count++;
      }
      // If we are still not at the end of the stream, then we must backtrack
      // to the last non-ignorable character.
      if (r.input_count < full_input_length) {
        while (r.input_count > 0 &&
               base64_ignorable(*(srcinit + r.input_count - 1), options)) {
          r.input_count--;
        }
      }
    }
    return r;
  }
  if (equalsigns > 0 && !ignore_garbage) {
    if ((size_t(dst - dstinit) % 3 == 0) ||
        ((size_t(dst - dstinit) % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation, size_t(dst - dstinit),
              true};
    }
  }
  return {SUCCESS, srclen, size_t(dst - dstinit)};
}
