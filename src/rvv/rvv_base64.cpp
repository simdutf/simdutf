template <bool insert_line_feeds>
size_t encode_base64_rvv(char *dst, const char *src, size_t srclen,
                         base64_options options,
                         size_t line_length = simdutf::default_line_length) {
  size_t offset = 0;
  if constexpr (insert_line_feeds) {
    if (line_length < 4) {
      line_length = 4;
    }
  }

  static constexpr uint8_t table_standard[64] = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
  };
  static constexpr uint8_t table_url[64] = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_',
  };

  const uint8_t *table = (options & base64_url) ? table_url : table_standard;
  uint8_t *out = (uint8_t *)dst;

  size_t triplets = srclen / 3;
  size_t i = 0;

  while (triplets > 0) {
    size_t max_vl = triplets;
    if constexpr (insert_line_feeds) {
      size_t max_per_line = line_length / 4;
      if (max_per_line < max_vl)
        max_vl = max_per_line;
    }
    size_t vl = __riscv_vsetvl_e8m2(max_vl);
    if (vl == 0)
      break;

    // 3-way deinterleaved load: a[k]=src[3k], b[k]=src[3k+1], c[k]=src[3k+2]
    vuint8m2_t a = __riscv_vlse8_v_u8m2((const uint8_t *)src + i, 3, vl);
    vuint8m2_t b = __riscv_vlse8_v_u8m2((const uint8_t *)src + i + 1, 3, vl);
    vuint8m2_t c = __riscv_vlse8_v_u8m2((const uint8_t *)src + i + 2, 3, vl);

    // Extract 6-bit indices from each triplet
    vuint8m2_t idx0 = __riscv_vsrl_vx_u8m2(a, 2, vl);
    vuint8m2_t idx1 = __riscv_vand_vx_u8m2(
        __riscv_vor_vv_u8m2(__riscv_vsll_vx_u8m2(a, 4, vl),
                            __riscv_vsrl_vx_u8m2(b, 4, vl), vl),
        0x3F, vl);
    vuint8m2_t idx2 = __riscv_vand_vx_u8m2(
        __riscv_vor_vv_u8m2(__riscv_vsll_vx_u8m2(b, 2, vl),
                            __riscv_vsrl_vx_u8m2(c, 6, vl), vl),
        0x3F, vl);
    vuint8m2_t idx3 = __riscv_vand_vx_u8m2(c, 0x3F, vl);

    // Table lookup: map 6-bit indices to base64 characters
    vuint8m2_t out0 = __riscv_vluxei8_v_u8m2(table, idx0, vl);
    vuint8m2_t out1 = __riscv_vluxei8_v_u8m2(table, idx1, vl);
    vuint8m2_t out2 = __riscv_vluxei8_v_u8m2(table, idx2, vl);
    vuint8m2_t out3 = __riscv_vluxei8_v_u8m2(table, idx3, vl);

    size_t chunk = vl * 4;

    if constexpr (insert_line_feeds) {
      if (offset >= line_length) {
        *out++ = '\n';
        offset = 0;
      }

      // 4-way interleaved store
      __riscv_vsse8_v_u8m2(out, 4, out0, vl);
      __riscv_vsse8_v_u8m2(out + 1, 4, out1, vl);
      __riscv_vsse8_v_u8m2(out + 2, 4, out2, vl);
      __riscv_vsse8_v_u8m2(out + 3, 4, out3, vl);

      if (offset + chunk <= line_length) {
        out += chunk;
        offset += chunk;
      } else {
        size_t before = line_length - offset;
        size_t after = chunk - before;
        std::memmove(out + before + 1, out + before, after);
        out[before] = '\n';
        offset = after;
        out += chunk + 1;
      }
    } else {
      // 4-way interleaved store
      __riscv_vsse8_v_u8m2(out, 4, out0, vl);
      __riscv_vsse8_v_u8m2(out + 1, 4, out1, vl);
      __riscv_vsse8_v_u8m2(out + 2, 4, out2, vl);
      __riscv_vsse8_v_u8m2(out + 3, 4, out3, vl);
      out += chunk;
    }

    triplets -= vl;
    i += vl * 3;
  }

  out += scalar::base64::tail_encode_base64_impl<insert_line_feeds>(
      (char *)out, src + i, srclen - i, options, line_length, offset);

  return (size_t)((char *)out - dst);
}

size_t encode_base64(char *dst, const char *src, size_t srclen,
                     base64_options options) {
  return encode_base64_rvv<false>(dst, src, srclen, options);
}
