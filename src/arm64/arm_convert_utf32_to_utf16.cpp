struct expansion_result_t {
  size_t u16count;
  uint8x16_t compressed_v;
};

// This function is used to check for invalid UTF-32 characters
// and surrogate pairs in the input
simdutf_really_inline uint64_t invalid_utf32(const uint32x4x2_t in) {
  const auto standardmax = vdupq_n_u32(0x10ffff);
  const auto v_d800 = vdupq_n_u32(0xd800);
  const auto v_fffff800 = vdupq_n_u32(0xfffff800);
  const auto too_large1 = vcgtq_u32(in.val[0], standardmax);
  const auto too_large2 = vcgtq_u32(in.val[1], standardmax);
  const auto surrogate1 = vceqq_u32(vandq_u32(in.val[0], v_fffff800), v_d800);
  const auto surrogate2 = vceqq_u32(vandq_u32(in.val[1], v_fffff800), v_d800);
  const auto err1 = vorrq_u32(too_large1, surrogate1);
  const auto err2 = vorrq_u32(too_large2, surrogate2);
  const auto err =
      vuzp2q_u16(vreinterpretq_u16_u32(err1), vreinterpretq_u16_u32(err2));

  return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(err, 8)), 0);
}

// This function is used to check for surrogate pairs in the input
simdutf_really_inline uint64_t fast_invalid_utf32(const uint32x4x2_t in) {
  const auto v_d800 = vdupq_n_u32(0xd800);
  const auto v_fffff800 = vdupq_n_u32(0xfffff800);
  const auto surrogate1 = vceqq_u32(vandq_u32(in.val[0], v_fffff800), v_d800);
  const auto surrogate2 = vceqq_u32(vandq_u32(in.val[1], v_fffff800), v_d800);
  const auto err = vuzp2q_u16(vreinterpretq_u16_u32(surrogate1),
                              vreinterpretq_u16_u32(surrogate2));
  return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(err, 8)), 0);
}

template <endianness byte_order>
simdutf_really_inline expansion_result_t
neon_expand_surrogate(const uint32x4_t in) {
  const uint32x4_t v_ffff0000 = vdupq_n_u32(0xffff0000);
  const uint32x4_t non_surrogate_mask = vceqzq_u32(vandq_u32(in, v_ffff0000));
  const uint64_t cmp_bits =
      vget_lane_u64(vreinterpret_u64_u32(vshrn_n_u64(
                        vreinterpretq_u64_u32(non_surrogate_mask), 31)),
                    0);
  const uint8_t mask =
      uint8_t(~((cmp_bits & 0x3) | ((cmp_bits >> 30) & 0xc)) & 0xf);
  const uint32x4_t v_10000 = vdupq_n_u32(0x00010000);
  const uint32x4_t t0 = vsubq_u32(in, v_10000);
  const uint32x4_t t1 = vandq_u32(t0, vdupq_n_u32(0xfffff));
  const uint32x4_t t2 = vshrq_n_u32(t1, 10);
  const uint32x4_t t3 = vsliq_n_u32(t2, t1, 16);
  const uint32x4_t surrogates = vorrq_u32(
      vandq_u32(t3, vdupq_n_u32(0x03ff03ff)), vdupq_n_u32(0xdc00d800));
  const uint8x16_t merged =
      vreinterpretq_u8_u32(vbslq_u32(non_surrogate_mask, in, surrogates));

  const uint8x16_t shuffle_v = vld1q_u8(reinterpret_cast<const uint8_t *>(
      (byte_order == endianness::LITTLE)
          ? tables::utf32_to_utf16::pack_utf32_to_utf16le[mask]
          : tables::utf32_to_utf16::pack_utf32_to_utf16be[mask]));

  const size_t u16count = 4 + vget_lane_u8(vcnt_u8(vcreate_u8(mask)), 0);
  const uint8x16_t compressed_v = vqtbl1q_u8(merged, shuffle_v);

  return {u16count, compressed_v};
}

template <endianness big_endian>
std::pair<const char32_t *, char16_t *>
arm_convert_utf32_to_utf16(const char32_t *buf, size_t len,
                           char16_t *utf16_out) {
  uint16_t *utf16_output = reinterpret_cast<uint16_t *>(utf16_out);
  const char32_t *end = buf + len;

  uint16x8_t forbidden_bytemask = vmovq_n_u16(0x0);
  // To avoid buffer overflow while writing compressed_v
  const size_t safety_margin = 4;
  while (end - buf >= std::ptrdiff_t(8 + safety_margin)) {
    uint32x4x2_t in = vld1q_u32_x2(reinterpret_cast<const uint32_t *>(buf));

    // Check if no bits set above 16th
    uint32_t max_val = vmaxvq_u32(vmaxq_u32(in.val[0], in.val[1]));
    if (simdutf_likely(max_val <= 0xFFFF)) {
      uint16x8_t utf16_packed = vuzp1q_u16(vreinterpretq_u16_u32(in.val[0]),
                                           vreinterpretq_u16_u32(in.val[1]));

      const uint16x8_t v_d800 = vmovq_n_u16((uint16_t)0xd800);
      const uint16x8_t v_f800 = vmovq_n_u16((uint16_t)0xf800);
      forbidden_bytemask =
          vorrq_u16(vceqq_u16(vandq_u16(utf16_packed, v_f800), v_d800),
                    forbidden_bytemask);

      if (!match_system(big_endian)) {
        utf16_packed = vreinterpretq_u16_u8(
            vrev16q_u8(vreinterpretq_u8_u16(utf16_packed)));
      }
      vst1q_u16(utf16_output, utf16_packed);
      utf16_output += 8;
      buf += 8;
    } else {
      if (simdutf_unlikely(fast_invalid_utf32(in) || max_val > 0x10ffff)) {
        return std::make_pair(nullptr,
                              reinterpret_cast<char16_t *>(utf16_output));
      }
      expansion_result_t res = neon_expand_surrogate<big_endian>(in.val[0]);
      vst1q_u8(reinterpret_cast<uint8_t *>(utf16_output), res.compressed_v);
      utf16_output += res.u16count;
      res = neon_expand_surrogate<big_endian>(in.val[1]);
      vst1q_u8(reinterpret_cast<uint8_t *>(utf16_output), res.compressed_v);
      utf16_output += res.u16count;
      buf += 8;
    }
  }

  // check for invalid input
  if (vmaxvq_u32(vreinterpretq_u32_u16(forbidden_bytemask)) != 0) {
    return std::make_pair(nullptr, reinterpret_cast<char16_t *>(utf16_output));
  }

  return std::make_pair(buf, reinterpret_cast<char16_t *>(utf16_output));
}

