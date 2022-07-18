template <endianness big_endian>
std::pair<const char32_t*, char16_t*> arm_convert_utf32_to_utf16(const char32_t* buf, size_t len, char16_t* utf16_out) {
  uint16_t * utf16_output = reinterpret_cast<uint16_t*>(utf16_out);
  const char32_t* end = buf + len;

  uint16x4_t forbidden_bytemask = vmov_n_u16(0x0);

  while(buf + 4 <= end) {
    uint32x4_t in = vld1q_u32(reinterpret_cast<const uint32_t *>(buf));

    // Check if no bits set above 16th
    if(vmaxvq_u32(in) <= 0xFFFF) {
      uint16x4_t utf16_packed = vmovn_u32(in);

      const uint16x4_t v_d800 = vmov_n_u16((uint16_t)0xd800);
      const uint16x4_t v_dfff = vmov_n_u16((uint16_t)0xdfff);
      forbidden_bytemask = vorr_u16(vand_u16(vcle_u16(utf16_packed, v_dfff), vcge_u16(utf16_packed, v_d800)), forbidden_bytemask);

      if (big_endian) {
        #ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
        const uint8x8_t swap = make_uint8x16_t(1, 0, 3, 2, 5, 4, 7, 6);
        #else
        const uint8x8_t swap = {1, 0, 3, 2, 5, 4, 7, 6};
        #endif
        utf16_packed = vreinterpret_u16_u8(vtbl1_u8(vreinterpret_u8_u16(utf16_packed), swap));
      }
      vst1_u16(utf16_output, utf16_packed);
      utf16_output += 4;
      buf += 4;
    } else {
      size_t forward = 3;
      size_t k = 0;
      if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
      for(; k < forward; k++) {
        uint32_t word = buf[k];
        if((word & 0xFFFF0000)==0) {
          // will not generate a surrogate pair
          if (word >= 0xD800 && word <= 0xDFFF) { return std::make_pair(nullptr, reinterpret_cast<char16_t*>(utf16_output)); }
          *utf16_output++ = big_endian ? char16_t(word >> 8 | word << 8) : char16_t(word);
        } else {
          // will generate a surrogate pair
          if (word > 0x10FFFF) { return std::make_pair(nullptr, reinterpret_cast<char16_t*>(utf16_output)); }
          word -= 0x10000;
          uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
          uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
          if (big_endian) {
            high_surrogate = uint16_t(high_surrogate >> 8 | high_surrogate << 8);
            low_surrogate = uint16_t(low_surrogate << 8 | low_surrogate >> 8);
          }
          *utf16_output++ = char16_t(high_surrogate);
          *utf16_output++ = char16_t(low_surrogate);
        }
      }
      buf += k;
    }
  }

  // check for invalid input
  if (vmaxv_u16(forbidden_bytemask) != 0) {
    return std::make_pair(nullptr, reinterpret_cast<char16_t*>(utf16_output));
  }

  return std::make_pair(buf, reinterpret_cast<char16_t*>(utf16_output));
}