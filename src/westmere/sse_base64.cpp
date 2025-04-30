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

// --- encoding ----------------------------------------------------

template <bool base64_url> __m128i lookup_pshufb_improved(const __m128i input) {
  // credit: Wojciech Muła
  // reduce  0..51 -> 0
  //        52..61 -> 1 .. 10
  //            62 -> 11
  //            63 -> 12
  __m128i result = _mm_subs_epu8(input, _mm_set1_epi8(51));

  // distinguish between ranges 0..25 and 26..51:
  //         0 .. 25 -> remains 0
  //        26 .. 51 -> becomes 13
  const __m128i less = _mm_cmpgt_epi8(_mm_set1_epi8(26), input);
  result = _mm_or_si128(result, _mm_and_si128(less, _mm_set1_epi8(13)));

  __m128i shift_LUT;
  if (base64_url) {
    shift_LUT = _mm_setr_epi8('a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                              '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                              '0' - 52, '-' - 62, '_' - 63, 'A', 0, 0);
  } else {
    shift_LUT = _mm_setr_epi8('a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                              '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                              '0' - 52, '+' - 62, '/' - 63, 'A', 0, 0);
  }

  // read shift
  result = _mm_shuffle_epi8(shift_LUT, result);

  return _mm_add_epi8(result, input);
}

template <bool isbase64url>
size_t encode_base64(char *dst, const char *src, size_t srclen,
                     base64_options options) {
  // credit: Wojciech Muła
  // SSE (lookup: pshufb improved unrolled)
  const uint8_t *input = (const uint8_t *)src;

  uint8_t *out = (uint8_t *)dst;
  const __m128i shuf =
      _mm_set_epi8(10, 11, 9, 10, 7, 8, 6, 7, 4, 5, 3, 4, 1, 2, 0, 1);

  size_t i = 0;
  for (; i + 52 <= srclen; i += 48) {
    __m128i in0 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 0));
    __m128i in1 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 1));
    __m128i in2 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 2));
    __m128i in3 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 3));

    in0 = _mm_shuffle_epi8(in0, shuf);
    in1 = _mm_shuffle_epi8(in1, shuf);
    in2 = _mm_shuffle_epi8(in2, shuf);
    in3 = _mm_shuffle_epi8(in3, shuf);

    const __m128i t0_0 = _mm_and_si128(in0, _mm_set1_epi32(0x0fc0fc00));
    const __m128i t0_1 = _mm_and_si128(in1, _mm_set1_epi32(0x0fc0fc00));
    const __m128i t0_2 = _mm_and_si128(in2, _mm_set1_epi32(0x0fc0fc00));
    const __m128i t0_3 = _mm_and_si128(in3, _mm_set1_epi32(0x0fc0fc00));

    const __m128i t1_0 = _mm_mulhi_epu16(t0_0, _mm_set1_epi32(0x04000040));
    const __m128i t1_1 = _mm_mulhi_epu16(t0_1, _mm_set1_epi32(0x04000040));
    const __m128i t1_2 = _mm_mulhi_epu16(t0_2, _mm_set1_epi32(0x04000040));
    const __m128i t1_3 = _mm_mulhi_epu16(t0_3, _mm_set1_epi32(0x04000040));

    const __m128i t2_0 = _mm_and_si128(in0, _mm_set1_epi32(0x003f03f0));
    const __m128i t2_1 = _mm_and_si128(in1, _mm_set1_epi32(0x003f03f0));
    const __m128i t2_2 = _mm_and_si128(in2, _mm_set1_epi32(0x003f03f0));
    const __m128i t2_3 = _mm_and_si128(in3, _mm_set1_epi32(0x003f03f0));

    const __m128i t3_0 = _mm_mullo_epi16(t2_0, _mm_set1_epi32(0x01000010));
    const __m128i t3_1 = _mm_mullo_epi16(t2_1, _mm_set1_epi32(0x01000010));
    const __m128i t3_2 = _mm_mullo_epi16(t2_2, _mm_set1_epi32(0x01000010));
    const __m128i t3_3 = _mm_mullo_epi16(t2_3, _mm_set1_epi32(0x01000010));

    const __m128i input0 = _mm_or_si128(t1_0, t3_0);
    const __m128i input1 = _mm_or_si128(t1_1, t3_1);
    const __m128i input2 = _mm_or_si128(t1_2, t3_2);
    const __m128i input3 = _mm_or_si128(t1_3, t3_3);

    _mm_storeu_si128(reinterpret_cast<__m128i *>(out),
                     lookup_pshufb_improved<isbase64url>(input0));
    out += 16;

    _mm_storeu_si128(reinterpret_cast<__m128i *>(out),
                     lookup_pshufb_improved<isbase64url>(input1));
    out += 16;

    _mm_storeu_si128(reinterpret_cast<__m128i *>(out),
                     lookup_pshufb_improved<isbase64url>(input2));
    out += 16;

    _mm_storeu_si128(reinterpret_cast<__m128i *>(out),
                     lookup_pshufb_improved<isbase64url>(input3));
    out += 16;
  }
  for (; i + 16 <= srclen; i += 12) {

    __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + i));

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
    in = _mm_shuffle_epi8(in, shuf);

    // unpacking

    // t0    = [0000cccc|cc000000|aaaaaa00|00000000]
    const __m128i t0 = _mm_and_si128(in, _mm_set1_epi32(0x0fc0fc00));
    // t1    = [00000000|00cccccc|00000000|00aaaaaa]
    //          (c * (1 << 10), a * (1 << 6)) >> 16 (note: an unsigned
    //          multiplication)
    const __m128i t1 = _mm_mulhi_epu16(t0, _mm_set1_epi32(0x04000040));

    // t2    = [00000000|00dddddd|000000bb|bbbb0000]
    const __m128i t2 = _mm_and_si128(in, _mm_set1_epi32(0x003f03f0));
    // t3    = [00dddddd|00000000|00bbbbbb|00000000](
    //          (d * (1 << 8), b * (1 << 4))
    const __m128i t3 = _mm_mullo_epi16(t2, _mm_set1_epi32(0x01000010));

    // res   = [00dddddd|00cccccc|00bbbbbb|00aaaaaa] = t1 | t3
    const __m128i indices = _mm_or_si128(t1, t3);

    _mm_storeu_si128(reinterpret_cast<__m128i *>(out),
                     lookup_pshufb_improved<isbase64url>(indices));
    out += 16;
  }

  return i / 3 * 4 + scalar::base64::tail_encode_base64((char *)out, src + i,
                                                        srclen - i, options);
}

