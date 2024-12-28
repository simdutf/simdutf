#ifndef SIMDUTF_H
#define SIMDUTF_H
#include <cstring>
#include <vector>

// #include "simdutf/compiler_check.h"
// #include "simdutf/common_defs.h"
// #include "simdutf/encoding_types.h"
// #include "simdutf/error.h"


namespace temporary {

enum endianness { LITTLE = 0, BIG = 1 };

enum error_code {
  SUCCESS = 0,
  HEADER_BITS, // Any byte must have fewer than 5 header bits.
  TOO_SHORT,   // The leading byte must be followed by N-1 continuation bytes,
  // where N is the UTF-8 character length This is also the error
  // when the input is truncated.
  TOO_LONG,    // We either have too many consecutive continuation bytes or the
  // string starts with a continuation byte.
  OVERLONG, // The decoded character must be above U+7F for two-byte characters,
  // U+7FF for three-byte characters, and U+FFFF for four-byte
  // characters.
  TOO_LARGE, // The decoded character must be less than or equal to
  // U+10FFFF,less than or equal than U+7F for ASCII OR less than
  // equal than U+FF for Latin1
  SURROGATE, // The decoded character must be not be in U+D800...DFFF (UTF-8 or
  // UTF-32) OR a high surrogate must be followed by a low surrogate
  // and a low surrogate must be preceded by a high surrogate
  // (UTF-16) OR there must be no surrogate at all (Latin1)
  INVALID_BASE64_CHARACTER, // Found a character that cannot be part of a valid
                            // base64 string. This may include a misplaced
                            // padding character ('=').
  BASE64_INPUT_REMAINDER,   // The base64 input terminates with a single
                          // character, excluding padding (=).
  BASE64_EXTRA_BITS,        // The base64 input terminates with non-zero
                     // padding bits.
  OUTPUT_BUFFER_TOO_SMALL,  // The provided buffer is too small.
  OTHER                     // Not related to validation/transcoding.
};

struct result {
  error_code error;
  size_t count; // In case of error, indicates the position of the error. In
                // case of success, indicates the number of code units
                // validated/written.

   result() : error{error_code::SUCCESS}, count{0} {}

   result(error_code err, size_t pos)
      : error{err}, count{pos} {}
};

temporary::result validate_utf16le_with_errors(
    const char16_t * buf, std::size_t len) noexcept;

std::vector<char16_t> get_test_data();
}


#endif // SIMDUTF_H
