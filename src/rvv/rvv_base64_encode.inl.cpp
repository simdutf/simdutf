/*
    This is based on proof-of-concept that lives at
    https://github.com/WojciechMula/base64simd/tree/master/encode.

    Note: an implementation worth checking is one that use
          segmented load.
*/

namespace base64_encoding {
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
//

// This limit is imposed by the fact we store indices as bytes.
// Thus maximum index would be reached for 255 / 3 = 85;
// 64 is the closest "nice" value.
constexpr size_t unpack_shuffle_size = 64;

const uint8_t unpack_shuffle[unpack_shuffle_size] = {
#define EXPAND(idx) (idx) + 1, (idx), (idx) + 2, (idx) + 1
    EXPAND(0 * 3),  EXPAND(1 * 3),  EXPAND(2 * 3),
    EXPAND(3 * 3),  EXPAND(4 * 3),  EXPAND(5 * 3),
    EXPAND(6 * 3),  EXPAND(7 * 3),  EXPAND(8 * 3),
    EXPAND(9 * 3),  EXPAND(10 * 3), EXPAND(11 * 3),
    EXPAND(12 * 3), EXPAND(13 * 3), EXPAND(14 * 3),
    EXPAND(15 * 3)
#undef EXPAND
};

constexpr size_t alphabet_size = 64;

const uint8_t std_alphabet[alphabet_size] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

const uint8_t url_alphabet[alphabet_size] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'};

template <typename vector_u8>
simdutf_really_inline vector_u8 translate_plain_gather(vector_u8 input,
                                                       vector_u8 alphabet,
                                                       size_t vl) {
  return __riscv_vrgather(alphabet, input, vl);
}

constexpr size_t shift_lut_size = 14;

const uint8_t shift_lut_std_alphabet[shift_lut_size] = {
    uint8_t('a' - 26), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52),
    uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52),
    uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('+' - 62),
    uint8_t('/' - 63)};

const uint8_t shift_lut_url_alphabet[shift_lut_size] = {
    uint8_t('a' - 26), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52),
    uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52),
    uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('-' - 62),
    uint8_t('_' - 63)};

/*
    The first 14 bytes of `shift_lut` have to contain the following values:

    * std alphabet: ['a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
   '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '+' - 62, '/' - 63]
    * url alphabet: ['a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
   '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '-' - 62, '_' - 63]
*/
template <typename vector_u8>
simdutf_really_inline vector_u8 translate_lookup(vector_u8 input,
                                                 vector_u8 shift_lut,
                                                 size_t vl) {
  // reduce  0..51 -> 0
  //        52..61 -> 1 .. 10
  //            62 -> 11
  //            63 -> 12
  const auto t0 = __riscv_vssubu(input, 51, vl);

  // distinguish between ranges 0..25 and 26..51:
  //         0 .. 25 -> becomes 13
  //        26 .. 51 -> remains 0
  const auto lt = __riscv_vmsltu(input, 26, vl);

  const auto t1 = __riscv_vadd_mu(lt, t0, t0, 13, vl);

  const auto shift = __riscv_vrgather(shift_lut, t1, vl);

  return __riscv_vadd(input, shift, vl);
}

