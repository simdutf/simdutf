simdutf_really_inline size_t arm64_utf16_length_from_utf8(const char *in,
                                                          size_t size) {
  const int8x16_t continuation_limit = vdupq_n_s8(-64);
  const uint8x16_t four_byte_limit = vdupq_n_u8(0xf0);
  size_t pos = 0;
  int64_t count = 0;
  while (pos + 64 <= size) {
    int16x8_t acc = vdupq_n_s16(0);
    size_t blocks = (size - pos) / 64;
    if (blocks > 1024) {
      blocks = 1024;
    }
    const size_t end = pos + blocks * 64;
    for (; pos < end; pos += 64) {
      const int8x16_t in0 =
          vld1q_s8(reinterpret_cast<const int8_t *>(in + pos));
      const int8x16_t in1 =
          vld1q_s8(reinterpret_cast<const int8_t *>(in + pos + 16));
      const int8x16_t in2 =
          vld1q_s8(reinterpret_cast<const int8_t *>(in + pos + 32));
      const int8x16_t in3 =
          vld1q_s8(reinterpret_cast<const int8_t *>(in + pos + 48));
      int8x16_t net = vreinterpretq_s8_u8(vcltq_s8(in0, continuation_limit));
      net =
          vaddq_s8(net, vreinterpretq_s8_u8(vcltq_s8(in1, continuation_limit)));
      net =
          vaddq_s8(net, vreinterpretq_s8_u8(vcltq_s8(in2, continuation_limit)));
      net =
          vaddq_s8(net, vreinterpretq_s8_u8(vcltq_s8(in3, continuation_limit)));
      net = vsubq_s8(net, vreinterpretq_s8_u8(vcgeq_u8(vreinterpretq_u8_s8(in0),
                                                       four_byte_limit)));
      net = vsubq_s8(net, vreinterpretq_s8_u8(vcgeq_u8(vreinterpretq_u8_s8(in1),
                                                       four_byte_limit)));
      net = vsubq_s8(net, vreinterpretq_s8_u8(vcgeq_u8(vreinterpretq_u8_s8(in2),
                                                       four_byte_limit)));
      net = vsubq_s8(net, vreinterpretq_s8_u8(vcgeq_u8(vreinterpretq_u8_s8(in3),
                                                       four_byte_limit)));
      acc = vpadalq_s8(acc, net);
    }
    count += vaddlvq_s16(acc);
  }
  return size_t(int64_t(pos) + count) +
         scalar::utf8::utf16_length_from_utf8(in + pos, size - pos);
}
