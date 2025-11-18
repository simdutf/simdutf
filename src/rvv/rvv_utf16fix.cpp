template <endianness big_endian, bool in_place, bool vlmax>
simdutf_really_inline void utf16fix_block_rvv(char16_t *out, const char16_t *in,
                                              size_t vl) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  simdutf_constexpr auto swap_if_needed = [](uint16_t c) -> uint16_t {
    return !simdutf::match_system(big_endian) ? scalar::u16_swap_bytes(c) : c;
  };

  vuint16m8_t block = __riscv_vle16_v_u16m8((const uint16_t *)in, vl);
  vuint16m8_t lookback = __riscv_vslide1up_vx_u16m8(block, in[-1], vl);
  vuint16m8_t lb_masked =
      __riscv_vand_vx_u16m8(lookback, swap_if_needed(0xfc00U), vl);
  vuint16m8_t block_masked =
      __riscv_vand_vx_u16m8(block, swap_if_needed(0xfc00U), vl);
  vbool2_t lb_is_high =
      __riscv_vmseq_vx_u16m8_b2(lb_masked, swap_if_needed(0xd800U), vl);
  vbool2_t block_is_low =
      __riscv_vmseq_vx_u16m8_b2(block_masked, swap_if_needed(0xdc00U), vl);

  vbool2_t illseq = __riscv_vmxor_mm_b2(lb_is_high, block_is_low, vl);
  if (__riscv_vfirst_m_b2(illseq, vl) >= 0) {
    vbool2_t lb_illseq = __riscv_vmandn_mm_b2(lb_is_high, block_is_low, vl);

    vbool2_t lb_illseq_right_shifted;
    if (vlmax) {
      /* right shift mask register directly via reinterpret at vlmax */
      size_t vlm = __riscv_vsetvlmax_e8mf2();
      vuint8mf2_t vlb_illseq =
          __riscv_vlmul_trunc_u8mf2(__riscv_vreinterpret_u8m1(lb_illseq));
      lb_illseq_right_shifted =
          __riscv_vreinterpret_b2(__riscv_vlmul_ext_u8m1(__riscv_vmacc_vx_u8mf2(
              __riscv_vsrl_vx_u8mf2(vlb_illseq, 1, vlm), 1 << 7,
              __riscv_vslide1down_vx_u8mf2(vlb_illseq, 0, vlm), vlm)));
    } else {
      lb_illseq_right_shifted = __riscv_vmandn_mm_b2(
          __riscv_vmseq_vx_u16m8_b2(
              __riscv_vslide1down_vx_u16m8(lb_masked, 0, vl),
              swap_if_needed(0xd800U), vl),
          __riscv_vmseq_vx_u16m8_b2(
              __riscv_vslide1down_vx_u16m8(block_masked, 0, vl),
              swap_if_needed(0xdc00U), vl),
          vl);
    }

    char16_t last = out[-1]; /* allow compiler to generate branchless code */
    out[-1] = __riscv_vfirst_m_b2(lb_illseq, vl) == 0 ? replacement : last;
    vbool2_t block_illseq =
        __riscv_vmor_mm_b2(__riscv_vmandn_mm_b2(block_is_low, lb_is_high, vl),
                           lb_illseq_right_shifted, vl);
    block = __riscv_vmerge_vxm_u16m8(block, replacement, block_illseq, vl);
    __riscv_vse16_v_u16m8((uint16_t *)out, block, vl);
  } else if (!in_place) {
    __riscv_vse16_v_u16m8((uint16_t *)out, block, vl);
  }
}

template <endianness big_endian>
void rvv_to_well_formed_utf16(const char16_t *in, size_t n, char16_t *out) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  const size_t VL = __riscv_vsetvlmax_e16m8();
  if (n == 0)
    return;

  out[0] =
      scalar::utf16::is_low_surrogate<big_endian>(in[0]) ? replacement : in[0];
  n -= 1;
  in += 1;
  out += 1;

  /* duplicate code to have the compiler specialise utf16fix_block() */
  if (in == out) {
    for (; n > VL; n -= VL, in += VL, out += VL) {
      utf16fix_block_rvv<big_endian, true, true>(out, in, VL);
    }
    utf16fix_block_rvv<big_endian, true, false>(out, in, n);
  } else {
    for (; n > VL; n -= VL, in += VL, out += VL) {
      utf16fix_block_rvv<big_endian, false, true>(out, in, VL);
    }
    utf16fix_block_rvv<big_endian, false, false>(out, in, n);
  }

  out[n - 1] = scalar::utf16::is_high_surrogate<big_endian>(out[n - 1])
                   ? replacement
                   : out[n - 1];
}

void implementation::to_well_formed_utf16le(const char16_t *input, size_t len,
                                            char16_t *output) const noexcept {
  return rvv_to_well_formed_utf16<endianness::LITTLE>(input, len, output);
}

void implementation::to_well_formed_utf16be(const char16_t *input, size_t len,
                                            char16_t *output) const noexcept {
  return rvv_to_well_formed_utf16<endianness::BIG>(input, len, output);
}

template <simdutf_ByteFlip bflip>
simdutf_really_inline static void
rvv_change_endianness_utf16(const char16_t *src, size_t len, char16_t *dst) {
  for (size_t vl; len > 0; len -= vl, src += vl, dst += vl) {
    vl = __riscv_vsetvl_e16m8(len);
    vuint16m8_t v = __riscv_vle16_v_u16m8((uint16_t *)src, vl);
    __riscv_vse16_v_u16m8((uint16_t *)dst, simdutf_byteflip<bflip>(v, vl), vl);
  }
}

void implementation::change_endianness_utf16(const char16_t *src, size_t len,
                                             char16_t *dst) const noexcept {
  if (supports_zvbb())
    return rvv_change_endianness_utf16<simdutf_ByteFlip::ZVBB>(src, len, dst);
  else
    return rvv_change_endianness_utf16<simdutf_ByteFlip::V>(src, len, dst);
}
