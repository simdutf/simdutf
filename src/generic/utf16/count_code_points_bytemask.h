namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16 {

using namespace simd;

template <endianness big_endian>
simdutf_really_inline size_t count_code_points(const char16_t *in,
                                               size_t size) {
  using vector_u16 = simd16<uint16_t>;
  constexpr size_t N = vector_u16::ELEMENTS;

  size_t pos = 0;
  size_t count = 0;

  constexpr size_t max_itertions = 65535;
  const auto one = vector_u16::splat(1);
  const auto zero = vector_u16::zero();

  size_t itertion = 0;

  auto counters = zero;
  for (; pos < size / N * N; pos += N) {
    auto input = vector_u16::load(in + pos);
    if (!match_system(big_endian)) {
      input = input.swap_bytes();
    }

    const auto t0 = input & uint16_t(0xfc00);
    const auto t1 = t0 ^ uint16_t(0xdc00);

    // t2[0] == 1 iff input[0] outside range 0xdc00..dfff (the word is not a
    // high surrogate)
    const auto t2 = min(t1, one);

    counters += t2;

    itertion += 1;
    if (itertion == max_itertions) {
      count += counters.sum();
      counters = zero;
      itertion = 0;
    }
  }

  if (itertion > 0) {
    count += counters.sum();
  }

  return count +
         scalar::utf16::count_code_points<big_endian>(in + pos, size - pos);
}

} // namespace utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
