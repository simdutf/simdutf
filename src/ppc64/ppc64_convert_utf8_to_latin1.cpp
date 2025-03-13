// depends on "tables/utf8_to_utf16_tables.h"

// Convert up to 12 bytes from utf8 to latin1 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
size_t convert_masked_utf8_to_latin1(const char *input,
                                     uint64_t utf8_end_of_code_point_mask,
                                     char *&latin1_output) {
  // we use an approach where we try to process up to 12 input bytes.
  // Why 12 input bytes and not 16? Because we are concerned with the size of
  // the lookup tables. Also 12 is nicely divisible by two and three.
  //
  //
  // Optimization note: our main path below is load-latency dependent. Thus it
  // is maybe beneficial to have fast paths that depend on branch prediction but
  // have less latency. This results in more instructions but, potentially, also
  // higher speeds.
  //
  const auto in = vector_u8::load(input);
  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask &
      0xfff; // we are only processing 12 bytes in case it is not all ASCII
  if (utf8_end_of_code_point_mask == 0xfff) {
    // We process the data in chunks of 12 bytes.
    in.store(latin1_output);
    latin1_output += 12; // We wrote 12 characters.
    return 12;           // We consumed 12 bytes.
  }
  /// We do not have a fast path available, so we fallback.
  const uint8_t idx =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][0];
  const uint8_t consumed =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][1];
  // this indicates an invalid input:
  if (idx >= 64) {
    return consumed;
  }
  // Here we should have (idx < 64), if not, there is a bug in the validation or
  // elsewhere. SIX (6) input code-code units this is a relatively easy scenario
  // we process SIX (6) input code-code units. The max length in bytes of six
  // code code units spanning between 1 and 2 bytes each is 12 bytes. On
  // processors where pdep/pext is fast, we might be able to use a small lookup
  // table.

  const auto reshuffle = vector_u8::load(&tables::utf8_to_utf16::shufutf8[idx]);
  const auto perm8 = reshuffle.lookup_32(in, vector_u8::zero());
#if SIMDUTF_IS_BIG_ENDIAN
  const auto perm16 = as_vector_u16(perm8).swap_bytes();
#else
  const auto perm16 = as_vector_u16(perm8);
#endif // SIMDUTF_IS_BIG_ENDIAN
  const auto ascii = perm16 & uint16_t(0x7f);
  const auto highbyte = perm16 & uint16_t(0x1f00);
  const auto composed = ascii | highbyte.shr<2>();

  const auto latin1_packed = vector_u16::pack(composed, composed);
#if defined(__clang__)
  __attribute__((aligned(16))) char buf[16];
  latin1_packed.store(buf);
  memcpy(latin1_output, buf, 6);
#else
  // writing 8 bytes even though we only care about the first 6 bytes.
  const auto tmp = vec_u64_t(latin1_packed.value);
  memcpy(latin1_output, &tmp[0], 8);
#endif
  latin1_output += 6; // We wrote 6 bytes.
  return consumed;
}
