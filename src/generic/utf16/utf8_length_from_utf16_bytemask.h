namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16 {

using namespace simd;

template <endianness big_endian>
simdutf_really_inline size_t utf8_length_from_utf16_bytemask(const char16_t *in,
                                                             size_t size) {
  size_t pos = 0;

  using vector_u16 = simd16<uint16_t>;
  constexpr size_t N = vector_u16::ELEMENTS;

  const auto one = vector_u16::splat(1);

  auto v_count = vector_u16::zero();

  // each char16 yields at least one byte
  size_t count = size / N * N;

  // in a single iteration the increment is 0, 1 or 2, despite we have
  // three additions
  constexpr size_t max_iterations = 65535 / 2;
  size_t iteration = max_iterations;

  for (; pos < size / N * N; pos += N) {
    auto input = vector_u16::load(reinterpret_cast<const uint16_t *>(in + pos));
    if (!match_system(big_endian)) {
      input = input.swap_bytes();
    }
    // 0xd800 .. 0xdbff - low surrogate
    // 0xdc00 .. 0xdfff - high surrogate
    const auto is_surrogate = ((input & uint16_t(0xf800)) == uint16_t(0xd800));

    // c0 - chars that yield 2- or 3-byte UTF-8 codes
    const auto c0 = min(input & uint16_t(0xff80), one);

    // c1 - chars that yield 3-byte UTF-8 codes (including surrogates)
    const auto c1 = min(input & uint16_t(0xf800), one);

    /*
        Explanation how the counting works.

        In the case of a non-surrogate character we count:
        * always 1 -- see how `count` is initialized above;
        * c0 = 1 if the current char yields 2 or 3 bytes;
        * c1 = 1 if the current char yields 3 bytes.

        Thus, we always have correct count for the current char:
        from 1, 2 or 3 bytes.

        A trickier part is how we count surrogate pairs. Whether
        we encounter a surrogate (low or high), we count it as
        3 chars and then minus 1 (`is_surrogate` is -1 or 0).
        Each surrogate char yields 2. A surrogate pair, that
        is a low surrogate followed by a high one, yields
        the expected 4 bytes.

        It also correctly handles cases when low surrogate is
        processed by the this loop, but high surrogate is counted
        by the scalar procedure. The scalar procedure uses exactly
        the described approach, thanks to that for valid UTF-16
        strings it always count correctly.
    */
    v_count += c0;
    v_count += c1;
    v_count += vector_u16(is_surrogate);

    iteration -= 1;
    if (iteration == 0) {
      count += v_count.sum();
      v_count = vector_u16::zero();
      iteration = max_iterations;
    }
  }

  if (iteration > 0) {
    count += v_count.sum();
  }

  return count + scalar::utf16::utf8_length_from_utf16<big_endian>(in + pos,
                                                                   size - pos);
}

template <endianness big_endian>
simdutf_really_inline size_t
utf8_length_from_utf16_with_replacement(const char16_t *in, size_t size) {
  using vector_u16 = simd16<uint16_t>;
  constexpr size_t N = vector_u16::ELEMENTS;
  if (N + 1 > size) {
    return scalar::utf16::utf8_length_from_utf16_with_replacement<big_endian>(
        in, size);
  } // special case for short inputs
  size_t pos = 0;

  const auto one = vector_u16::splat(1);

  auto v_count = vector_u16::zero();
  auto v_mismatched_count = vector_u16::zero();

  size_t count = 0;
  size_t mismatched_count = 0;

  // in a single iteration the increment is 0, 1 or 2, despite we have
  // three additions
  constexpr size_t max_iterations = 65535 / 2;
  size_t iteration = max_iterations;

  if (scalar::utf16::is_low_surrogate<big_endian>(in[0])) {
    mismatched_count += 1;
  }

  for (; pos < (size - 1) / N * N; pos += N) {
    auto input = vector_u16::load(reinterpret_cast<const uint16_t *>(in + pos));
    if (!match_system(big_endian)) {
      input = input.swap_bytes();
    }
    auto input_next =
        vector_u16::load(reinterpret_cast<const uint16_t *>(in + pos + 1));
    if (!match_system(big_endian)) {
      input_next = input_next.swap_bytes();
    }

    const auto lb_masked = input & (0xfc00);
    const auto block_masked = input_next & (0xfc00);

    const auto lb_is_high = lb_masked == (0xd800);
    const auto block_is_low = block_masked == (0xdc00);

    const auto illseq = min(vector_u16(lb_is_high ^ block_is_low), one);
    // 0xd800 .. 0xdbff - low surrogate
    // 0xdc00 .. 0xdfff - high surrogate
    const auto is_surrogate = ((input & uint16_t(0xf800)) == uint16_t(0xd800));

    // c0 - chars that yield 2- or 3-byte UTF-8 codes
    const auto c0 = min(input & uint16_t(0xff80), one);

    // c1 - chars that yield 3-byte UTF-8 codes (including surrogates)
    const auto c1 = min(input & uint16_t(0xf800), one);

    v_count += c0;
    v_count += c1;
    v_count += vector_u16(is_surrogate);
    v_mismatched_count += illseq;

    iteration -= 1;
    if (iteration == 0) {
      count += v_count.sum();
      v_count = vector_u16::zero();
      mismatched_count += v_mismatched_count.sum();
      v_mismatched_count = vector_u16::zero();
      iteration = max_iterations;
    }
  }

  if (iteration > 0) {
    count += v_count.sum();
    mismatched_count += v_mismatched_count.sum();
  }

  if (scalar::utf16::is_low_surrogate<big_endian>(in[pos])) {
    if (!scalar::utf16::is_high_surrogate<big_endian>(in[pos - 1])) {
      mismatched_count -= 1;
      count += 2;
      pos += 1;
    }
  }
  count += pos;
  count += mismatched_count;
  if (scalar::utf16::is_high_surrogate<big_endian>(in[pos - 1])) {
    if (scalar::utf16::is_low_surrogate<big_endian>(in[pos])) {
      pos += 1;
      count += 2;
    }
  }
  return count +
         scalar::utf16::utf8_length_from_utf16_with_replacement<big_endian>(
             in + pos, size - pos);
}

} // namespace utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