template <bool use_url_alphabet, LMUL lmul, Base64EncodingProcedure procedure>
size_t perform(const uint8_t *input, size_t bytes, uint8_t *output, size_t step,
               size_t max_size, base64_options options) {
  using vector_u8 = typename vector_u8_type<lmul>::type;
  using vector_u16 = typename vector_u16_type<lmul>::type;
  using vector_u32 = typename vector_u32_type<lmul>::type;

  const auto shuf = rvv_vle8_v<vector_u8>(unpack_shuffle, unpack_shuffle_size);

  vector_u8 lookup;
  switch (procedure) {
  case Base64EncodingProcedure::PlainGatherM1:
  case Base64EncodingProcedure::PlainGatherM2:
  case Base64EncodingProcedure::PlainGatherM4:
  case Base64EncodingProcedure::PlainGatherM8:
    if (use_url_alphabet) {
      lookup = rvv_vle8_v<vector_u8>(url_alphabet, alphabet_size);
    } else {
      lookup = rvv_vle8_v<vector_u8>(std_alphabet, alphabet_size);
    }
    break;

  case Base64EncodingProcedure::LookupM1:
  case Base64EncodingProcedure::LookupM2:
  case Base64EncodingProcedure::LookupM4:
  case Base64EncodingProcedure::LookupM8:
    if (use_url_alphabet) {
      lookup = rvv_vle8_v<vector_u8>(shift_lut_url_alphabet, shift_lut_size);
    } else {
      lookup = rvv_vle8_v<vector_u8>(shift_lut_std_alphabet, shift_lut_size);
    }
    break;
  } // switch

  const uint8_t *out_start = output;
  while (bytes >= step) {
    (void)rvv_vsetvl<vector_u8>(step);

    const auto in = rvv_vle8_v<vector_u8>(input, step);
    input += step;
    bytes -= step;

    const size_t vl = rvv_vsetvl<vector_u8>(max_size);

    // in32  = [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]
    const vector_u8 tmp = __riscv_vrgather(in, shuf, vl);
    const vector_u32 in32 = rvv_reinterpret<vector_u8, vector_u32>(tmp);

    // unpacking

    // ca_t0 = [0000cccc|cc000000|aaaaaa00|00000000]
    const vector_u32 ca32_t0 = __riscv_vand(in32, 0x0fc0fc00, vl);
    const vector_u16 ca_t0 = rvv_reinterpret<vector_u32, vector_u16>(ca32_t0);
    const vector_u32 ca32_shift = rvv_splat<vector_u32>(0x0006000a, vl);
    const vector_u16 ca_shift =
        rvv_reinterpret<vector_u32, vector_u16>(ca32_shift);

    // ca_t1 = [00000000|00cccccc|00000000|00aaaaaa]
    //          field c >> 6      field a >> 10
    const vector_u16 ca_t1 = __riscv_vsrl(ca_t0, ca_shift, vl);

    // db_t0  = [00000000|00dddddd|000000bb|bbbb0000]
    const vector_u32 db32_t0 = __riscv_vand(in32, 0x003f03f0, vl);
    const vector_u16 db_t0 = rvv_reinterpret<vector_u32, vector_u16>(db32_t0);
    const vector_u32 db32_shift = rvv_splat<vector_u32>(0x00080004, vl);
    const vector_u16 db_shift =
        rvv_reinterpret<vector_u32, vector_u16>(db32_shift);

    // db_t1  = [00dddddd|00000000|00bbbbbb|00000000](
    //          field d << 8       field b << 4
    const vector_u16 db_t1 = __riscv_vsll(db_t0, db_shift, vl);

    // res   = [00dddddd|00cccccc|00bbbbbb|00aaaaaa] = t1 | t3
    const vector_u16 indices16 = __riscv_vor(ca_t1, db_t1, vl);
    const vector_u8 indices = rvv_reinterpret<vector_u16, vector_u8>(indices16);

    vector_u8 result;
    switch (procedure) {
    case Base64EncodingProcedure::PlainGatherM1:
    case Base64EncodingProcedure::PlainGatherM2:
    case Base64EncodingProcedure::PlainGatherM4:
    case Base64EncodingProcedure::PlainGatherM8:
      result = translate_plain_gather<vector_u8>(indices, lookup, vl);
      break;

    case Base64EncodingProcedure::LookupM1:
    case Base64EncodingProcedure::LookupM2:
    case Base64EncodingProcedure::LookupM4:
    case Base64EncodingProcedure::LookupM8:
      result = translate_lookup<vector_u8>(indices, lookup, vl);
      break;
    }

    rvv_vse8_v<vector_u8>(output, result, vl);
    output += vl;
  }

  const size_t saved_bytes = output - out_start;

  return saved_bytes + scalar::base64::tail_encode_base64(
                           reinterpret_cast<char *>(output),
                           reinterpret_cast<const char *>(input), bytes,
                           options);
}
} // namespace base64_encoding

// ------------------------------------------------------------

Base64Encoding::Base64Encoding()
    : procedure(Base64EncodingProcedure::Scalar), step(8), max_size(16) {

  const size_t vl_m8 = __riscv_vsetvlmax_e8m8();
  const size_t vl_m4 = __riscv_vsetvlmax_e8m4();
  const size_t vl_m2 = __riscv_vsetvlmax_e8m2();
  const size_t vl_m1 = __riscv_vsetvlmax_e8m1();

  if (vl_m1 >= 64) {
    procedure = Base64EncodingProcedure::PlainGatherM1;
    max_size = 64;
  } else if (vl_m2 >= 64) {
    procedure = Base64EncodingProcedure::PlainGatherM2;
    max_size = 64;
  } else if (vl_m4 >= 64) {
    procedure = Base64EncodingProcedure::PlainGatherM4;
    max_size = 64;
  } else if (vl_m8 >= 64) {
    procedure = Base64EncodingProcedure::PlainGatherM8;
    max_size = 64;
  } else if (vl_m8 >= 16) {
    procedure = Base64EncodingProcedure::LookupM8;
    max_size = 16;
  } else if (vl_m4 >= 16) {
    procedure = Base64EncodingProcedure::LookupM4;
    max_size = 16;
  } else if (vl_m2 >= 16) {
    procedure = Base64EncodingProcedure::LookupM2;
    max_size = 16;
  } else if (vl_m1 >= 16) {
    procedure = Base64EncodingProcedure::LookupM1;
    max_size = 16;
  }

  step = 3 * max_size / 4;
}