// --- decoding -----------------------------------------------

static simdutf_really_inline void compress(__m128i data, uint16_t mask,
                                           char *output) {
  if (mask == 0) {
    _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
    return;
  }

  // this particular implementation was inspired by work done by @animetosho
  // we do it in two steps, first 8 bytes and then second 8 bytes
  uint8_t mask1 = uint8_t(mask);      // least significant 8 bits
  uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
  // next line just loads the 64-bit values thintable_epi8[mask1] and
  // thintable_epi8[mask2] into a 128-bit register, using only
  // two instructions on most compilers.

  __m128i shufmask = _mm_set_epi64x(tables::base64::thintable_epi8[mask2],
                                    tables::base64::thintable_epi8[mask1]);
  // we increment by 0x08 the second half of the mask
  shufmask =
      _mm_add_epi8(shufmask, _mm_set_epi32(0x08080808, 0x08080808, 0, 0));
  // this is the version "nearly pruned"
  __m128i pruned = _mm_shuffle_epi8(data, shufmask);
  // we still need to put the two halves together.
  // we compute the popcount of the first half:
  int pop1 = tables::base64::BitsSetTable256mul2[mask1];
  // then load the corresponding mask, what it does is to write
  // only the first pop1 bytes from the first 8 bytes, and then
  // it fills in with the bytes from the second 8 bytes + some filling
  // at the end.
  __m128i compactmask = _mm_loadu_si128(reinterpret_cast<const __m128i *>(
      tables::base64::pshufb_combine_table + pop1 * 8));
  __m128i answer = _mm_shuffle_epi8(pruned, compactmask);
  _mm_storeu_si128(reinterpret_cast<__m128i *>(output), answer);
}

static simdutf_really_inline void base64_decode(char *out, __m128i str) {
  // credit: aqrit

  const __m128i pack_shuffle =
      _mm_setr_epi8(2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1);

  const __m128i t0 = _mm_maddubs_epi16(str, _mm_set1_epi32(0x01400140));
  const __m128i t1 = _mm_madd_epi16(t0, _mm_set1_epi32(0x00011000));
  const __m128i t2 = _mm_shuffle_epi8(t1, pack_shuffle);
  // Store the output:
  // this writes 16 bytes, but we only need 12.
  _mm_storeu_si128((__m128i *)out, t2);
}

