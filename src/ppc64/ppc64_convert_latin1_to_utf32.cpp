std::pair<const char *, char32_t *>
ppc64_convert_latin1_to_utf32(const char *buf, size_t len,
                            char32_t *utf32_output) {
  const char *end = buf + len;

  using Vector = simd8<uint8_t>;

  constexpr const size_t N = Vector::ELEMENTS;
  constexpr const size_t K = simd32<uint32_t>::ELEMENTS;

  const Vector zero = Vector::zero();
#define mkpat(idx0, idx1, idx2, idx3) Vector(16, 16, 16, idx0, 16, 16, 16, idx1, 16, 16, 16, idx2, 16, 16, 16, idx3)
  const Vector expand0 = mkpat(0, 1, 2, 3);
  const Vector expand1 = mkpat(4, 5, 6, 7);
  const Vector expand2 = mkpat(8, 9, 10, 11);
  const Vector expand3 = mkpat(12, 13, 14, 15);
#undef mkpat

  while (buf + N <= end) {
    const auto in = Vector::load(buf);

    const auto expanded0 = expand0.lookup_32(in, zero);
    const auto expanded1 = expand1.lookup_32(in, zero);
    const auto expanded2 = expand2.lookup_32(in, zero);
    const auto expanded3 = expand3.lookup_32(in, zero);

    expanded0.store(utf32_output + 0*K);
    expanded1.store(utf32_output + 1*K);
    expanded2.store(utf32_output + 2*K);
    expanded3.store(utf32_output + 3*K);

    buf += N;
    utf32_output += N;
  }

  return std::make_pair(buf, utf32_output);
}
