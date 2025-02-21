#include <limits>

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf32 {

template <typename T> T min(T a, T b) { return a <= b ? a : b; }

size_t utf8_length_from_utf32(const char32_t *input, size_t length) {
  using vector_u32 = simd32<uint32_t>;

  const char32_t *start = input;

  // we add up to three ones in a single iteration (see the vectorized loop in
  // section #2 below)
  const size_t max_increment = 3;

  const size_t N = vector_u32::ELEMENTS;

  const auto one = vector_u32::splat(1);
  const auto v_ffffff80 = vector_u32::splat(0xffffff80);
  const auto v_fffff800 = vector_u32::splat(0xfffff800);
  const auto v_ffff0000 = vector_u32::splat(0xffff0000);

  size_t counter = 0;

  // 1. vectorized loop unrolled 4 times
  {
    // we use uint32 counters, this is
    const size_t max_iterations =
        std::numeric_limits<uint32_t>::max() / (max_increment * 4);
    size_t blocks = length / (N * 4);
    length -= blocks * (N * 4);
    while (blocks != 0) {
      const size_t iterations = min(blocks, max_iterations);
      blocks -= iterations;

      simd32<uint32_t> acc = vector_u32::zero();
      for (size_t i = 0; i < iterations; i++) {
        const auto in0 = vector_u32(input + 0 * N);
        const auto in1 = vector_u32(input + 1 * N);
        const auto in2 = vector_u32(input + 2 * N);
        const auto in3 = vector_u32(input + 3 * N);

        acc += min(one, in0 & v_ffffff80);
        acc += min(one, in1 & v_ffffff80);
        acc += min(one, in2 & v_ffffff80);
        acc += min(one, in3 & v_ffffff80);

        acc += min(one, in0 & v_fffff800);
        acc += min(one, in1 & v_fffff800);
        acc += min(one, in2 & v_fffff800);
        acc += min(one, in3 & v_fffff800);

        acc += min(one, in0 & v_ffff0000);
        acc += min(one, in1 & v_ffff0000);
        acc += min(one, in2 & v_ffff0000);
        acc += min(one, in3 & v_ffff0000);

        input += 4 * N;
      }

      counter += acc.sum();
    }
  }

  // 2. vectorized loop for tail
  {
    const size_t max_iterations =
        std::numeric_limits<uint32_t>::max() / max_increment;
    size_t blocks = length / N;
    length -= blocks * N;
    while (blocks != 0) {
      const size_t iterations = min(blocks, max_iterations);
      blocks -= iterations;

      auto acc = vector_u32::zero();
      for (size_t i = 0; i < iterations; i++) {
        const auto in = vector_u32(input);

        acc += min(one, in & v_ffffff80);
        acc += min(one, in & v_fffff800);
        acc += min(one, in & v_ffff0000);

        input += N;
      }

      counter += acc.sum();
    }
  }

  const size_t consumed = input - start;
  if (consumed != 0) {
    // We don't count 0th bytes in the vectorized loops above, this
    // is why we need to count them in the end.
    counter += consumed;
  }

  return counter + scalar::utf32::utf8_length_from_utf32(input, length);
}

} // namespace utf32
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
