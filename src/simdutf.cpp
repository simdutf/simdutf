#include "simdutf.h"

#include <cstdint>

namespace temporary {

bool match_system2(temporary::endianness e) {
#define SIMDUTF_IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#if SIMDUTF_IS_BIG_ENDIAN
  return e == endianness::BIG;
#else
  return e == temporary::endianness::LITTLE;
#endif
}
inline uint16_t swap_bytes(const uint16_t word) {
  return uint16_t((word >> 8) | (word << 8));
}
template <temporary::endianness big_endian>
inline temporary::result validate_with_errors(const char16_t *data,
                                              size_t len) noexcept {
  size_t pos = 0;
  while (pos < len) {
    char16_t word =
        !match_system2(big_endian) ? swap_bytes(data[pos]) : data[pos];
    if ((word & 0xF800) == 0xD800) {
      if (pos + 1 >= len) {
        return temporary::result(temporary::error_code::SURROGATE, pos);
      }
      char16_t diff = char16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return temporary::result(temporary::error_code::SURROGATE, pos);
      }
      char16_t next_word = !match_system2(big_endian)
                               ? swap_bytes(data[pos + 1])
                               : data[pos + 1];
      char16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return temporary::result(temporary::error_code::SURROGATE, pos);
      }
      pos += 2;
    } else {
      pos++;
    }
  }
  return temporary::result(temporary::error_code::SUCCESS, pos);
}
temporary::result validate_utf16le_with_errors(const char16_t *buf,
                                               size_t len) noexcept {
  return validate_with_errors<temporary::endianness::LITTLE>(buf, len);
}

std::vector<char16_t> get_test_data() {
  return {34086, 23924, 65167, 9449,  32427, 65404, 14192, 60752, 57590, 63026,
          21580, 61350, 10983, 62185, 57347, 60300, 4467,  62929, 8682,  23452,
          36980, 28869, 3039,  58105, 58452, 61481, 47340, 62550, 60371, 5138,
          18914, 64294, 59075, 18991, 59766, 51746, 62468, 10585, 47438, 23289,
          26964, 60025, 45402, 46457, 14455, 43444, 4468,  61865, 62211, 20469,
          64275, 64580, 59200, 65492, 58097, 62185, 62338, 21634, 46275, 63289,
          65112, 60074, 8557,  64274, 60533, 65032, 4111,  36288, 26464, 40249,
          61369, 45399, 32932, 64028, 64190, 23838, 1945,  58776, 18272, 64343,
          63253, 18963, 62372, 32657, 61650, 58466, 8742,  21137, 37523, 54819,
          43802, 61335, 58945, 25550, 19772, 36049, 59839, 63373, 32361, 40867,
          31315, 62197, 32946, 59498, 60676, 59519, 28254, 62015, 58067, 62280,
          52750, 21820, 63529, 38616, 13940, 60929, 59059, 60084, 31374, 64431,
          59037, 27232, 64273, 11103, 65006, 46645, 65432, 5559};
}

} // namespace temporary