// decode 64 bytes and output 48 bytes
static inline void base64_decode_block(char *out, const char *src) {
  base64_decode(out, _mm_loadu_si128(reinterpret_cast<const __m128i *>(src)));
  base64_decode(out + 12,
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 16)));
  base64_decode(out + 24,
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 32)));
  base64_decode(out + 36,
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 48)));
}

static inline void base64_decode_block_safe(char *out, const char *src) {
  base64_decode(out, _mm_loadu_si128(reinterpret_cast<const __m128i *>(src)));
  base64_decode(out + 12,
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 16)));
  base64_decode(out + 24,
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 32)));
  char buffer[16];
  base64_decode(buffer,
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 48)));
  std::memcpy(out + 36, buffer, 12);
}

// --- decoding - base64 class --------------------------------

class block64 {
  __m128i chunks[4];

public:
  // The caller of this function is responsible to ensure that there are 64
  // bytes available from reading at src.
  simdutf_really_inline block64(const char *src) {
    chunks[0] = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
    chunks[1] = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 16));
    chunks[2] = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 32));
    chunks[3] = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 48));
  }

public:
  // The caller of this function is responsible to ensure that there are 128
  // bytes available from reading at src. The data is read into a block64
  // structure.
  simdutf_really_inline block64(const char16_t *src) {
    const auto m1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
    const auto m2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 8));
    const auto m3 =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 16));
    const auto m4 =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 24));
    const auto m5 =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 32));
    const auto m6 =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 40));
    const auto m7 =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 48));
    const auto m8 =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 56));
    chunks[0] = _mm_packus_epi16(m1, m2);
    chunks[1] = _mm_packus_epi16(m3, m4);
    chunks[2] = _mm_packus_epi16(m5, m6);
    chunks[3] = _mm_packus_epi16(m7, m8);
  }

public:
  simdutf_really_inline void copy_block(char *output) {
    _mm_storeu_si128(reinterpret_cast<__m128i *>(output), chunks[0]);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(output + 16), chunks[1]);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(output + 32), chunks[2]);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(output + 48), chunks[3]);
  }

public:
  simdutf_really_inline uint64_t compress_block(uint64_t mask, char *output) {
    if (is_power_of_two(mask)) {
      return compress_block_single(mask, output);
    }

    uint64_t nmask = ~mask;
    compress(chunks[0], uint16_t(mask), output);
    compress(chunks[1], uint16_t(mask >> 16),
             output + count_ones(nmask & 0xFFFF));
    compress(chunks[2], uint16_t(mask >> 32),
             output + count_ones(nmask & 0xFFFFFFFF));
    compress(chunks[3], uint16_t(mask >> 48),
             output + count_ones(nmask & 0xFFFFFFFFFFFFULL));
    return count_ones(nmask);
  }

