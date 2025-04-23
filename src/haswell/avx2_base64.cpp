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

template <bool base64_url>
simdutf_really_inline __m256i lookup_pshufb_improved(const __m256i input) {
  // credit: Wojciech Muła
  __m256i result = _mm256_subs_epu8(input, _mm256_set1_epi8(51));
  const __m256i less = _mm256_cmpgt_epi8(_mm256_set1_epi8(26), input);
  result =
      _mm256_or_si256(result, _mm256_and_si256(less, _mm256_set1_epi8(13)));
  __m256i shift_LUT;
  if (base64_url) {
    shift_LUT = _mm256_setr_epi8(
        'a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
        '0' - 52, '0' - 52, '0' - 52, '0' - 52, '-' - 62, '_' - 63, 'A', 0, 0,

        'a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
        '0' - 52, '0' - 52, '0' - 52, '0' - 52, '-' - 62, '_' - 63, 'A', 0, 0);
  } else {
    shift_LUT = _mm256_setr_epi8(
        'a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
        '0' - 52, '0' - 52, '0' - 52, '0' - 52, '+' - 62, '/' - 63, 'A', 0, 0,

        'a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
        '0' - 52, '0' - 52, '0' - 52, '0' - 52, '+' - 62, '/' - 63, 'A', 0, 0);
  }

  result = _mm256_shuffle_epi8(shift_LUT, result);
  return _mm256_add_epi8(result, input);
}

template <bool isbase64url>
size_t encode_base64(char *dst, const char *src, size_t srclen,
                     base64_options options) {
  // credit: Wojciech Muła
  const uint8_t *input = (const uint8_t *)src;

  uint8_t *out = (uint8_t *)dst;
  const __m256i shuf =
      _mm256_set_epi8(10, 11, 9, 10, 7, 8, 6, 7, 4, 5, 3, 4, 1, 2, 0, 1,

                      10, 11, 9, 10, 7, 8, 6, 7, 4, 5, 3, 4, 1, 2, 0, 1);
  size_t i = 0;
  for (; i + 100 <= srclen; i += 96) {
    const __m128i lo0 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 0));
    const __m128i hi0 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 1));
    const __m128i lo1 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 2));
    const __m128i hi1 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 3));
    const __m128i lo2 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 4));
    const __m128i hi2 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 5));
    const __m128i lo3 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 6));
    const __m128i hi3 = _mm_loadu_si128(
        reinterpret_cast<const __m128i *>(input + i + 4 * 3 * 7));

    __m256i in0 = _mm256_shuffle_epi8(_mm256_set_m128i(hi0, lo0), shuf);
    __m256i in1 = _mm256_shuffle_epi8(_mm256_set_m128i(hi1, lo1), shuf);
    __m256i in2 = _mm256_shuffle_epi8(_mm256_set_m128i(hi2, lo2), shuf);
    __m256i in3 = _mm256_shuffle_epi8(_mm256_set_m128i(hi3, lo3), shuf);

    const __m256i t0_0 = _mm256_and_si256(in0, _mm256_set1_epi32(0x0fc0fc00));
    const __m256i t0_1 = _mm256_and_si256(in1, _mm256_set1_epi32(0x0fc0fc00));
    const __m256i t0_2 = _mm256_and_si256(in2, _mm256_set1_epi32(0x0fc0fc00));
    const __m256i t0_3 = _mm256_and_si256(in3, _mm256_set1_epi32(0x0fc0fc00));

    const __m256i t1_0 =
        _mm256_mulhi_epu16(t0_0, _mm256_set1_epi32(0x04000040));
    const __m256i t1_1 =
        _mm256_mulhi_epu16(t0_1, _mm256_set1_epi32(0x04000040));
    const __m256i t1_2 =
        _mm256_mulhi_epu16(t0_2, _mm256_set1_epi32(0x04000040));
    const __m256i t1_3 =
        _mm256_mulhi_epu16(t0_3, _mm256_set1_epi32(0x04000040));

    const __m256i t2_0 = _mm256_and_si256(in0, _mm256_set1_epi32(0x003f03f0));
    const __m256i t2_1 = _mm256_and_si256(in1, _mm256_set1_epi32(0x003f03f0));
    const __m256i t2_2 = _mm256_and_si256(in2, _mm256_set1_epi32(0x003f03f0));
    const __m256i t2_3 = _mm256_and_si256(in3, _mm256_set1_epi32(0x003f03f0));

    const __m256i t3_0 =
        _mm256_mullo_epi16(t2_0, _mm256_set1_epi32(0x01000010));
    const __m256i t3_1 =
        _mm256_mullo_epi16(t2_1, _mm256_set1_epi32(0x01000010));
    const __m256i t3_2 =
        _mm256_mullo_epi16(t2_2, _mm256_set1_epi32(0x01000010));
    const __m256i t3_3 =
        _mm256_mullo_epi16(t2_3, _mm256_set1_epi32(0x01000010));

    const __m256i input0 = _mm256_or_si256(t1_0, t3_0);
    const __m256i input1 = _mm256_or_si256(t1_1, t3_1);
    const __m256i input2 = _mm256_or_si256(t1_2, t3_2);
    const __m256i input3 = _mm256_or_si256(t1_3, t3_3);

    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out),
                        lookup_pshufb_improved<isbase64url>(input0));
    out += 32;

    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out),
                        lookup_pshufb_improved<isbase64url>(input1));
    out += 32;

    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out),
                        lookup_pshufb_improved<isbase64url>(input2));
    out += 32;
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out),
                        lookup_pshufb_improved<isbase64url>(input3));
    out += 32;
  }
  for (; i + 28 <= srclen; i += 24) {
    // lo = [xxxx|DDDC|CCBB|BAAA]
    // hi = [xxxx|HHHG|GGFF|FEEE]
    const __m128i lo =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + i));
    const __m128i hi =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(input + i + 4 * 3));

    // bytes from groups A, B and C are needed in separate 32-bit lanes
    // in = [0HHH|0GGG|0FFF|0EEE[0DDD|0CCC|0BBB|0AAA]
    __m256i in = _mm256_shuffle_epi8(_mm256_set_m128i(hi, lo), shuf);

    // this part is well commented in encode.sse.cpp

    const __m256i t0 = _mm256_and_si256(in, _mm256_set1_epi32(0x0fc0fc00));
    const __m256i t1 = _mm256_mulhi_epu16(t0, _mm256_set1_epi32(0x04000040));
    const __m256i t2 = _mm256_and_si256(in, _mm256_set1_epi32(0x003f03f0));
    const __m256i t3 = _mm256_mullo_epi16(t2, _mm256_set1_epi32(0x01000010));
    const __m256i indices = _mm256_or_si256(t1, t3);

    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out),
                        lookup_pshufb_improved<isbase64url>(indices));
    out += 32;
  }
  return i / 3 * 4 + scalar::base64::tail_encode_base64((char *)out, src + i,
                                                        srclen - i, options);
}

static inline void compress(__m128i data, uint16_t mask, char *output) {
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

// --- decoding -----------------------------------------------

template <typename = void>
simdutf_really_inline void compress(__m256i data, uint32_t mask, char *output) {
  if (mask == 0) {
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(output), data);
    return;
  }
  compress(_mm256_castsi256_si128(data), uint16_t(mask), output);
  compress(_mm256_extracti128_si256(data, 1), uint16_t(mask >> 16),
           output + count_ones(~mask & 0xFFFF));
}

