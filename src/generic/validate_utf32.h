namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf32 {

simdutf_really_inline const char32_t *validate_utf32(const char32_t *input,
                                                     size_t size) {
  const char32_t *end = input + size;

  using Vector = simd32<uint32_t>;

  const auto standardmax = Vector::splat(0x10ffff);
  const auto offset = Vector::splat(0xffff2000);
  const auto standardoffsetmax = Vector::splat(0xfffff7ff);
  auto currentmax = Vector();
  auto currentoffsetmax = Vector();

  constexpr size_t N = Vector::ELEMENTS;

  while (input + N < end) {
    auto in = Vector(input);
    if (!match_system(endianness::BIG)) {
      in.swap_bytes();
    }

    currentmax = max_val(currentmax, in);
    currentoffsetmax = max_val(currentoffsetmax, in + offset);
    input += N;
  }

  const auto too_large = currentmax > standardmax;
  if (too_large.any()) {
    return nullptr;
  }

  const auto surrogate = currentoffsetmax > standardoffsetmax;
  if (surrogate.any()) {
    return nullptr;
  }

  return input;
}

simdutf_really_inline result validate_utf32_with_errors(const char32_t *input,
                                                        size_t size) {
  const char32_t *start = input;
  const char32_t *end = input + size;

  using Vector = simd32<uint32_t>;

  const auto standardmax = Vector::splat(0x10ffff);
  const auto offset = Vector::splat(0xffff2000);
  const auto standardoffsetmax = Vector::splat(0xfffff7ff);

  constexpr size_t N = Vector::ELEMENTS;

  while (input + N < end) {
    auto in = Vector(input);
    if (!match_system(endianness::BIG)) {
      in.swap_bytes();
    }

    const auto too_large = in > standardmax;
    const auto surrogate = (in + offset) > standardoffsetmax;

    const auto combined = too_large | surrogate;
    if (simdutf_unlikely(combined.any())) {
      // scalar code will detect exact error and the position
      return result(error_code::OTHER, input - start);
    }

    input += N;
  }

  return result(error_code::SUCCESS, input - start);
}

} // namespace utf32
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
