// Compact four UTF-8 code units, each stored in a 32-bit lane.
#ifndef SIMDUTF_UTF32_TO_UTF8_TABLES_H
#define SIMDUTF_UTF32_TO_UTF8_TABLES_H

namespace simdutf {
namespace {
namespace tables {
namespace utf32_to_utf8 {

// Each row starts with the packed byte count, followed by a PSHUFB control
// vector. The row index contains four two-bit (width - 1) fields, one per
// input lane from least to most significant.
struct pack_1_2_3_4_utf8_bytes_table {
  uint8_t rows[256][17] = {};

  constexpr pack_1_2_3_4_utf8_bytes_table() {
    for (uint16_t key = 0; key < 256; key++) {
      uint8_t output_index = 0;
      for (uint8_t lane = 0; lane < 4; lane++) {
        const uint8_t width =
            static_cast<uint8_t>(((key >> (lane * 2)) & 0x3) + 1);
        for (uint8_t byte = 0; byte < width; byte++) {
          rows[key][output_index + 1] = static_cast<uint8_t>(lane * 4 + byte);
          output_index++;
        }
      }
      rows[key][0] = output_index;
      for (; output_index < 16; output_index++) {
        rows[key][output_index + 1] = static_cast<uint8_t>(0x80);
      }
    }
  }
};

constexpr pack_1_2_3_4_utf8_bytes_table pack_1_2_3_4_utf8_bytes{};

} // namespace utf32_to_utf8
} // namespace tables
} // unnamed namespace
} // namespace simdutf

#endif // SIMDUTF_UTF32_TO_UTF8_TABLES_H