template <typename = void>
simdutf_really_inline void base64_decode(char *out, __m256i str) {
  // credit: aqrit
  const __m256i pack_shuffle =
      _mm256_setr_epi8(2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1,
                       2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1);
  const __m256i t0 = _mm256_maddubs_epi16(str, _mm256_set1_epi32(0x01400140));
  const __m256i t1 = _mm256_madd_epi16(t0, _mm256_set1_epi32(0x00011000));
  const __m256i t2 = _mm256_shuffle_epi8(t1, pack_shuffle);

  // Store the output:
  _mm_storeu_si128((__m128i *)out, _mm256_castsi256_si128(t2));
  _mm_storeu_si128((__m128i *)(out + 12), _mm256_extracti128_si256(t2, 1));
}

template <typename = void>
simdutf_really_inline void base64_decode_block(char *out, const char *src) {
  base64_decode(out,
                _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
  base64_decode(out + 24, _mm256_loadu_si256(
                              reinterpret_cast<const __m256i *>(src + 32)));
}

template <typename = void>
simdutf_really_inline void base64_decode_block_safe(char *out,
                                                    const char *src) {
  base64_decode(out,
                _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src)));
  char buffer[32]; // We enforce safety with a buffer.
  base64_decode(
      buffer, _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32)));
  std::memcpy(out + 24, buffer, 24);
}

// --- decoding - base64 class --------------------------------