private:
  simdutf_really_inline size_t compress_block_single(uint64_t mask,
                                                     char *output) {
    const size_t pos64 = trailing_zeroes(mask);
    const int8_t pos = pos64 & 0xf;
    switch (pos64 >> 4) {
    case 0b00: {
      const __m128i v0 = _mm_set1_epi8(char(pos - 1));
      const __m128i v1 =
          _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      const __m128i v2 = _mm_cmpgt_epi8(v1, v0);
      const __m128i sh = _mm_sub_epi8(v1, v2);
      const __m128i compressed = _mm_shuffle_epi8(chunks[0], sh);

      _mm_storeu_si128((__m128i *)(output + 0 * 16), compressed);
      _mm_storeu_si128((__m128i *)(output + 1 * 16 - 1), chunks[1]);
      _mm_storeu_si128((__m128i *)(output + 2 * 16 - 1), chunks[2]);
      _mm_storeu_si128((__m128i *)(output + 3 * 16 - 1), chunks[3]);
    } break;
    case 0b01: {
      _mm_storeu_si128((__m128i *)(output + 0 * 16), chunks[0]);

      const __m128i v0 = _mm_set1_epi8(char(pos - 1));
      const __m128i v1 =
          _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      const __m128i v2 = _mm_cmpgt_epi8(v1, v0);
      const __m128i sh = _mm_sub_epi8(v1, v2);
      const __m128i compressed = _mm_shuffle_epi8(chunks[1], sh);

      _mm_storeu_si128((__m128i *)(output + 1 * 16), compressed);
      _mm_storeu_si128((__m128i *)(output + 2 * 16 - 1), chunks[2]);
      _mm_storeu_si128((__m128i *)(output + 3 * 16 - 1), chunks[3]);
    } break;
    case 0b10: {
      _mm_storeu_si128((__m128i *)(output + 0 * 16), chunks[0]);
      _mm_storeu_si128((__m128i *)(output + 1 * 16), chunks[1]);

      const __m128i v0 = _mm_set1_epi8(char(pos - 1));
      const __m128i v1 =
          _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      const __m128i v2 = _mm_cmpgt_epi8(v1, v0);
      const __m128i sh = _mm_sub_epi8(v1, v2);
      const __m128i compressed = _mm_shuffle_epi8(chunks[2], sh);

      _mm_storeu_si128((__m128i *)(output + 2 * 16), compressed);
      _mm_storeu_si128((__m128i *)(output + 3 * 16 - 1), chunks[3]);
    } break;
    case 0b11: {
      _mm_storeu_si128((__m128i *)(output + 0 * 16), chunks[0]);
      _mm_storeu_si128((__m128i *)(output + 1 * 16), chunks[1]);
      _mm_storeu_si128((__m128i *)(output + 2 * 16), chunks[2]);

      const __m128i v0 = _mm_set1_epi8(char(pos - 1));
      const __m128i v1 =
          _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      const __m128i v2 = _mm_cmpgt_epi8(v1, v0);
      const __m128i sh = _mm_sub_epi8(v1, v2);
      const __m128i compressed = _mm_shuffle_epi8(chunks[3], sh);

      _mm_storeu_si128((__m128i *)(output + 3 * 16), compressed);
    } break;
    }

    return 63;
  }

public:
  template <bool base64_url, bool ignore_garbage, bool default_or_url>
  simdutf_really_inline uint64_t to_base64_mask(uint64_t *error) {
    uint32_t err0 = 0;
    uint32_t err1 = 0;
    uint32_t err2 = 0;
    uint32_t err3 = 0;
    uint64_t m0 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        &chunks[0], &err0);
    uint64_t m1 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        &chunks[1], &err1);
    uint64_t m2 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        &chunks[2], &err2);
    uint64_t m3 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        &chunks[3], &err3);
    if (!ignore_garbage) {
      *error = (err0) | ((uint64_t)err1 << 16) | ((uint64_t)err2 << 32) |
               ((uint64_t)err3 << 48);
    }
    return m0 | (m1 << 16) | (m2 << 32) | (m3 << 48);
  }

