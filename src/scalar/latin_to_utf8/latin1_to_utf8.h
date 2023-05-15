#ifndef SIMDUTF_LATIN1_TO_UTF8_H
#define SIMDUTF_LATIN1_TO_UTF8_H

namespace simdutf {
namespace scalar {
namespace {
namespace latin1_to_utf8 {

inline size_t latin_to_UTF8(const char* buf, size_t len, char* utf8_output) {
  const unsigned char *data = reinterpret_cast<const unsigned char *>(buf);
  size_t pos = 0;
  char* start{utf8_output};
  while (pos < len) {

    //To be completed later. 
/*     // try to convert the next block of 8 ASCII characters
    if (pos + 8 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
      uint64_t v;
      ::memcpy(&v, data + pos, sizeof(uint64_t));
      if ((v & 0x8080808080808080) == 0) {
        *utf8_output++ = char(buf[pos]);
				//*utf8_output++ = char(buf[pos+1]);
        pos += 8;
        continue;
      }
    } */
    unsigned char byte = data[pos];
    if((byte & 0x80)==0) { //if ASCII
      // will generate one UTF-8 bytes
      *utf8_output++ = char(byte);
      pos++;
    } else {
      // will generate two UTF-8 bytes
      *utf8_output++ = char((byte>>6) | 0b11000000); //
      *utf8_output++ = char((byte & 0b111111) | 0b10000000); //
      pos++;
    } else {return 0;}

  }
  return utf8_output - start;
}


inline result latin_to_UTF8(const char* buf, size_t len, char* utf8_output) {
  const unsigned char *data = reinterpret_cast<const unsigned char *>(buf);
  size_t pos = 0;
  char* start{utf8_output};
  while (pos < len) {

    //To be completed later. 
    // try to convert the next block of 8 ASCII characters
    if (pos + 16 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
      uint64_t v;
      ::memcpy(&v, data + pos, sizeof(uint64_t));
      if ((v & 0x8080808080808080) == 0) {
        size_t final_pos = pos + 16;
        while(pos < final_pos) {
          *latin_output++ = char(buf[pos]);
          pos++;
        }
        pos += 8;
        continue;
      }
    }
    unsigned char byte = data[pos];
    if((byte & 0x80)==0) { //if ASCII
      // will generate one UTF-8 bytes
      *utf8_output++ = char(byte);
      pos++;
    } else {
      // will generate two UTF-8 bytes
      *utf8_output++ = char((byte>>6) | 0b11000000); //
      *utf8_output++ = char((byte & 0b111111) | 0b10000000); //
      pos++;
}

  }

  return result(error_code::SUCCESS, utf16_output - start);
}

} // latin1_to_utf8 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif