#ifndef SIMDUTF_ERROR_H
#define SIMDUTF_ERROR_H
namespace simdutf {

enum error_code {
  SUCCESS = 0,
  HEADER_BITS,  // Any byte must have fewer than 5 header bits.
  TOO_SHORT,    // The leading byte must be followed by N-1 continuation bytes, where N is the UTF-8 character length
                // This is also the error when the input is truncated.
  TOO_LONG,     // We either have too many consecutive continuation bytes or the string starts with a continuation byte.
  OVERLONG,     // The decoded character must be above U+7F for two-byte characters, U+7FF for three-byte characters,
                // and U+FFFF for four-byte characters.
  TOO_LARGE,    // The decoded character must be less than or equal to U+10FFFF,less than or equal than U+7F for ASCII OR less than equal than U+FF for Latin1
  SURROGATE,    // The decoded character must be not be in U+D800...DFFF (UTF-8 or UTF-32) OR
                // a high surrogate must be followed by a low surrogate and a low surrogate must be preceded by a high surrogate (UTF-16) OR
                // there must be no surrogate at all (Latin1)
  OTHER         // Not related to validation/transcoding.
};

struct result {
  error_code error;
  size_t count;     // In case of error, indicates the position of the error. In case of success, indicates the number of code units validated/written.

  simdutf_really_inline result();

  simdutf_really_inline result(error_code, size_t);
};

}
#endif