template <endianness big_endian>
std::pair<result, char16_t *>
arm_convert_utf32_to_utf16_with_errors(const char32_t *buf, size_t len,
                                       char16_t *utf16_out) {
  uint16_t *utf16_output = reinterpret_cast<uint16_t *>(utf16_out);
  const char32_t *start = buf;
  const char32_t *end = buf + len;

  // To avoid buffer overflow while writing compressed_v
  const size_t safety_margin = 4;
  while (end - buf >= std::ptrdiff_t(8 + safety_margin)) {
    uint32x4x2_t in = vld1q_u32_x2(reinterpret_cast<const uint32_t *>(buf));

    // Check if no bits set above 16th
    uint32_t max_val = vmaxvq_u32(vmaxq_u32(in.val[0], in.val[1]));
    if (simdutf_likely(max_val <= 0xFFFF)) {
      uint16x8_t utf16_packed = vuzp1q_u16(vreinterpretq_u16_u32(in.val[0]),
                                           vreinterpretq_u16_u32(in.val[1]));

      const uint16x8_t v_d800 = vmovq_n_u16((uint16_t)0xd800);
      const uint16x8_t v_f800 = vmovq_n_u16((uint16_t)0xf800);
      const uint16x8_t forbidden_bytemask =
          vceqq_u16(vandq_u16(utf16_packed, v_f800), v_d800);
      if (vmaxvq_u16(forbidden_bytemask) != 0) {
        return std::make_pair(result(error_code::SURROGATE, buf - start),
                              reinterpret_cast<char16_t *>(utf16_output));
      }

      if (!match_system(big_endian)) {
        utf16_packed = vreinterpretq_u16_u8(
            vrev16q_u8(vreinterpretq_u8_u16(utf16_packed)));
      }
      vst1q_u16(utf16_output, utf16_packed);
      utf16_output += 8;
      buf += 8;
    } else {
      const uint64_t err =
          max_val <= 0x10ffff ? fast_invalid_utf32(in) : invalid_utf32(in);
      if (simdutf_unlikely(err)) {
        const size_t pos = trailing_zeroes(err) / 8;
        for (size_t k = 0; k < pos; k++) {
          uint32_t word = buf[k];
          if ((word & 0xFFFF0000) == 0) {
            // will not generate a surrogate pair
            *utf16_output++ = !match_system(big_endian)
                                  ? char16_t(word >> 8 | word << 8)
                                  : char16_t(word);
          } else {
            // will generate a surrogate pair
            word -= 0x10000;
            uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
            uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
            if (!match_system(big_endian)) {
              high_surrogate =
                  uint16_t(high_surrogate >> 8 | high_surrogate << 8);
              low_surrogate = uint16_t(low_surrogate << 8 | low_surrogate >> 8);
            }
            *utf16_output++ = char16_t(high_surrogate);
            *utf16_output++ = char16_t(low_surrogate);
          }
        }
        const uint32_t word = buf[pos];
        const size_t error_pos = buf - start + pos;
        if (word > 0x10FFFF) {
          return {result(error_code::TOO_LARGE, error_pos),
                  reinterpret_cast<char16_t *>(utf16_output)};
        }
        if (word >= 0xD800 && word <= 0xDFFF) {
          return {result(error_code::SURROGATE, error_pos),
                  reinterpret_cast<char16_t *>(utf16_output)};
        }
        return {result(error_code::OTHER, error_pos),
                reinterpret_cast<char16_t *>(utf16_output)};
      }
      expansion_result_t res = neon_expand_surrogate<big_endian>(in.val[0]);
      vst1q_u8(reinterpret_cast<uint8_t *>(utf16_output), res.compressed_v);
      utf16_output += res.u16count;
      res = neon_expand_surrogate<big_endian>(in.val[1]);
      vst1q_u8(reinterpret_cast<uint8_t *>(utf16_output), res.compressed_v);
      utf16_output += res.u16count;
      buf += 8;
    }
  }

  return std::make_pair(result(error_code::SUCCESS, buf - start),
                        reinterpret_cast<char16_t *>(utf16_output));
}
