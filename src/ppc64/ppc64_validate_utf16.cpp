template <endianness big_endian>
simd8<uint8_t> utf16_gather_high_bytes(const simd16<uint16_t> in0,
                                       const simd16<uint16_t> in1) {
  if (big_endian) {
    const vec_u8_t pack_high = {
        0,  2,  4,  6,  8,  10, 12, 14, // in0
        16, 18, 20, 22, 24, 26, 28, 30  // in1
    };

    return vec_perm(vec_u8_t(in0.value), vec_u8_t(in1.value), pack_high);
  } else {
    const vec_u8_t pack_high = {
        1,  3,  5,  7,  9,  11, 13, 15, // in0
        17, 19, 21, 23, 25, 27, 29, 31  // in1
    };

    return vec_perm(vec_u8_t(in0.value), vec_u8_t(in1.value), pack_high);
  }
}
