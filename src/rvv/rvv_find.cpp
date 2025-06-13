simdutf_really_inline const char *util_find(const char *start, const char *end,
                                            char character) noexcept {
  const char *src = start;
  for (size_t len = end - start, vl; len > 0; len -= vl, src += vl) {
    vl = __riscv_vsetvl_e8m8(len);
    vuint8m8_t v = __riscv_vle8_v_u8m8((uint8_t *)src, vl);
    long idx =
        __riscv_vfirst_m_b1(__riscv_vmseq_vx_u8m8_b1(v, character, vl), vl);
    if (idx >= 0)
      return src + idx;
  }
  return end;
}

simdutf_really_inline const char16_t *util_find(const char16_t *start,
                                                const char16_t *end,
                                                char16_t character) noexcept {
  const char16_t *src = start;
  for (size_t len = end - start, vl; len > 0; len -= vl, src += vl) {
    vl = __riscv_vsetvl_e16m8(len);
    vuint16m8_t v = __riscv_vle16_v_u16m8((uint16_t *)src, vl);
    long idx =
        __riscv_vfirst_m_b2(__riscv_vmseq_vx_u16m8_b2(v, character, vl), vl);
    if (idx >= 0)
      return src + idx;
  }
  return end;
}
