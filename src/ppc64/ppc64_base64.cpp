/*
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
 *
 * AMD XOP specific: http://0x80.pl/notesen/2016-01-12-sse-base64-encoding.html
 * Altivec has capabilites of AMD XOP (or vice versa): shuffle using 2 vectors
 * and variable shifts, thus this implementation shares some code solution
 * (modulo intrisic function names).
 */

constexpr bool with_base64_std = false;
constexpr bool with_base64_url = true;
constexpr bool with_ignore_errors = true;
constexpr bool with_ignore_garbage = true;
constexpr bool with_strict_checking = false;

// --- encoding -----------------------------------------------

/*
    Procedure translates vector of bytes having 6-bit values
    into ASCII counterparts.
*/
template <bool base64_url>
vector_u8 encoding_translate_6bit_values(const vector_u8 input) {
  // credit: Wojciech Muła
  // reduce  0..51 -> 0
  //        52..61 -> 1 .. 10
  //            62 -> 11
  //            63 -> 12
  auto result = input.saturating_sub(vector_u8::splat(51));

  // distinguish between ranges 0..25 and 26..51:
  //         0 .. 25 -> remains 13
  //        26 .. 51 -> becomes 0
  const auto lt = input < vector_u8::splat(26);
  result = select(as_vector_u8(lt), vector_u8::splat(13), result);

  const auto shift_LUT =
      base64_url ? vector_u8('a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                             '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                             '0' - 52, '-' - 62, '_' - 63, 'A', 0, 0)
                 : vector_u8('a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                             '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                             '0' - 52, '+' - 62, '/' - 63, 'A', 0, 0);
  // read shift
  result = result.lookup_16(shift_LUT);

  return input + result;
}

/*
   Procedure expands 12 bytes (4*3 bytes) into 16 bytes,
   each byte stores 6 bits of data
*/
template <typename = void>
simdutf_really_inline vector_u8 encoding_expand_6bit_fields(vector_u8 input) {
#if SIMDUTF_IS_BIG_ENDIAN
  #define indices4(dx) (dx + 0), (dx + 1), (dx + 1), (dx + 2)
  const auto expand_3_to_4 = vector_u8(indices4(0 * 3), indices4(1 * 3),
                                       indices4(2 * 3), indices4(3 * 3));
  #undef indices4

  // input = [........|ccdddddd|bbbbcccc|aaaaaabb] as uint8_t
  //              3        2        1        0
  //
  // in'   = [aaaaaabb|bbbbcccc|bbbbcccc|ccdddddd] as uint32_t
  //              0        1        1        2
  const auto in = as_vector_u32(expand_3_to_4.lookup_16(input));

  // t0    = [00000000|00000000|00000000|00dddddd]
  const auto t0 = in & uint32_t(0x0000003f);

  // t1    = [00000000|00000000|00cccccc|00dddddd]
  const auto t1 = select(uint32_t(0x00003f00), in.shl<2>(), t0);

  // t2    = [00000000|00bbbbbb|00cccccc|00dddddd]
  const auto t2 = select(uint32_t(0x003f0000), in.shr<4>(), t1);

  // t3    = [00aaaaaa|00bbbbbb|00cccccc|00dddddd]
  const auto t3 = select(uint32_t(0x3f000000), in.shr<2>(), t2);

  return as_vector_u8(t3);
#else
  #define indices4(dx) (dx + 1), (dx + 0), (dx + 2), (dx + 1)
  const auto expand_3_to_4 = vector_u8(indices4(0 * 3), indices4(1 * 3),
                                       indices4(2 * 3), indices4(3 * 3));
  #undef indices4

  // input = [........|ccdddddd|bbbbcccc|aaaaaabb] as uint8_t
  //              3        2        1        0
  //
  // in'   = [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc] as uint32_t
  //              1        2        0        1
  const auto in = as_vector_u32(expand_3_to_4.lookup_16(input));

  // t0    = [00dddddd|00000000|00000000|00000000]
  const auto t0 = in.shl<8>() & uint32_t(0x3f000000);

  // t1    = [00dddddd|00cccccc|00000000|00000000]
  const auto t1 = select(uint32_t(0x003f0000), in.shr<6>(), t0);

  // t2    = [00dddddd|00cccccc|00bbbbbb|00000000]
  const auto t2 = select(uint32_t(0x00003f00), in.shl<4>(), t1);

  // t3    = [00dddddd|00cccccc|00bbbbbb|00aaaaaa]
  const auto t3 = select(uint32_t(0x0000003f), in.shr<10>(), t2);

  return as_vector_u8(t3);
#endif // SIMDUTF_IS_BIG_ENDIAN
}

template <bool isbase64url>
size_t encode_base64(char *dst, const char *src, size_t srclen,
                     base64_options options) {

  const uint8_t *input = (const uint8_t *)src;

  uint8_t *out = (uint8_t *)dst;

  size_t i = 0;
  for (; i + 52 <= srclen; i += 48) {
    const auto in0 = vector_u8::load(input + i + 12 * 0);
    const auto in1 = vector_u8::load(input + i + 12 * 1);
    const auto in2 = vector_u8::load(input + i + 12 * 2);
    const auto in3 = vector_u8::load(input + i + 12 * 3);

    const auto expanded0 = encoding_expand_6bit_fields(in0);
    const auto expanded1 = encoding_expand_6bit_fields(in1);
    const auto expanded2 = encoding_expand_6bit_fields(in2);
    const auto expanded3 = encoding_expand_6bit_fields(in3);

    const auto base64_0 =
        encoding_translate_6bit_values<isbase64url>(expanded0);
    const auto base64_1 =
        encoding_translate_6bit_values<isbase64url>(expanded1);
    const auto base64_2 =
        encoding_translate_6bit_values<isbase64url>(expanded2);
    const auto base64_3 =
        encoding_translate_6bit_values<isbase64url>(expanded3);

    base64_0.store(out);
    out += 16;

    base64_1.store(out);
    out += 16;

    base64_2.store(out);
    out += 16;

    base64_3.store(out);
    out += 16;
  }
  for (; i + 16 <= srclen; i += 12) {
    const auto in = vector_u8::load(input + i);
    const auto expanded = encoding_expand_6bit_fields(in);
    const auto base64 = encoding_translate_6bit_values<isbase64url>(expanded);

    base64.store(out);
    out += 16;
  }

  return i / 3 * 4 + scalar::base64::tail_encode_base64((char *)out, src + i,
                                                        srclen - i, options);
}

// --- decoding -----------------------------------------------

static simdutf_really_inline void compress(const vector_u8 data, uint16_t mask,
                                           char *output) {
  if (mask == 0) {
    data.store(output);
    return;
  }

  // this particular implementation was inspired by work done by @animetosho
  // we do it in two steps, first 8 bytes and then second 8 bytes
  uint8_t mask1 = uint8_t(mask);      // least significant 8 bits
  uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
  // next line just loads the 64-bit values thintable_epi8[mask1] and
  // thintable_epi8[mask2] into a 128-bit register, using only
  // two instructions on most compilers.

#if SIMDUTF_IS_BIG_ENDIAN
  vec_u64_t tmp = {
      tables::base64::thintable_epi8[mask2],
      tables::base64::thintable_epi8[mask1],
  };

  auto shufmask = vector_u8(vec_reve(vec_u8_t(tmp)));

  // we increment by 0x08 the second half of the mask
  shufmask =
      shufmask + vector_u8(0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8);
#else
  vec_u64_t tmp = {
      tables::base64::thintable_epi8[mask1],
      tables::base64::thintable_epi8[mask2],
  };

  auto shufmask = vector_u8(vec_u8_t(tmp));

  // we increment by 0x08 the second half of the mask
  shufmask =
      shufmask + vector_u8(0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8);
#endif // SIMDUTF_IS_BIG_ENDIAN

  // this is the version "nearly pruned"
  const auto pruned = shufmask.lookup_16(data);
  // we still need to put the two halves together.
  // we compute the popcount of the first half:
  const int pop1 = tables::base64::BitsSetTable256mul2[mask1];
  // then load the corresponding mask, what it does is to write
  // only the first pop1 bytes from the first 8 bytes, and then
  // it fills in with the bytes from the second 8 bytes + some filling
  // at the end.
  const auto compactmask =
      vector_u8::load(tables::base64::pshufb_combine_table + pop1 * 8);

  const auto answer = compactmask.lookup_16(pruned);

  answer.store(output);
}

