template <endianness big_endian>
size_t ppc64_convert_latin1_to_utf16(const char *latin1_input, size_t len,
                                     char16_t *utf16_output) {
  const size_t rounded_len = len & ~0xF; // Round down to nearest multiple of 16

  using vector_u8 = simd8<uint8_t>;

  for (size_t i = 0; i < rounded_len; i += vector_u8::ELEMENTS) {
    const auto in = vector_u8::load(&latin1_input[i]);
    in.store_bytes_as_utf16<big_endian>(&utf16_output[i]);
  }

  return rounded_len;
}
