template <endianness big_endian>
simd8<uint8_t> utf16_gather_high_bytes(const simd16<uint16_t> in0,
                                       const simd16<uint16_t> in1) {
  if (big_endian) {
    const auto mask = simd16<uint16_t>(0x00ff);
    const auto t0 = in0 & mask;
    const auto t1 = in1 & mask;

    return simd16<uint16_t>::pack(t0, t1);
  } else {
    return simd16<uint16_t>::pack_shifted_right<8>(in0, in1);
  }
}
