std::pair<const char32_t*, char*> sse_convert_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) {

  const char32_t* end = buf + len;

  return std::make_pair(buf, utf8_output);
}