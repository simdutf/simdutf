#ifndef SIMDUTF_LATIN1_TO_UTF8_H
#define SIMDUTF_LATIN1_TO_UTF8_H

namespace simdutf {
namespace scalar {
namespace {
namespace latin1_to_utf8 {

inline size_t convert(const char* buf, size_t len, char* utf8_output) {
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
				*utf8_output++ = char(buf[pos+1]);
        pos += 8;
        continue;
      }
    } */

    // try to convert the next block of 16 ASCII bytes
    if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that they are ascii
      uint64_t v1;
      ::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2}; //We're only interested in these bits: 1000 1000 1000 1000, so it makes sense to concatenate everything
      if ((v & 0x8080808080808080) == 0) { //if NONE of these are set, e.g. all of them are zero, then everything is ASCII
        size_t final_pos = pos + 16;
        while(pos < final_pos) {
          *utf8_output++ = char(buf[pos]);
          pos++;
        }
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
  return utf8_output - start;
}


//Do we have a need for this?

/* inline result convert_with_errors(const char* buf, size_t len, char* utf8_output) {
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
          *utf8_output++ = char(buf[pos]);
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

  return result(error_code::SUCCESS, utf8_output - start);
}
 */


} // latin1_to_utf8 namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif