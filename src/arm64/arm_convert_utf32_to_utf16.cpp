std::pair<const char32_t*, char16_t*> arm_convert_utf32_to_utf16(const char32_t* buf, size_t len, char16_t* utf16_out) {
  uint16_t * utf16_output = reinterpret_cast<uint16_t*>(utf16_out);
  const char32_t* end = buf + len;

  while(buf + 4 <= end) {
    uint32x4_t in = vld1q_u32(reinterpret_cast<const uint32_t *>(buf));

    // Check if no bits set above 16th
    if(vmaxvq_u32(in) <= 0xFFFF) {
      uint16x4_t utf16_packed = vmovn_u32(in);
      const uint16x4_t v_d800 = vmov_n_u16((uint16_t)0xd800);
      const uint16x4_t v_dfff = vmov_n_u16((uint16_t)0xdfff);
      if (vmaxv_u16(vand_u16(vcle_u16(utf16_packed, v_dfff), vcge_u16(utf16_packed, v_d800))) != 0) {
        return std::make_pair(nullptr, reinterpret_cast<char16_t*>(utf16_output));
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
          *utf16_output++ = char16_t(word);
        } else {
          // will generate a surrogate pair
          if (word > 0x10FFFF) { return std::make_pair(nullptr, reinterpret_cast<char16_t*>(utf16_output)); }
          word -= 0x10000;
          *utf16_output++ = char16_t(0xD800 + (word >> 10));
          *utf16_output++ = char16_t(0xDC00 + (word & 0x3FF));
        }
      }
      buf += k;
    }
  }

  return std::make_pair(buf, reinterpret_cast<char16_t*>(utf16_output));
}