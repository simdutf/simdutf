namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf16 {

// Note: this is direct translation of westmere implementation.

/*
 * Process one block of 8 characters.  If in_place is false,
 * copy the block from in to out.  If there is a sequencing
 * error in the block, overwrite the illsequenced characters
 * with the replacement character.  This function reads one
 * character before the beginning of the buffer as a lookback.
 * If that character is illsequenced, it too is overwritten.
 */
template <endianness big_endian, bool in_place>
simdutf_really_inline void utf16fix_block(char16_t *out, const char16_t *in) {
  const char16_t replacement = scalar::utf16::replacement<big_endian>();
  auto swap_if_needed = [](uint16_t c) -> uint16_t {
    return !simdutf::match_system(big_endian) ? scalar::u16_swap_bytes(c) : c;
  };

  using vector_u16 = simd16<uint16_t>;

  const auto lookback = vector_u16::load(in - 1);
  const auto block = vector_u16::load(in);

  const auto lb_masked = lookback & swap_if_needed(0xfc00);
  const auto block_masked = block & swap_if_needed(0xfc00);

  const auto lb_is_high = lb_masked == swap_if_needed(0xd800);
  const auto block_is_low = block_masked == swap_if_needed(0xdc00);

  const auto illseq = lb_is_high ^ block_is_low;
  if (!illseq.is_zero()) {
    /* compute the cause of the illegal sequencing */
    const auto lb_illseq = ~block_is_low & lb_is_high;
    const auto block_illseq =
        (~lb_is_high & block_is_low) | lb_illseq.template byte_right_shift<2>();

    /* fix illegal sequencing in the lookback */
    const auto lb = lb_illseq.first();
    out[-1] = char16_t((lb & replacement) | (~lb & out[-1]));
    /* fix illegal sequencing in the main block */
    const auto mask = as_vector_u16(block_illseq);
    const auto fixed = (~mask & block) | (mask & replacement);

    fixed.store(reinterpret_cast<uint16_t *>(out));
  } else if (!in_place) {
    block.store(reinterpret_cast<uint16_t *>(out));
  }
}

template <endianness big_endian>
void to_well_formed(const char16_t *in, size_t n, char16_t *out) {
  using vector_u16 = simd16<uint16_t>;
  constexpr size_t N = vector_u16::ELEMENTS;

  if (n < N + 1) {
    scalar::utf16::to_well_formed_utf16<big_endian>(in, n, out);
    return;
  }

  const char16_t replacement = scalar::utf16::replacement<big_endian>();

  out[0] =
      scalar::utf16::is_low_surrogate<big_endian>(in[0]) ? replacement : in[0];

  /* duplicate code to have the compiler specialise utf16fix_block() */
  if (in == out) {
    constexpr bool inplace = true;
    for (size_t i = 1; i + N < n; i += N) {
      utf16fix_block<big_endian, inplace>(out + i, in + i);
    }

    utf16fix_block<big_endian, inplace>(out + n - N, in + n - N);
  } else {
    constexpr bool copy_data = false;
    for (size_t i = 1; i + N < n; i += N) {
      utf16fix_block<big_endian, copy_data>(out + i, in + i);
    }

    utf16fix_block<big_endian, copy_data>(out + n - N, in + n - N);
  }

  out[n - 1] = scalar::utf16::is_high_surrogate<big_endian>(out[n - 1])
                   ? replacement
                   : out[n - 1];
}

} // namespace utf16
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
