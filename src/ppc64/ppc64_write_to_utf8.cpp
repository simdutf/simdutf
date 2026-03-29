/*
 * reads a vector of uint16 values
 * bits after 11th are ignored
 * first 11 bits are encoded into utf8
 * !important! utf8_output must have at least 16 writable bytes
 */
simdutf_really_inline void
write_v_u16_11bits_to_utf8(const vector_u16 v_u16, char *&utf8_output,
                           const vector_u8 one_byte_bytemask,
                           const uint16_t one_byte_bitmask) {

  // 0b1100_0000_1000_0000
  const auto v_c080 = vector_u16(0xc080);
  // 0b0011_1111_0000_0000
  const auto v_1f00 = vector_u16(0x1f00);
  // 0b0000_0000_0011_1111
  const auto v_003f = vector_u16(0x003f);

  // 1. prepare 2-byte values
  // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
  // expected output   : [110a|aaaa|10bb|bbbb] x 8

  // t0 = [0000|0000|00bb|bbbb]
  const auto t0 = v_u16 & v_003f;
  // t1 = [000a|aaaa|bbbb|bb00]
  const auto t1 = v_u16.shl<2>();
  // t2 = [000a|aaaa|00bb|bbbb]
  const auto t2 = select(v_1f00, t1, t0);
  // t3 = [110a|aaaa|10bb|bbbb]
  const auto t3 = t2 | v_c080;

  // 2. merge ASCII and 2-byte codewords
  const auto utf8_unpacked1 =
      select(one_byte_bytemask, as_vector_u8(v_u16), as_vector_u8(t3));

#if SIMDUTF_IS_BIG_ENDIAN
  const auto tmp = as_vector_u16(utf8_unpacked1).swap_bytes();
#else
  const auto tmp = as_vector_u16(utf8_unpacked1);
#endif // SIMDUTF_IS_BIG_ENDIAN
  const auto utf8_unpacked = as_vector_u8(tmp);

  // 3. prepare bitmask for 8-bit lookup
  //    one_byte_bitmask = hhggffeeddccbbaa -- the bits are doubled (h - MSB, a
  //    - LSB)
  const uint16_t m0 = one_byte_bitmask & 0x5555;      // m0 = 0h0g0f0e0d0c0b0a
  const uint16_t m1 = static_cast<uint16_t>(m0 >> 7); // m1 = 00000000h0g0f0e0
  const uint8_t m2 = static_cast<uint8_t>((m0 | m1) & 0xff); // m2 = hdgcfbea
  // 4. pack the bytes
  const uint8_t *row =
      &simdutf::tables::utf16_to_utf8::pack_1_2_utf8_bytes[m2][0];
  const auto shuffle = vector_u8::load(row + 1);
  const auto utf8_packed = shuffle.lookup_16(utf8_unpacked);

  // 5. store bytes
  utf8_packed.store(utf8_output);

  // 6. adjust pointers
  utf8_output += row[0];
}

inline void write_v_u16_11bits_to_utf8(const vector_u16 v_u16,
                                       char *&utf8_output,
                                       const vector_u16 v_0000,
                                       const vector_u16 v_ff80) {
  // no bits set above 7th bit
  const auto one_byte_bytemask = (v_u16 & v_ff80) == v_0000;
  const uint16_t one_byte_bitmask = one_byte_bytemask.to_bitmask();

  write_v_u16_11bits_to_utf8(v_u16, utf8_output,
                             as_vector_u8(one_byte_bytemask), one_byte_bitmask);
}
