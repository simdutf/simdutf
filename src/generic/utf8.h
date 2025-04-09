namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8 {

using namespace simd;

simdutf_really_inline size_t count_code_points(const char *in, size_t size) {
  size_t pos = 0;
  size_t count = 0;
  for (; pos + 64 <= size; pos += 64) {
    simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + pos));
    uint64_t utf8_continuation_mask = input.gt(-65);
    count += count_ones(utf8_continuation_mask);
  }
  return count + scalar::utf8::count_code_points(in + pos, size - pos);
}

#ifdef SIMDUTF_SIMD_HAS_BYTEMASK
simdutf_really_inline size_t count_code_points_bytemask(const char *in,
                                                        size_t size) {
  using vector_i8 = simd8<int8_t>;
  using vector_u8 = simd8<uint8_t>;
  using vector_u64 = simd64<uint64_t>;

  constexpr size_t N = vector_i8::SIZE;
  constexpr size_t max_iterations = 255 / 4;

  size_t pos = 0;
  size_t count = 0;

  auto counters = vector_u64::zero();
  auto local = vector_u8::zero();
  size_t iterations = 0;
  for (; pos + 4 * N <= size; pos += 4 * N) {
    const auto input0 =
        simd8<int8_t>::load(reinterpret_cast<const int8_t *>(in + pos + 0 * N));
    const auto input1 =
        simd8<int8_t>::load(reinterpret_cast<const int8_t *>(in + pos + 1 * N));
    const auto input2 =
        simd8<int8_t>::load(reinterpret_cast<const int8_t *>(in + pos + 2 * N));
    const auto input3 =
        simd8<int8_t>::load(reinterpret_cast<const int8_t *>(in + pos + 3 * N));
    const auto mask0 = input0 > int8_t(-65);
    const auto mask1 = input1 > int8_t(-65);
    const auto mask2 = input2 > int8_t(-65);
    const auto mask3 = input3 > int8_t(-65);

    local -= vector_u8(mask0);
    local -= vector_u8(mask1);
    local -= vector_u8(mask2);
    local -= vector_u8(mask3);

    iterations += 1;
    if (iterations == max_iterations) {
      counters += sum_8bytes(local);
      local = vector_u8::zero();
      iterations = 0;
    }
  }

  if (iterations > 0) {
    count += local.sum_bytes();
  }

  count += counters.sum();

  return count + scalar::utf8::count_code_points(in + pos, size - pos);
}
#endif // SIMDUTF_SIMD_HAS_BYTEMASK

simdutf_really_inline size_t utf16_length_from_utf8(const char *in,
                                                    size_t size) {
  size_t pos = 0;
  size_t count = 0;
  // This algorithm could no doubt be improved!
  for (; pos + 64 <= size; pos += 64) {
    simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + pos));
    uint64_t utf8_continuation_mask = input.lt(-65 + 1);
    // We count one word for anything that is not a continuation (so
    // leading bytes).
    count += 64 - count_ones(utf8_continuation_mask);
    int64_t utf8_4byte = input.gteq_unsigned(240);
    count += count_ones(utf8_4byte);
  }
  return count + scalar::utf8::utf16_length_from_utf8(in + pos, size - pos);
}

} // namespace utf8
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
