namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf32 {

simdutf_really_inline bool validate(const char32_t *input, size_t size) {
  if (simdutf_unlikely(size == 0)) {
    // empty input is valid UTF-32. protect the implementation from
    // handling nullptr
    return true;
  }

  const char32_t *end = input + size;

  using vector_u32 = simd32<uint32_t>;

  const auto standardmax = vector_u32::splat(0x10ffff);
  const auto offset = vector_u32::splat(0xffff2000);
  const auto standardoffsetmax = vector_u32::splat(0xfffff7ff);
  auto currentmax = vector_u32::zero();
  auto currentoffsetmax = vector_u32::zero();

  constexpr size_t N = vector_u32::ELEMENTS;

  while (input + N < end) {
    auto in = vector_u32(input);
    if (!match_system(endianness::BIG)) {
      in.swap_bytes();
    }

    currentmax = max(currentmax, in);
    currentoffsetmax = max(currentoffsetmax, in + offset);
    input += N;
  }

  const auto too_large = currentmax > standardmax;
  if (too_large.any()) {
    return false;
  }

  const auto surrogate = currentoffsetmax > standardoffsetmax;
  if (surrogate.any()) {
    return false;
  }

  return scalar::utf32::validate(input, end - input);
}

simdutf_really_inline result validate_with_errors(const char32_t *input,
                                                  size_t size) {
  if (simdutf_unlikely(size == 0)) {
    // empty input is valid UTF-32. protect the implementation from
    // handling nullptr
    return result(error_code::SUCCESS, 0);
  }

  const char32_t *start = input;
  const char32_t *end = input + size;

  using vector_u32 = simd32<uint32_t>;

  const auto standardmax = vector_u32::splat(0x10ffff);
  const auto offset = vector_u32::splat(0xffff2000);
  const auto standardoffsetmax = vector_u32::splat(0xfffff7ff);

  constexpr size_t N = vector_u32::ELEMENTS;

  while (input + N < end) {
    auto in = vector_u32(input);
    if (!match_system(endianness::BIG)) {
      in.swap_bytes();
    }

    const auto too_large = in > standardmax;
    const auto surrogate = (in + offset) > standardoffsetmax;

    const auto combined = too_large | surrogate;
    if (simdutf_unlikely(combined.any())) {
      const size_t consumed = input - start;
      auto sr = scalar::utf32::validate_with_errors(input, end - input);
      sr.count += consumed;

      return sr;
    }

    input += N;
  }

  const size_t consumed = input - start;
  auto sr = scalar::utf32::validate_with_errors(input, end - input);
  sr.count += consumed;

  return sr;
}

} // namespace utf32
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
