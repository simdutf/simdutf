template <endianness big_endian, bool in_place>
simdutf_really_inline void utf16fix_block_rvv(char16_t *out, const char16_t *in,
                                              size_t vl) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  auto swap_if_needed = [](uint16_t c) -> uint16_t {
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
    vbool2_t lb_illseq_right_shifted = __riscv_vmandn_mm_b2(
        __riscv_vmseq_vx_u16m8_b2(
            __riscv_vslide1down_vx_u16m8(lb_masked, 0, vl),
            swap_if_needed(0xd800U), vl),
        __riscv_vmseq_vx_u16m8_b2(
            __riscv_vslide1down_vx_u16m8(block_masked, 0, vl),
            swap_if_needed(0xdc00U), vl),
        vl);
    if (__riscv_vfirst_m_b2(lb_illseq, vl) == 0) {
      out[-1] = replacement;
    }
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
  if (n == 0)
    return;

  out[0] =
      scalar::utf16::is_low_surrogate<big_endian>(in[0]) ? replacement : in[0];
  n -= 1;
  in += 1;
  out += 1;

  /* duplicate code to have the compiler specialise utf16fix_block() */
  if (in == out) {
    for (size_t vl; n > 0; n -= vl, in += vl, out += vl) {
      vl = __riscv_vsetvl_e16m8(n);
      utf16fix_block_rvv<big_endian, true>(out, in, vl);
    }
  } else {
    for (size_t vl; n > 0; n -= vl, in += vl, out += vl) {
      vl = __riscv_vsetvl_e16m8(n);
      utf16fix_block_rvv<big_endian, false>(out, in, vl);
    }
  }

  out[n - 1] = scalar::utf16::is_high_surrogate<big_endian>(out[n - 1])
                   ? replacement
                   : out[n - 1];
}