private:
  template <bool base64_url, bool ignore_garbage, bool default_or_url>
  simdutf_really_inline uint16_t to_base64_mask(__m128i *src, uint32_t *error) {
    const __m128i ascii_space_tbl =
        _mm_setr_epi8(0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x9, 0xa,
                      0x0, 0xc, 0xd, 0x0, 0x0);
    // credit: aqrit
    __m128i delta_asso;
    if (default_or_url) {
      delta_asso =
          _mm_setr_epi8(0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x16);
    } else if (base64_url) {
      delta_asso = _mm_setr_epi8(0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0,
                                 0x0, 0x0, 0x0, 0x0, 0xF, 0x0, 0xF);
    } else {
      delta_asso =
          _mm_setr_epi8(0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F);
    }
    __m128i delta_values;
    if (default_or_url) {
      delta_values = _mm_setr_epi8(
          uint8_t(0xBF), uint8_t(0xE0), uint8_t(0xB9), uint8_t(0x13),
          uint8_t(0x04), uint8_t(0xBF), uint8_t(0xBF), uint8_t(0xB9),
          uint8_t(0xB9), uint8_t(0x00), uint8_t(0xFF), uint8_t(0x11),
          uint8_t(0xFF), uint8_t(0xBF), uint8_t(0x10), uint8_t(0xB9));

    } else if (base64_url) {
      delta_values = _mm_setr_epi8(0x0, 0x0, 0x0, 0x13, 0x4, uint8_t(0xBF),
                                   uint8_t(0xBF), uint8_t(0xB9), uint8_t(0xB9),
                                   0x0, 0x11, uint8_t(0xC3), uint8_t(0xBF),
                                   uint8_t(0xE0), uint8_t(0xB9), uint8_t(0xB9));
    } else {
      delta_values =
          _mm_setr_epi8(int8_t(0x00), int8_t(0x00), int8_t(0x00), int8_t(0x13),
                        int8_t(0x04), int8_t(0xBF), int8_t(0xBF), int8_t(0xB9),
                        int8_t(0xB9), int8_t(0x00), int8_t(0x10), int8_t(0xC3),
                        int8_t(0xBF), int8_t(0xBF), int8_t(0xB9), int8_t(0xB9));
    }
    __m128i check_asso;
    if (default_or_url) {
      check_asso =
          _mm_setr_epi8(0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x03, 0x07, 0x0B, 0x0E, 0x0B, 0x06);
    } else if (base64_url) {
      check_asso = _mm_setr_epi8(0xD, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
                                 0x1, 0x3, 0x7, 0xB, 0xE, 0xB, 0x6);
    } else {
      check_asso =
          _mm_setr_epi8(0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x03, 0x07, 0x0B, 0x0B, 0x0B, 0x0F);
    }
    __m128i check_values;
    if (default_or_url) {
      check_values = _mm_setr_epi8(
          uint8_t(0x80), uint8_t(0x80), uint8_t(0x80), uint8_t(0x80),
          uint8_t(0xCF), uint8_t(0xBF), uint8_t(0xD5), uint8_t(0xA6),
          uint8_t(0xB5), uint8_t(0xA1), uint8_t(0x00), uint8_t(0x80),
          uint8_t(0x00), uint8_t(0x80), uint8_t(0x00), uint8_t(0x80));
    } else if (base64_url) {
      check_values = _mm_setr_epi8(uint8_t(0x80), uint8_t(0x80), uint8_t(0x80),
                                   uint8_t(0x80), uint8_t(0xCF), uint8_t(0xBF),
                                   uint8_t(0xB6), uint8_t(0xA6), uint8_t(0xB5),
                                   uint8_t(0xA1), 0x0, uint8_t(0x80), 0x0,
                                   uint8_t(0x80), 0x0, uint8_t(0x80));
    } else {
      check_values =
          _mm_setr_epi8(int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0x80),
                        int8_t(0xCF), int8_t(0xBF), int8_t(0xD5), int8_t(0xA6),
                        int8_t(0xB5), int8_t(0x86), int8_t(0xD1), int8_t(0x80),
                        int8_t(0xB1), int8_t(0x80), int8_t(0x91), int8_t(0x80));
    }
    const __m128i shifted = _mm_srli_epi32(*src, 3);

    __m128i delta_hash =
        _mm_avg_epu8(_mm_shuffle_epi8(delta_asso, *src), shifted);
    if (default_or_url) {
      delta_hash = _mm_and_si128(delta_hash, _mm_set1_epi8(0xf));
    }
    const __m128i check_hash =
        _mm_avg_epu8(_mm_shuffle_epi8(check_asso, *src), shifted);

    const __m128i out =
        _mm_adds_epi8(_mm_shuffle_epi8(delta_values, delta_hash), *src);
    const __m128i chk =
        _mm_adds_epi8(_mm_shuffle_epi8(check_values, check_hash), *src);
    const int mask = _mm_movemask_epi8(chk);
    if (!ignore_garbage && mask) {
      __m128i ascii_space =
          _mm_cmpeq_epi8(_mm_shuffle_epi8(ascii_space_tbl, *src), *src);
      *error = (mask ^ _mm_movemask_epi8(ascii_space));
    }
    *src = out;
    return (uint16_t)mask;
  }

public:
  simdutf_really_inline void base64_decode_block(char *out) {
    base64_decode(out, chunks[0]);
    base64_decode(out + 12, chunks[1]);
    base64_decode(out + 24, chunks[2]);
    base64_decode(out + 36, chunks[3]);
  }

public:
  simdutf_really_inline void base64_decode_block_safe(char *out) {
    base64_decode(out, chunks[0]);
    base64_decode(out + 12, chunks[1]);
    base64_decode(out + 24, chunks[2]);
    char buffer[16];
    base64_decode(buffer, chunks[3]);
    std::memcpy(out + 36, buffer, 12);
  }
};
