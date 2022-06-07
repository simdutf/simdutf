std::pair<const char32_t*, char16_t*> avx2_convert_utf32_to_utf16(const char32_t* buf, size_t len, char16_t* utf16_output) {
  const char32_t* end = buf + len;

  return std::make_pair(buf, utf16_output);
}