std::pair<const char *, char32_t *>
ppc64_convert_latin1_to_utf32(const char *buf, size_t len,
                              char32_t *utf32_output) {
  const char *end = buf + len;

  constexpr const size_t N = vector_u8::ELEMENTS;

  while (buf + N <= end) {
    const auto in = vector_u8::load(buf);
    in.store_bytes_as_utf32(utf32_output);

    buf += N;
    utf32_output += N;
  }

  return std::make_pair(buf, utf32_output);
}