static simdutf_really_inline vector_u8 decoding_pack(vector_u8 input) {
#if SIMDUTF_IS_BIG_ENDIAN
  // in   = [00aaaaaa|00bbbbbb|00cccccc|00dddddd]
  // want = [00000000|aaaaaabb|bbbbcccc|ccdddddd]

  auto in = as_vector_u16(input);
  // t0   = [00??aaaa|aabbbbbb|00??cccc|ccdddddd]
  const auto t0 = in.shr<2>();
  const auto t1 = select(uint16_t(0x0fc0), t0, in);

  // t0   = [00??????|aaaaaabb|bbbbcccc|ccdddddd]
  const auto t2 = as_vector_u32(t1);
  const auto t3 = t2.shr<4>();
  const auto t4 = select(uint32_t(0x00fff000), t3, t2);

  const auto tmp = as_vector_u8(t4);

  const auto shuffle =
      vector_u8(1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 0, 0, 0, 0);

  const auto t = shuffle.lookup_16(tmp);

  return t;
#else
  // in   = [00dddddd|00cccccc|00bbbbbb|00aaaaaa]
  // want = [00000000|aaaaaabb|bbbbcccc|ccdddddd]

  auto u = as_vector_u32(input).swap_bytes();

  auto in = vector_u16((vec_u16_t)u.value);
  // t0   = [00??aaaa|aabbbbbb|00??cccc|ccdddddd]
  const auto t0 = in.shr<2>();
  const auto t1 = select(uint16_t(0x0fc0), t0, in);

  // t0   = [00??????|aaaaaabb|bbbbcccc|ccdddddd]
  const auto t2 = as_vector_u32(t1);
  const auto t3 = t2.shr<4>();
  const auto t4 = select(uint32_t(0x00fff000), t3, t2);

  const auto tmp = as_vector_u8(t4);

  const auto shuffle =
      vector_u8(2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, 0, 0, 0, 0);

  const auto t = shuffle.lookup_16(tmp);

  return t;
#endif // SIMDUTF_IS_BIG_ENDIAN
}
static simdutf_really_inline void base64_decode(char *out, vector_u8 input) {
  const auto expanded = decoding_pack(input);
  expanded.store(out);
}

static simdutf_really_inline void base64_decode_block(char *out,
                                                      const char *src) {
  base64_decode(out + 12 * 0, vector_u8::load(src + 0 * 16));
  base64_decode(out + 12 * 1, vector_u8::load(src + 1 * 16));
  base64_decode(out + 12 * 2, vector_u8::load(src + 2 * 16));
  base64_decode(out + 12 * 3, vector_u8::load(src + 3 * 16));
}

static simdutf_really_inline void base64_decode_block_safe(char *out,
                                                           const char *src) {
  base64_decode(out + 12 * 0, vector_u8::load(src + 0 * 16));
  base64_decode(out + 12 * 1, vector_u8::load(src + 1 * 16));
  base64_decode(out + 12 * 2, vector_u8::load(src + 2 * 16));

  char buffer[16];
  base64_decode(buffer, vector_u8::load(src + 3 * 16));
  std::memcpy(out + 36, buffer, 12);
}

// ---base64 decoding::block64 class --------------------------

class block64 {
  simd8x64<uint8_t> b;

public:
  simdutf_really_inline block64(const char *src) : b(load_block(src)) {}
  simdutf_really_inline block64(const char16_t *src) : b(load_block(src)) {}

private:
  // The caller of this function is responsible to ensure that there are 64
  // bytes available from reading at src. The data is read into a block64
  // structure.
  static simdutf_really_inline simd8x64<uint8_t> load_block(const char *src) {
    const auto v0 = vector_u8::load(src + 16 * 0);
    const auto v1 = vector_u8::load(src + 16 * 1);
    const auto v2 = vector_u8::load(src + 16 * 2);
    const auto v3 = vector_u8::load(src + 16 * 3);

    return simd8x64<uint8_t>(v0, v1, v2, v3);
  }