size_t Base64Encoding::perform(const uint8_t *input, size_t bytes,
                               uint8_t *output, base64_options options) const {
  using Procedure = Base64EncodingProcedure;

  if (options & base64_url) {
    switch (procedure) {
    case Procedure::PlainGatherM1:
      return base64_encoding::perform<Base64UrlAlphabet, LMUL::M1,
                                      Procedure::PlainGatherM1>(
          input, bytes, output, step, max_size, options);
    case Procedure::PlainGatherM2:
      return base64_encoding::perform<Base64UrlAlphabet, LMUL::M2,
                                      Procedure::PlainGatherM2>(
          input, bytes, output, step, max_size, options);
    case Procedure::PlainGatherM4:
      return base64_encoding::perform<Base64UrlAlphabet, LMUL::M4,
                                      Procedure::PlainGatherM4>(
          input, bytes, output, step, max_size, options);
    case Procedure::PlainGatherM8:
      return base64_encoding::perform<Base64UrlAlphabet, LMUL::M4,
                                      Procedure::PlainGatherM8>(
          input, bytes, output, step, max_size, options);
    case Procedure::LookupM1:
      return base64_encoding::perform<Base64UrlAlphabet, LMUL::M1,
                                      Procedure::LookupM1>(
          input, bytes, output, step, max_size, options);
    case Procedure::LookupM2:
      return base64_encoding::perform<Base64UrlAlphabet, LMUL::M2,
                                      Procedure::LookupM2>(
          input, bytes, output, step, max_size, options);
    case Procedure::LookupM4:
      return base64_encoding::perform<Base64UrlAlphabet, LMUL::M4,
                                      Procedure::LookupM4>(
          input, bytes, output, step, max_size, options);
    case Procedure::LookupM8:
      return base64_encoding::perform<Base64UrlAlphabet, LMUL::M4,
                                      Procedure::LookupM8>(
          input, bytes, output, step, max_size, options);
    case Procedure::Scalar:
      return scalar::base64::tail_encode_base64(
          reinterpret_cast<char *>(output),
          reinterpret_cast<const char *>(input), bytes, options);
    }
  } else {
    switch (procedure) {
    case Procedure::PlainGatherM1:
      return base64_encoding::perform<Base64StandardAlphabet, LMUL::M1,
                                      Procedure::PlainGatherM1>(
          input, bytes, output, step, max_size, options);
    case Procedure::PlainGatherM2:
      return base64_encoding::perform<Base64StandardAlphabet, LMUL::M2,
                                      Procedure::PlainGatherM2>(
          input, bytes, output, step, max_size, options);
    case Procedure::PlainGatherM4:
      return base64_encoding::perform<Base64StandardAlphabet, LMUL::M4,
                                      Procedure::PlainGatherM4>(
          input, bytes, output, step, max_size, options);
    case Procedure::PlainGatherM8:
      return base64_encoding::perform<Base64StandardAlphabet, LMUL::M4,
                                      Procedure::PlainGatherM8>(
          input, bytes, output, step, max_size, options);
    case Procedure::LookupM1:
      return base64_encoding::perform<Base64StandardAlphabet, LMUL::M1,
                                      Procedure::LookupM1>(
          input, bytes, output, step, max_size, options);
    case Procedure::LookupM2:
      return base64_encoding::perform<Base64StandardAlphabet, LMUL::M2,
                                      Procedure::LookupM2>(
          input, bytes, output, step, max_size, options);
    case Procedure::LookupM4:
      return base64_encoding::perform<Base64StandardAlphabet, LMUL::M4,
                                      Procedure::LookupM4>(
          input, bytes, output, step, max_size, options);
    case Procedure::LookupM8:
      return base64_encoding::perform<Base64StandardAlphabet, LMUL::M4,
                                      Procedure::LookupM8>(
          input, bytes, output, step, max_size, options);
    case Procedure::Scalar:
      return scalar::base64::tail_encode_base64(
          reinterpret_cast<char *>(output),
          reinterpret_cast<const char *>(input), bytes, options);
    }
  }

  return 0;
}