class block64 {
  __m256i chunks[2];

public:
  // The caller of this function is responsible to ensure that there are 64
  // bytes available from reading at src.
  simdutf_really_inline block64(const char *src) {
    chunks[0] = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
    chunks[1] = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32));
  }

  // The caller of this function is responsible to ensure that there are 128
  // bytes available from reading at src.
  simdutf_really_inline block64(const char16_t *src) {
    const auto m1 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src));
    const auto m2 =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 16));
    const auto m3 =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 32));
    const auto m4 =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + 48));

    const auto m1p = _mm256_permute2x128_si256(m1, m2, 0x20);
    const auto m2p = _mm256_permute2x128_si256(m1, m2, 0x31);
    const auto m3p = _mm256_permute2x128_si256(m3, m4, 0x20);
    const auto m4p = _mm256_permute2x128_si256(m3, m4, 0x31);

    chunks[0] = _mm256_packus_epi16(m1p, m2p);
    chunks[1] = _mm256_packus_epi16(m3p, m4p);
  }

  simdutf_really_inline void copy_block(char *output) {
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(output), chunks[0]);
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(output + 32), chunks[1]);
  }

  // decode 64 bytes and output 48 bytes
  simdutf_really_inline void base64_decode_block(char *out) {
    base64_decode(out, chunks[0]);
    base64_decode(out + 24, chunks[1]);
  }

  simdutf_really_inline void base64_decode_block_safe(char *out) {
    base64_decode(out, chunks[0]);
    char buffer[32]; // We enforce safety with a buffer.
    base64_decode(buffer, chunks[1]);
    std::memcpy(out + 24, buffer, 24);
  }

  template <bool base64_url, bool ignore_garbage, bool default_or_url>
  simdutf_really_inline uint64_t to_base64_mask(uint64_t *error) {
    uint32_t err0 = 0;
    uint32_t err1 = 0;
    uint64_t m0 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        &chunks[0], &err0);
    uint64_t m1 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        &chunks[1], &err1);
    if (!ignore_garbage) {
      *error = err0 | ((uint64_t)err1 << 32);
    }
    return m0 | (m1 << 32);
  }

  template <bool base64_url, bool ignore_garbage, bool default_or_url>
  simdutf_really_inline uint32_t to_base64_mask(__m256i *src, uint32_t *error) {
    const __m256i ascii_space_tbl =
        _mm256_setr_epi8(0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x9, 0xa,
                         0x0, 0xc, 0xd, 0x0, 0x0, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0,
                         0x0, 0x0, 0x0, 0x9, 0xa, 0x0, 0xc, 0xd, 0x0, 0x0);
    // credit: aqrit
    __m256i delta_asso;
    if (default_or_url) {
      delta_asso = _mm256_setr_epi8(
          0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x11, 0x00, 0x16, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
          0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x16);
    } else if (base64_url) {
      delta_asso = _mm256_setr_epi8(0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0,
                                    0x0, 0x0, 0x0, 0x0, 0xF, 0x0, 0xF, 0x1, 0x1,
                                    0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0,
                                    0x0, 0x0, 0xF, 0x0, 0xF);
    } else {
      delta_asso = _mm256_setr_epi8(
          0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x0F, 0x00, 0x0F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
          0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F);
    }

    __m256i delta_values;
    if (default_or_url) {
      delta_values = _mm256_setr_epi8(
          uint8_t(0xBF), uint8_t(0xE0), uint8_t(0xB9), uint8_t(0x13),
          uint8_t(0x04), uint8_t(0xBF), uint8_t(0xBF), uint8_t(0xB9),
          uint8_t(0xB9), uint8_t(0x00), uint8_t(0xFF), uint8_t(0x11),
          uint8_t(0xFF), uint8_t(0xBF), uint8_t(0x10), uint8_t(0xB9),
          uint8_t(0xBF), uint8_t(0xE0), uint8_t(0xB9), uint8_t(0x13),
          uint8_t(0x04), uint8_t(0xBF), uint8_t(0xBF), uint8_t(0xB9),
          uint8_t(0xB9), uint8_t(0x00), uint8_t(0xFF), uint8_t(0x11),
          uint8_t(0xFF), uint8_t(0xBF), uint8_t(0x10), uint8_t(0xB9));
    } else if (base64_url) {
      delta_values = _mm256_setr_epi8(
          0x0, 0x0, 0x0, 0x13, 0x4, uint8_t(0xBF), uint8_t(0xBF), uint8_t(0xB9),
          uint8_t(0xB9), 0x0, 0x11, uint8_t(0xC3), uint8_t(0xBF), uint8_t(0xE0),
          uint8_t(0xB9), uint8_t(0xB9), 0x0, 0x0, 0x0, 0x13, 0x4, uint8_t(0xBF),
          uint8_t(0xBF), uint8_t(0xB9), uint8_t(0xB9), 0x0, 0x11, uint8_t(0xC3),
          uint8_t(0xBF), uint8_t(0xE0), uint8_t(0xB9), uint8_t(0xB9));
    } else {
      delta_values = _mm256_setr_epi8(
          int8_t(0x00), int8_t(0x00), int8_t(0x00), int8_t(0x13), int8_t(0x04),
          int8_t(0xBF), int8_t(0xBF), int8_t(0xB9), int8_t(0xB9), int8_t(0x00),
          int8_t(0x10), int8_t(0xC3), int8_t(0xBF), int8_t(0xBF), int8_t(0xB9),
          int8_t(0xB9), int8_t(0x00), int8_t(0x00), int8_t(0x00), int8_t(0x13),
          int8_t(0x04), int8_t(0xBF), int8_t(0xBF), int8_t(0xB9), int8_t(0xB9),
          int8_t(0x00), int8_t(0x10), int8_t(0xC3), int8_t(0xBF), int8_t(0xBF),
          int8_t(0xB9), int8_t(0xB9));
    }

    __m256i check_asso;
    if (default_or_url) {
      check_asso = _mm256_setr_epi8(
          0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03,
          0x07, 0x0B, 0x0E, 0x0B, 0x06, 0x0D, 0x01, 0x01, 0x01, 0x01, 0x01,
          0x01, 0x01, 0x01, 0x01, 0x03, 0x07, 0x0B, 0x0E, 0x0B, 0x06);
    } else if (base64_url) {
      check_asso = _mm256_setr_epi8(0xD, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
                                    0x1, 0x3, 0x7, 0xB, 0xE, 0xB, 0x6, 0xD, 0x1,
                                    0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x3,
                                    0x7, 0xB, 0xE, 0xB, 0x6);
    } else {
      check_asso = _mm256_setr_epi8(
          0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03,
          0x07, 0x0B, 0x0B, 0x0B, 0x0F, 0x0D, 0x01, 0x01, 0x01, 0x01, 0x01,
          0x01, 0x01, 0x01, 0x01, 0x03, 0x07, 0x0B, 0x0B, 0x0B, 0x0F);
    }
    __m256i check_values;
    if (default_or_url) {
      check_values = _mm256_setr_epi8(
          uint8_t(0x80), uint8_t(0x80), uint8_t(0x80), uint8_t(0x80),
          uint8_t(0xCF), uint8_t(0xBF), uint8_t(0xD5), uint8_t(0xA6),
          uint8_t(0xB5), uint8_t(0xA1), uint8_t(0x00), uint8_t(0x80),
          uint8_t(0x00), uint8_t(0x80), uint8_t(0x00), uint8_t(0x80),
          uint8_t(0x80), uint8_t(0x80), uint8_t(0x80), uint8_t(0x80),
          uint8_t(0xCF), uint8_t(0xBF), uint8_t(0xD5), uint8_t(0xA6),
          uint8_t(0xB5), uint8_t(0xA1), uint8_t(0x00), uint8_t(0x80),
          uint8_t(0x00), uint8_t(0x80), uint8_t(0x00), uint8_t(0x80));
    } else if (base64_url) {
      check_values = _mm256_setr_epi8(
          uint8_t(0x80), uint8_t(0x80), uint8_t(0x80), uint8_t(0x80),
          uint8_t(0xCF), uint8_t(0xBF), uint8_t(0xB6), uint8_t(0xA6),
          uint8_t(0xB5), uint8_t(0xA1), 0x0, uint8_t(0x80), 0x0, uint8_t(0x80),
          0x0, uint8_t(0x80), uint8_t(0x80), uint8_t(0x80), uint8_t(0x80),
          uint8_t(0x80), uint8_t(0xCF), uint8_t(0xBF), uint8_t(0xB6),
          uint8_t(0xA6), uint8_t(0xB5), uint8_t(0xA1), 0x0, uint8_t(0x80), 0x0,
          uint8_t(0x80), 0x0, uint8_t(0x80));
    } else {
      check_values = _mm256_setr_epi8(
          int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0xCF),
          int8_t(0xBF), int8_t(0xD5), int8_t(0xA6), int8_t(0xB5), int8_t(0x86),
          int8_t(0xD1), int8_t(0x80), int8_t(0xB1), int8_t(0x80), int8_t(0x91),
          int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0x80),
          int8_t(0xCF), int8_t(0xBF), int8_t(0xD5), int8_t(0xA6), int8_t(0xB5),
          int8_t(0x86), int8_t(0xD1), int8_t(0x80), int8_t(0xB1), int8_t(0x80),
          int8_t(0x91), int8_t(0x80));
    }
    const __m256i shifted = _mm256_srli_epi32(*src, 3);
    __m256i delta_hash =
        _mm256_avg_epu8(_mm256_shuffle_epi8(delta_asso, *src), shifted);
    if (default_or_url) {
      delta_hash = _mm256_and_si256(delta_hash, _mm256_set1_epi8(0xf));
    }
    const __m256i check_hash =
        _mm256_avg_epu8(_mm256_shuffle_epi8(check_asso, *src), shifted);
    const __m256i out =
        _mm256_adds_epi8(_mm256_shuffle_epi8(delta_values, delta_hash), *src);
    const __m256i chk =
        _mm256_adds_epi8(_mm256_shuffle_epi8(check_values, check_hash), *src);
    const int mask = _mm256_movemask_epi8(chk);
    if (!ignore_garbage && mask) {
      __m256i ascii_space =
          _mm256_cmpeq_epi8(_mm256_shuffle_epi8(ascii_space_tbl, *src), *src);
      *error = (mask ^ _mm256_movemask_epi8(ascii_space));
    }
    *src = out;
    return (uint32_t)mask;
  }

  simdutf_really_inline uint64_t compress_block(uint64_t mask, char *output) {
    if (is_power_of_two(mask)) {
      return compress_block_single(mask, output);
    }

    uint64_t nmask = ~mask;
    compress(chunks[0], uint32_t(mask), output);
    compress(chunks[1], uint32_t(mask >> 32),
             output + count_ones(nmask & 0xFFFFFFFF));
    return count_ones(nmask);
  }

  simdutf_really_inline size_t compress_block_single(uint64_t mask,
                                                     char *output) {
    const size_t pos64 = trailing_zeroes(mask);
    const int8_t pos = pos64 & 0xf;
    switch (pos64 >> 4) {
    case 0b00: {
      const __m128i lane0 = _mm256_extracti128_si256(chunks[0], 0);
      const __m128i lane1 = _mm256_extracti128_si256(chunks[0], 1);

      const __m128i v0 = _mm_set1_epi8(char(pos - 1));
      const __m128i v1 =
          _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      const __m128i v2 = _mm_cmpgt_epi8(v1, v0);
      const __m128i sh = _mm_sub_epi8(v1, v2);
      const __m128i compressed = _mm_shuffle_epi8(lane0, sh);

      _mm_storeu_si128((__m128i *)(output + 0 * 16), compressed);
      _mm_storeu_si128((__m128i *)(output + 1 * 16 - 1), lane1);
      _mm256_storeu_si256((__m256i *)(output + 2 * 16 - 1), chunks[1]);
    } break;
    case 0b01: {
      const __m128i lane0 = _mm256_extracti128_si256(chunks[0], 0);
      const __m128i lane1 = _mm256_extracti128_si256(chunks[0], 1);
      _mm_storeu_si128((__m128i *)(output + 0 * 16), lane0);

      const __m128i v0 = _mm_set1_epi8(char(pos - 1));
      const __m128i v1 =
          _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      const __m128i v2 = _mm_cmpgt_epi8(v1, v0);
      const __m128i sh = _mm_sub_epi8(v1, v2);
      const __m128i compressed = _mm_shuffle_epi8(lane1, sh);

      _mm_storeu_si128((__m128i *)(output + 1 * 16), compressed);
      _mm256_storeu_si256((__m256i *)(output + 2 * 16 - 1), chunks[1]);
    } break;
    case 0b10: {
      const __m128i lane2 = _mm256_extracti128_si256(chunks[1], 0);
      const __m128i lane3 = _mm256_extracti128_si256(chunks[1], 1);

      _mm256_storeu_si256((__m256i *)(output + 0 * 16), chunks[0]);

      const __m128i v0 = _mm_set1_epi8(char(pos - 1));
      const __m128i v1 =
          _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      const __m128i v2 = _mm_cmpgt_epi8(v1, v0);
      const __m128i sh = _mm_sub_epi8(v1, v2);
      const __m128i compressed = _mm_shuffle_epi8(lane2, sh);

      _mm_storeu_si128((__m128i *)(output + 2 * 16), compressed);
      _mm_storeu_si128((__m128i *)(output + 3 * 16 - 1), lane3);
    } break;
    case 0b11: {
      const __m128i lane2 = _mm256_extracti128_si256(chunks[1], 0);
      const __m128i lane3 = _mm256_extracti128_si256(chunks[1], 1);

      _mm256_storeu_si256((__m256i *)(output + 0 * 16), chunks[0]);
      _mm_storeu_si128((__m128i *)(output + 2 * 16), lane2);

      const __m128i v0 = _mm_set1_epi8(char(pos - 1));
      const __m128i v1 =
          _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      const __m128i v2 = _mm_cmpgt_epi8(v1, v0);
      const __m128i sh = _mm_sub_epi8(v1, v2);
      const __m128i compressed = _mm_shuffle_epi8(lane3, sh);

      _mm_storeu_si128((__m128i *)(output + 3 * 16), compressed);
    } break;
    }

    return 63;
  }
};
