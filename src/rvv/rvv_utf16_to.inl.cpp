template<simdutf_ByteFlip bflip>
simdutf_really_inline static result rvv_utf16_to_latin1_with_errors(const char16_t *src, size_t len, char *dst) {
  const char16_t *const beg = src;
  for (size_t vl; len > 0; len -= vl, src += vl, dst += vl) {
    vl = __riscv_vsetvl_e16m8(len);
    vuint16m8_t v = __riscv_vle16_v_u16m8((uint16_t*)src, vl);
    v = simdutf_byteflip<bflip>(v, vl);
    long idx = __riscv_vfirst_m_b2(__riscv_vmsgtu_vx_u16m8_b2(v, 255, vl), vl);
    if (idx >= 0)
      return result(error_code::TOO_LARGE, beg - src + idx);
    __riscv_vse8_v_u8m4((uint8_t*)dst, __riscv_vncvt_x_x_w_u8m4(v, vl), vl);
  }
  return result(error_code::SUCCESS, src - beg);
}

simdutf_warn_unused size_t implementation::convert_utf16le_to_latin1(const char16_t *src, size_t len, char *dst) const noexcept {
  result res = convert_utf16le_to_latin1_with_errors(src, len, dst);
  return res.error == error_code::SUCCESS ? res.count : 0;
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_latin1(const char16_t *src, size_t len, char *dst) const noexcept {
  result res = convert_utf16be_to_latin1_with_errors(src, len, dst);
  return res.error == error_code::SUCCESS ? res.count : 0;
}

simdutf_warn_unused result implementation::convert_utf16le_to_latin1_with_errors(const char16_t *src, size_t len, char *dst) const noexcept {
  return rvv_utf16_to_latin1_with_errors<simdutf_ByteFlip::NONE>(src, len, dst);
}

simdutf_warn_unused result implementation::convert_utf16be_to_latin1_with_errors(const char16_t *src, size_t len, char *dst) const noexcept {
  if (supports_zvbb())
    return rvv_utf16_to_latin1_with_errors<simdutf_ByteFlip::ZVBB>(src, len, dst);
  else
    return rvv_utf16_to_latin1_with_errors<simdutf_ByteFlip::V>(src, len, dst);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_latin1(const char16_t *src, size_t len, char *dst) const noexcept {
  const char16_t *const beg = src;
  for (size_t vl; len > 0; len -= vl, src += vl, dst += vl) {
    vl = __riscv_vsetvl_e16m8(len);
    vuint16m8_t v = __riscv_vle16_v_u16m8((uint16_t*)src, vl);
    __riscv_vse8_v_u8m4((uint8_t*)dst, __riscv_vncvt_x_x_w_u8m4(v, vl), vl);
  }
  return src - beg;
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_latin1(const char16_t *src, size_t len, char *dst) const noexcept {
  const char16_t *const beg = src;
  for (size_t vl; len > 0; len -= vl, src += vl, dst += vl) {
    vl = __riscv_vsetvl_e16m8(len);
    vuint16m8_t v = __riscv_vle16_v_u16m8((uint16_t*)src, vl);
    __riscv_vse8_v_u8m4((uint8_t*)dst, __riscv_vnsrl_wx_u8m4(v, 8, vl), vl);
  }
  return src - beg;
}

template<simdutf_ByteFlip bflip>
simdutf_really_inline static result rvv_utf16_to_utf8_with_errors(const char16_t *src, size_t len, char *dst) {
  size_t n = len;
  const char16_t *srcBeg = src;
  const char *dstBeg = dst;
  size_t vl8m4 = __riscv_vsetvlmax_e8m4();
  vbool2_t m4mulp2 = __riscv_vmseq_vx_u8m4_b2(__riscv_vand_vx_u8m4(__riscv_vid_v_u8m4(vl8m4), 3, vl8m4), 2, vl8m4);

  for (size_t vl, vlOut; n > 0; ) {
    vl = __riscv_vsetvl_e16m2(n);

    vuint16m2_t v = __riscv_vle16_v_u16m2((uint16_t const*)src, vl);
    v = simdutf_byteflip<bflip>(v, vl);
    vbool8_t m234 = __riscv_vmsgtu_vx_u16m2_b8(v, 0x80-1, vl);

    if (__riscv_vfirst_m_b8(m234,vl) < 0) { /* 1 byte utf8 */
      vlOut = vl;
      __riscv_vse8_v_u8m1((uint8_t*)dst, __riscv_vncvt_x_x_w_u8m1(v, vlOut), vlOut);
      n -= vl, src += vl, dst += vlOut;
      continue;
    }

    vbool8_t m34  = __riscv_vmsgtu_vx_u16m2_b8(v, 0x800-1, vl);

    if (__riscv_vfirst_m_b8(m34,vl) < 0) { /* 1/2 byte utf8 */
      /* 0: [     aaa|aabbbbbb]
       * 1: [aabbbbbb|        ] vsll 8
       * 2: [        |   aaaaa] vsrl 6
       * 3: [00111111|00011111]
       * 4: [  bbbbbb|000aaaaa] (1|2)&3
       * 5: [11000000|11000000]
       * 6: [10bbbbbb|110aaaaa] 4|5 */
      vuint16m2_t twoByte  =
        __riscv_vand_vx_u16m2(__riscv_vor_vv_u16m2(
          __riscv_vsll_vx_u16m2(v, 8, vl),
          __riscv_vsrl_vx_u16m2(v, 6, vl),
        vl), 0b0011111100011111, vl);
      vuint16m2_t vout16 = __riscv_vor_vx_u16m2_mu(m234, v, twoByte, 0b1000000011000000, vl);
      vuint8m2_t vout = __riscv_vreinterpret_v_u16m2_u8m2(vout16);

      /* Every high byte that is zero should be compressed
       * low bytes should never be compressed, so we set them
       * to all ones, and then create a non-zero bytes mask */
      vbool4_t mcomp = __riscv_vmsne_vx_u8m2_b4(__riscv_vreinterpret_v_u16m2_u8m2(__riscv_vor_vx_u16m2(vout16, 0xFF, vl)), 0, vl*2);
      vlOut = __riscv_vcpop_m_b4(mcomp, vl*2);

      vout = __riscv_vcompress_vm_u8m2(vout, mcomp, vl*2);
      __riscv_vse8_v_u8m2((uint8_t*)dst, vout, vlOut);

      n -= vl, src += vl, dst += vlOut;
      continue;
    }

    vbool8_t sur = __riscv_vmseq_vx_u16m2_b8(__riscv_vand_vx_u16m2(v, 0xF800, vl), 0xD800, vl);
    long first = __riscv_vfirst_m_b8(sur, vl);
    size_t tail = vl - first;
    vl = first < 0 ? vl : first;

    if (vl > 0) { /* 1/2/3 byte utf8 */
      /* in: [aaaabbbb|bbcccccc]
       * v1: [0bcccccc|        ] vsll  8
       * v1: [10cccccc|        ] vsll  8 & 0b00111111 | 0b10000000
       * v2: [        |110bbbbb] vsrl  6 & 0b00111111 | 0b11000000
       * v2: [        |10bbbbbb] vsrl  6 & 0b00111111 | 0b10000000
       * v3: [        |1110aaaa] vsrl 12 | 0b11100000
       *  1: [00000000|0bcccccc|00000000|00000000] => [0bcccccc]
       *  2: [00000000|10cccccc|110bbbbb|00000000] => [110bbbbb] [10cccccc]
       *  3: [00000000|10cccccc|10bbbbbb|1110aaaa] => [1110aaaa] [10bbbbbb] [10cccccc]
       */
      vuint16m2_t v1, v2, v3, v12;
      v1 = __riscv_vor_vx_u16m2_mu(m234, v, __riscv_vand_vx_u16m2(v, 0b00111111, vl), 0b10000000, vl);
      v1 = __riscv_vsll_vx_u16m2(v1, 8, vl);

      v2 = __riscv_vor_vx_u16m2(__riscv_vand_vx_u16m2(__riscv_vsrl_vx_u16m2(v, 6, vl), 0b00111111, vl), 0b10000000, vl);
      v2 = __riscv_vor_vx_u16m2_mu(__riscv_vmnot_m_b8(m34,vl), v2, v2, 0b01000000, vl);
      v3 = __riscv_vor_vx_u16m2(__riscv_vsrl_vx_u16m2(v, 12, vl), 0b11100000, vl);
      v12 = __riscv_vor_vv_u16m2_mu(m234, v1, v1, v2, vl);

      vuint32m4_t w12 = __riscv_vwmulu_vx_u32m4(v12, 1<<8, vl);
      vuint32m4_t w123 = __riscv_vwaddu_wv_u32m4_mu(m34, w12, w12, v3, vl);
      vuint8m4_t vout = __riscv_vreinterpret_v_u32m4_u8m4(w123);

      vbool2_t mcomp = __riscv_vmor_mm_b2(m4mulp2, __riscv_vmsne_vx_u8m4_b2(vout, 0, vl*4), vl*4);
      vlOut = __riscv_vcpop_m_b2(mcomp, vl*4);

      vout = __riscv_vcompress_vm_u8m4(vout, mcomp, vl*4);
      __riscv_vse8_v_u8m4((uint8_t*)dst, vout, vlOut);

      n -= vl, src += vl, dst += vlOut;
    }

    if (tail) while (n) {
      uint16_t word = simdutf_byteflip<bflip>(src[0]);
      if((word & 0xFF80)==0) {
        break;
      } else if((word & 0xF800)==0) {
        break;
      } else if ((word & 0xF800) != 0xD800) {
        break;
      } else {
        // must be a surrogate pair
        if (n <= 1) return result(error_code::SURROGATE, src - srcBeg);
        uint16_t diff = word - 0xD800;
        if (diff > 0x3FF) return result(error_code::SURROGATE, src - srcBeg);
        uint16_t diff2 = simdutf_byteflip<bflip>(src[1]) - 0xDC00;
        if (diff2 > 0x3FF) return result(error_code::SURROGATE, src - srcBeg);

        uint32_t value = ((diff + 0x40) << 10) + diff2 ;

        // will generate four UTF-8 bytes
        // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
        *dst++ = (char)( (value>>18)             | 0b11110000);
        *dst++ = (char)(((value>>12) & 0b111111) | 0b10000000);
        *dst++ = (char)(((value>> 6) & 0b111111) | 0b10000000);
        *dst++ = (char)(( value      & 0b111111) | 0b10000000);
        src += 2;
        n -= 2;
      }
    }
  }

  return result(error_code::SUCCESS, dst - dstBeg);
}

simdutf_warn_unused size_t implementation::convert_utf16le_to_utf8(const char16_t *src, size_t len, char *dst) const noexcept {
  result res = convert_utf16le_to_utf8_with_errors(src, len, dst);
  return res.error == error_code::SUCCESS ? res.count : 0;
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_utf8(const char16_t *src, size_t len, char *dst) const noexcept {
  result res = convert_utf16be_to_utf8_with_errors(src, len, dst);
  return res.error == error_code::SUCCESS ? res.count : 0;
}

simdutf_warn_unused result implementation::convert_utf16le_to_utf8_with_errors(const char16_t *src, size_t len, char *dst) const noexcept {
  return rvv_utf16_to_utf8_with_errors<simdutf_ByteFlip::NONE>(src, len, dst);
}

simdutf_warn_unused result implementation::convert_utf16be_to_utf8_with_errors(const char16_t *src, size_t len, char *dst) const noexcept {
  if (supports_zvbb())
    return rvv_utf16_to_utf8_with_errors<simdutf_ByteFlip::ZVBB>(src, len, dst);
  else
    return rvv_utf16_to_utf8_with_errors<simdutf_ByteFlip::V>(src, len, dst);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_utf8(const char16_t *src, size_t len, char *dst) const noexcept {
  return convert_utf16le_to_utf8(src, len, dst);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_utf8(const char16_t *src, size_t len, char *dst) const noexcept {
  return convert_utf16be_to_utf8(src, len, dst);
}

template<simdutf_ByteFlip bflip>
simdutf_really_inline static result rvv_utf16_to_utf32_with_errors(const char16_t *src, size_t len, char32_t *dst) {
  const char16_t *const srcBeg = src;
  char32_t *const dstBeg = dst;

  uint16_t last = 0;
  for (size_t vl, vlOut; len > 0; len -= vl, src += vl, dst += vlOut, last = simdutf_byteflip<bflip>(src[-1])) {
    vl = __riscv_vsetvl_e16m2(len);
    vuint16m2_t v1 = __riscv_vle16_v_u16m2((uint16_t const*)src, vl);
    v1 = simdutf_byteflip<bflip>(v1, vl);
    vuint16m2_t v0 = __riscv_vslide1up_vx_u16m2(v1, last, vl);

    vbool8_t surhi0 = __riscv_vmseq_vx_u16m2_b8(__riscv_vand_vx_u16m2(v0, 0xFC00, vl), 0xD800, vl);
    vbool8_t surlo1 = __riscv_vmseq_vx_u16m2_b8(__riscv_vand_vx_u16m2(v1, 0xFC00, vl), 0xDC00, vl);

    /* no surrogates */
    if (__riscv_vfirst_m_b8(__riscv_vmor_mm_b8(surhi0, surlo1, vl), vl) < 0) {
      vlOut = vl;
      __riscv_vse32_v_u32m4((uint32_t*)dst, __riscv_vzext_vf2_u32m4(v1, vl), vl);
      continue;
    }

    long idx = __riscv_vfirst_m_b8(__riscv_vmxor_mm_b8(surhi0, surlo1, vl), vl);
    if (idx >= 0) {
      last = idx > 0 ? simdutf_byteflip<bflip>(src[idx-1]) : last;
      return result(error_code::SURROGATE, src - srcBeg + idx - (last - 0xD800u < 0x400u));
    }

    vbool8_t surhi1 = __riscv_vmseq_vx_u16m2_b8(__riscv_vand_vx_u16m2(v1, 0xFC00, vl), 0xD800, vl);
    uint16_t next = vl < len ? simdutf_byteflip<bflip>(src[vl]) : 0;

    vuint32m4_t wide    = __riscv_vzext_vf2_u32m4(v1, vl);
    vuint32m4_t slided  = __riscv_vslide1down_vx_u32m4(wide, next, vl);
    vuint32m4_t aligned = __riscv_vsll_vx_u32m4_mu(surhi1, wide, wide, 10, vl);
    vuint32m4_t added   = __riscv_vadd_vv_u32m4_mu(surhi1, aligned, aligned, slided, vl);
    vuint32m4_t utf32   = __riscv_vadd_vx_u32m4_mu(surhi1, added, added, 0xFCA02400, vl);
    vbool8_t m = __riscv_vmnot_m_b8(surlo1, vl);
    vlOut = __riscv_vcpop_m_b8(m, vl);
    vuint32m4_t comp = __riscv_vcompress_vm_u32m4(utf32, m, vl);
    __riscv_vse32_v_u32m4((uint32_t*)dst, comp, vlOut);
  }

  if (last - 0xD800u < 0x400u)
    return result(error_code::SURROGATE, src - srcBeg - 1); /* end on high surrogate */
  else
    return result(error_code::SUCCESS, dst - dstBeg);
}

simdutf_warn_unused size_t implementation::convert_utf16le_to_utf32(const char16_t *src, size_t len, char32_t *dst) const noexcept {
  result res = convert_utf16le_to_utf32_with_errors(src, len, dst);
  return res.error == error_code::SUCCESS ? res.count : 0;
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_utf32(const char16_t *src, size_t len, char32_t *dst) const noexcept {
  result res = convert_utf16be_to_utf32_with_errors(src, len, dst);
  return res.error == error_code::SUCCESS ? res.count : 0;
}

simdutf_warn_unused result implementation::convert_utf16le_to_utf32_with_errors(const char16_t *src, size_t len, char32_t *dst) const noexcept {
  return rvv_utf16_to_utf32_with_errors<simdutf_ByteFlip::NONE>(src, len, dst);
}

simdutf_warn_unused result implementation::convert_utf16be_to_utf32_with_errors(const char16_t *src, size_t len, char32_t *dst) const noexcept {
  if (supports_zvbb())
    return rvv_utf16_to_utf32_with_errors<simdutf_ByteFlip::ZVBB>(src, len, dst);
  else
    return rvv_utf16_to_utf32_with_errors<simdutf_ByteFlip::V>(src, len, dst);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_utf32(const char16_t *src, size_t len, char32_t *dst) const noexcept {
  return convert_utf16le_to_utf32(src, len, dst);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_utf32(const char16_t *src, size_t len, char32_t *dst) const noexcept {
  return convert_utf16be_to_utf32(src, len, dst);
}
