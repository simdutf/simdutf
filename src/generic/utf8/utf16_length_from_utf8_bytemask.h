namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8 {

using namespace simd;

simdutf_really_inline size_t utf16_length_from_utf8_bytemask(const char *in,
                                                             size_t size) {
  using vector_i8 = simd8<int8_t>;
  using vector_u8 = simd8<uint8_t>;
  using vector_u64 = simd64<uint64_t>;

  constexpr size_t N = vector_i8::SIZE;
  constexpr size_t max_iterations = 255 / 2;

  auto counters = vector_u64::zero();
  auto local = vector_u8::zero();

  size_t iterations = 0;
  size_t pos = 0;
  size_t count = 0;
  for (; pos + N <= size; pos += N) {
    const auto input =
        vector_i8::load(reinterpret_cast<const int8_t *>(in + pos));

    const auto continuation = input > int8_t(-65);
    const auto utf_4bytes = vector_u8(input.value) >= uint8_t(240);

    local -= vector_u8(continuation);
    local -= vector_u8(utf_4bytes);

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

  return count + scalar::utf8::utf16_length_from_utf8(in + pos, size - pos);
}

} // namespace utf8
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
