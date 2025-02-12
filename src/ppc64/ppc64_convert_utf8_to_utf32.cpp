// depends on "tables/utf8_to_utf16_tables.h"

// Convert up to 12 bytes from utf8 to utf32 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
size_t convert_masked_utf8_to_utf32(const char * /*input*/,
                                    uint64_t /*utf8_end_of_code_point_mask*/,
                                    char32_t *& /*utf32_output*/) {
  return 0; // XXX
}
