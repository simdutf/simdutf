std::pair<const char *, char32_t *>
ppc64_convert_latin1_to_utf32(const char *buf, size_t len,
                              char32_t *utf32_output) {
  const size_t rounded_len = align_down<vector_u8::ELEMENTS>(len);

  for (size_t i = 0; i < rounded_len; i += vector_u8::ELEMENTS) {
    const auto in = vector_u8::load(&buf[i]);
    in.store_bytes_as_utf32(&utf32_output[i]);
  }

  return std::make_pair(buf + rounded_len, utf32_output + rounded_len);
}