  // The caller of this function is responsible to ensure that there are 128
  // bytes available from reading at src. The data is read into a block64
  // structure.
  static simdutf_really_inline simd8x64<uint8_t>
  load_block(const char16_t *src) {
    const auto m1 = vector_u16::load(src + 8 * 0);
    const auto m2 = vector_u16::load(src + 8 * 1);
    const auto m3 = vector_u16::load(src + 8 * 2);
    const auto m4 = vector_u16::load(src + 8 * 3);
    const auto m5 = vector_u16::load(src + 8 * 4);
    const auto m6 = vector_u16::load(src + 8 * 5);
    const auto m7 = vector_u16::load(src + 8 * 6);
    const auto m8 = vector_u16::load(src + 8 * 7);

    return simd8x64<uint8_t>(vector_u16::pack(m1, m2), vector_u16::pack(m3, m4),
                             vector_u16::pack(m5, m6),
                             vector_u16::pack(m7, m8));
  }

public:
  template <bool base64_url, bool ignore_garbage, bool default_or_url>
  static inline uint16_t to_base64_mask(vector_u8 &src, uint16_t &error) {
    const auto ascii_space_tbl =
        vector_u8(0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x9, 0xa, 0x0,
                  0xc, 0xd, 0x0, 0x0);

    // credit: aqrit
    const auto delta_asso =
        default_or_url
            ? vector_u8(0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x16)
            : vector_u8(0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F);

    const auto delta_values =
        default_or_url
            ? vector_u8(0xBF, 0xE0, 0xB9, 0x13, 0x04, 0xBF, 0xBF, 0xB9, 0xB9,
                        0x00, 0xFF, 0x11, 0xFF, 0xBF, 0x10, 0xB9)
            : (base64_url
                   ? vector_u8(0x0, 0x0, 0x0, 0x13, 0x4, 0xBF, 0xBF, 0xB9, 0xB9,
                               0x0, 0x11, 0xC3, 0xBF, 0xE0, 0xB9, 0xB9)
                   : vector_u8(0x00, 0x00, 0x00, 0x13, 0x04, 0xBF, 0xBF, 0xB9,
                               0xB9, 0x00, 0x10, 0xC3, 0xBF, 0xBF, 0xB9, 0xB9));

    const auto check_asso =
        default_or_url
            ? vector_u8(0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                        0x01, 0x03, 0x07, 0x0B, 0x0E, 0x0B, 0x06)
            : (base64_url
                   ? vector_u8(0xD, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
                               0x3, 0x7, 0xB, 0xE, 0xB, 0x6)
                   : vector_u8(0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                               0x01, 0x01, 0x03, 0x07, 0x0B, 0x0B, 0x0B, 0x0F));

    const auto check_values =
        default_or_url
            ? vector_u8(0x80, 0x80, 0x80, 0x80, 0xCF, 0xBF, 0xD5, 0xA6, 0xB5,
                        0xA1, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80)
            : (base64_url
                   ? vector_u8(0x80, 0x80, 0x80, 0x80, 0xCF, 0xBF, 0xB6, 0xA6,
                               0xB5, 0xA1, 0x0, 0x80, 0x0, 0x80, 0x0, 0x80)
                   : vector_u8(0x80, 0x80, 0x80, 0x80, 0xCF, 0xBF, 0xD5, 0xA6,
                               0xB5, 0x86, 0xD1, 0x80, 0xB1, 0x80, 0x91, 0x80));

    const auto shifted = src.shr<3>();

    const auto delta_hash = avg(src.lookup_16(delta_asso), shifted);
    const auto check_hash = avg(src.lookup_16(check_asso), shifted);

    const auto out = as_vector_i8(delta_hash.lookup_16(delta_values))
                         .saturating_add(as_vector_i8(src));
    const auto chk = as_vector_i8(check_hash.lookup_16(check_values))
                         .saturating_add(as_vector_i8(src));

    const uint16_t mask = chk.to_bitmask();
    if (!ignore_garbage && mask) {
      const auto ascii = src.lookup_16(ascii_space_tbl);
      const auto ascii_space = (ascii == src);
      error = (mask ^ ascii_space.to_bitmask());
    }
    src = out;

    return mask;
  }

  template <bool base64_url, bool ignore_garbage, bool default_or_url>
  simdutf_really_inline uint64_t to_base64_mask(uint64_t *error) {
    uint16_t err0 = 0;
    uint16_t err1 = 0;
    uint16_t err2 = 0;
    uint16_t err3 = 0;
    uint64_t m0 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        b.chunks[0], err0);
    uint64_t m1 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        b.chunks[1], err1);
    uint64_t m2 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        b.chunks[2], err2);
    uint64_t m3 = to_base64_mask<base64_url, ignore_garbage, default_or_url>(
        b.chunks[3], err3);

    if (!ignore_garbage) {
      *error = (err0) | ((uint64_t)err1 << 16) | ((uint64_t)err2 << 32) |
               ((uint64_t)err3 << 48);
    }
    return m0 | (m1 << 16) | (m2 << 32) | (m3 << 48);
  }

  simdutf_really_inline void copy_block(char *output) {
    b.store(reinterpret_cast<uint8_t *>(output));
  }

  simdutf_really_inline uint64_t compress_block(uint64_t mask, char *output) {
    uint64_t nmask = ~mask;
    compress(b.chunks[0], uint16_t(mask), output);
    compress(b.chunks[1], uint16_t(mask >> 16),
             output + count_ones(nmask & 0xFFFF));
    compress(b.chunks[2], uint16_t(mask >> 32),
             output + count_ones(nmask & 0xFFFFFFFF));
    compress(b.chunks[3], uint16_t(mask >> 48),
             output + count_ones(nmask & 0xFFFFFFFFFFFFULL));
    return count_ones(nmask);
  }

  simdutf_really_inline void base64_decode_block(char *out) {
    base64_decode(out + 12 * 0, b.chunks[0]);
    base64_decode(out + 12 * 1, b.chunks[1]);
    base64_decode(out + 12 * 2, b.chunks[2]);
    base64_decode(out + 12 * 3, b.chunks[3]);
  }

  simdutf_really_inline void base64_decode_block_safe(char *out) {
    base64_decode(out + 12 * 0, b.chunks[0]);
    base64_decode(out + 12 * 1, b.chunks[1]);
    base64_decode(out + 12 * 2, b.chunks[2]);
    char buffer[16];
    base64_decode(buffer, b.chunks[3]);
    std::memcpy(out + 12 * 3, buffer, 12);
  }
};
