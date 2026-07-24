// Fast way to create a bitmask/movemask on arm64. THe returned integer has 4
// bits corresponding to each element of the inputted vector.
simdutf_really_inline uint64_t arm_bitmask4(uint8x16_t v) {
  uint16x8_t v16 = vreinterpretq_u16_u8(v);
  uint8x8_t res = vshrn_n_u16(v16, 4);
  return vget_lane_u64(vreinterpret_u64_u8(res), 0);
}

simdutf_really_inline uint16x4_t arm_hangul_mask(uint16x4_t input) {
  uint16x4_t ge = vcge_u16(input, vdup_n_u16(scalar::normalization::s_base));
  uint16x4_t lt = vclt_u16(input, vdup_n_u16(scalar::normalization::s_base +
                                             scalar::normalization::s_count));
  uint16x4_t cmp = vand_u16(lt, ge);
  return cmp;
}

uint16x4x3_t arm_compute_hangul_jamo(uint16x4_t chars) {
  // Compute the S index
  uint16x4_t s = vsub_u16(chars, vdup_n_u16(scalar::normalization::s_base));

  uint32x4_t l_fixed = vmull_n_u16(s, 28533);
  // Shift the fixed point number
  uint32x4_t l_wide = vshrq_n_u32(l_fixed, 24);
  // L index: s / N_COUNT
  uint16x4_t l = vmovn_u32(l_wide);

  // Multiply and subtract to get the remainder
  uint16x4_t v_modulo = vmls_n_u16(s, l, scalar::normalization::n_count);
  uint32x4_t v_fixed = vmull_n_u16(v_modulo, 2341);
  uint32x4_t v_wide = vshrq_n_u32(v_fixed, 16);
  // V index: (s % N_COUNT) / T_COUNT
  uint16x4_t v = vmovn_u32(v_wide);

  uint16x4_t t_shifted = vshr_n_u16(s, 2);
  uint32x4_t t_fixed = vmull_n_u16(t_shifted, 18725);
  // s / T_COUNT
  uint32x4_t t_div_wide = vshrq_n_u32(t_fixed, 17);
  uint16x4_t t_div = vmovn_u32(t_div_wide);
  // T index: s % T_COUNT
  uint16x4_t t = vmls_n_u16(s, t_div, scalar::normalization::t_count);

  uint16x4x3_t vals;
  vals.val[0] = vadd_u16(l, vdup_n_u16(scalar::normalization::l_base));
  vals.val[1] = vadd_u16(v, vdup_n_u16(scalar::normalization::v_base));
  vals.val[2] = vadd_u16(t, vdup_n_u16(scalar::normalization::t_base));

  return vals;
}
