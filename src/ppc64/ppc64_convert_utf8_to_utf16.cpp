// depends on "tables/utf8_to_utf16_tables.h"

// Convert up to 12 bytes from utf8 to utf16 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
template <endianness big_endian>
size_t convert_masked_utf8_to_utf16(const char * /*input*/,
                                    uint64_t /*utf8_end_of_code_point_mask*/,
                                    char16_t *& /*utf16_output*/) {
  return 0; // XXX
}
