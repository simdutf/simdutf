namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_to_utf16 {

size_t scalar_convert_valid_utf8_to_utf16(const char* buf, size_t len, char16_t* utf16_output) {
 const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  size_t pos = 0;
  char16_t* start{utf16_output};
  while (pos < len) {
    // try to convert the next block of 16 ASCII bytes
    if (pos + 16 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
      uint64_t v1;
      ::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) == 0) {
        size_t final_pos = pos + 16;
        while(pos < final_pos) { 
          *utf16_output++ = char16_t(buf[pos]);
          pos++;
        }
        continue;
      }
    }
    uint8_t leading_byte = data[pos]; // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf16_output++ = char16_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8, it should become
      // a single UTF-16 word.
      if(pos + 1 > len) { break; } // minimal bound checking
      *utf16_output++ = char16_t(((leading_byte & 31) << 6) | (data[pos + 1] & 63));
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8, it should become
      // a single UTF-16 word.
      if(pos + 2 > len) { break; } // minimal bound checking
      *utf16_output++ = char16_t(((leading_byte & 15) << 6) | ((data[pos + 1] & 63) << 12) | (data[pos + 2] & 63));
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if(pos + 3 > len) { break; } // minimal bound checking
      uint32_t code_word = ((leading_byte & 7) << 18 )| ((data[pos + 1] & 63) << 12) 
                           | ((data[pos + 2] & 63) << 6) | (data[pos + 3] & 63);
      code_word -= 0x10000;
      *utf16_output++ = char16_t(0xD800 + (code_word >> 10));
      *utf16_output++ = char16_t(0xDC00 + (code_word & 0x3FF));
      pos += 4;
    } else {
      // we may have a continuation but we do not do error checking
    }
  }
  return utf16_output - start;
}


} // namespace utf8_to_utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
