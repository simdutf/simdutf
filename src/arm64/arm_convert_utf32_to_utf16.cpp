std::pair<const char32_t*, char16_t*> arm_convert_utf32_to_utf16(const char32_t* buf, size_t len, char16_t* utf16_out) {
  uint8_t * utf16_output = reinterpret_cast<uint8_t*>(utf16_out);
  const char32_t* end = buf + len;

  return std::make_pair(buf, reinterpret_cast<char16_t*>(utf16_output));
}