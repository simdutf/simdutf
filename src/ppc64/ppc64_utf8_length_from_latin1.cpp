template <typename T> T min(T a, T b) { return a <= b ? a : b; }

std::pair<const char *, size_t> ppc64_utf8_length_from_latin1(const char *input,
                                                              size_t length) {
  constexpr size_t N = vector_u8::ELEMENTS;
  length = (length / N);

  size_t count = length * N;
  while (length != 0) {
    vector_u32 partial = vector_u32::zero();

    // partial accumulator has 32 bits => this yields (2^31 / 16)
    size_t chunk = min(length, size_t(0xffffffff / N));
    length -= chunk;
    while (chunk != 0) {
      auto local = vector_u8::zero();
      // local accumulator has 8 bits => this yields 255 max (we increment by 1
      // in each iteration)
      const size_t n = min(chunk, size_t(255));
      chunk -= n;
      for (size_t i = 0; i < n; i++) {
        const auto in = vector_i8::load(input);
        input += N;

        local -= as_vector_u8(in < vector_i8::splat(0));
      }

      partial = sum4bytes(local, partial);
    }

    for (int i = 0; i < vector_u32::ELEMENTS; i++) {
      count += size_t(partial.value[i]);
    }
  }

  return std::make_pair(input, count);
}
