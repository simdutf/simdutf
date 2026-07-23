namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_validation {

/**
 * Validates that the string is actual UTF-8.
 */
template <class checker>
bool generic_validate_utf8(const uint8_t *input, size_t length) {
  checker c{};
  buf_block_reader<64> reader(input, length);
  while (reader.has_full_block()) {
    simd::simd8x64<uint8_t> in(reader.full_block());
    c.check_next_input(in);
    reader.advance();
  }
  uint8_t block[64]{};
  reader.get_remainder(block);
  simd::simd8x64<uint8_t> in(block);
  c.check_next_input(in);
  reader.advance();
  c.check_eof();
  return !c.errors();
}

bool generic_validate_utf8(const char *input, size_t length) {
  return generic_validate_utf8<utf8_checker>(
      reinterpret_cast<const uint8_t *>(input), length);
}

/**
 * Validates that the string is actual UTF-8 and stops on errors.
 */
template <class checker>
result generic_validate_utf8_with_errors(const uint8_t *input, size_t length) {
  checker c{};
  buf_block_reader<64> reader(input, length);
  size_t count{0};
  while (reader.has_full_block()) {
    simd::simd8x64<uint8_t> in(reader.full_block());
    c.check_next_input(in);
    if (c.errors()) {
      if (count != 0) {
        count--;
      } // Sometimes the error is only detected in the next chunk
      result res = scalar::utf8::rewind_and_validate_with_errors(
          reinterpret_cast<const char *>(input),
          reinterpret_cast<const char *>(input + count), length - count);
      res.count += count;
      return res;
    }
    reader.advance();
    count += 64;
  }
  uint8_t block[64]{};
  reader.get_remainder(block);
  simd::simd8x64<uint8_t> in(block);
  c.check_next_input(in);
  reader.advance();
  c.check_eof();
  if (c.errors()) {
    if (count != 0) {
      count--;
    } // Sometimes the error is only detected in the next chunk
    result res = scalar::utf8::rewind_and_validate_with_errors(
        reinterpret_cast<const char *>(input),
        reinterpret_cast<const char *>(input) + count, length - count);
    res.count += count;
    return res;
  } else {
    return result(error_code::SUCCESS, length);
  }
}

result generic_validate_utf8_with_errors(const char *input, size_t length) {
  return generic_validate_utf8_with_errors<utf8_checker>(
      reinterpret_cast<const uint8_t *>(input), length);
}

/**
 * Validates that the string is actual UTF-8 and stops on errors.
 * Tracks the amount of continuation and 4-byte leads.
 */
template <class checker>
utf8_result generic_validate_utf8_with_counts(const uint8_t *input,
                                              size_t length) {
  checker c{};
  buf_block_reader<64> reader(input, length);
  size_t count{0};
  while (reader.has_full_block()) {
    simd::simd8x64<uint8_t> in(reader.full_block());
    block_counts last_counts = c.check_next_input_with_counts(in);
    if (c.errors()) {
      utf8_result res = scalar::utf8::rewind_and_validate_with_counts(
          reinterpret_cast<const char *>(input),
          reinterpret_cast<const char *>(input + count), length - count);
      res.input_count += count;
      res.continuation_count +=
          c.continuation_count() - last_counts.continuations;
      res.four_byte_count += c.four_byte_count() - last_counts.four_byte;
      return res;
    }
    reader.advance();
    count += 64;
  }
  uint8_t block[64]{};
  reader.get_remainder(block);
  simd::simd8x64<uint8_t> in(block);
  block_counts last_counts = c.check_next_input_with_counts(in);
  reader.advance();
  c.check_eof();
  if (c.errors()) {
    utf8_result res = scalar::utf8::rewind_and_validate_with_counts(
        reinterpret_cast<const char *>(input),
        reinterpret_cast<const char *>(input) + count, length - count);
    res.input_count += count;
    res.continuation_count +=
        c.continuation_count() - last_counts.continuations;
    res.four_byte_count += c.four_byte_count() - last_counts.four_byte;
    return res;
  } else {
    return utf8_result(error_code::SUCCESS, length, c.continuation_count(),
                       c.four_byte_count());
  }
}

#if SIMDUTF_IMPLEMENTATION_ARM64
// NEON-optimized counting. Weaker arm cores (e.g. Graviton 2 / Neoverse N1)
// have a slow cross-lane reduction (vaddvq / vaddlvq). The generic path pays
// one such reduction per 16-byte chunk; here we instead accumulate the
// continuation and four-byte-lead comparison masks with cheap vertical adds
// into int8 lane accumulators (each match adds 0xFF == -1), and only reduce
// across lanes when the int8 lanes might overflow (every 124 chunks), on error,
// and at the end. This mirrors SimdUnicode's arm64 kernel.
struct neon_counter {
  int8x16_t contv = vdupq_n_s8(0);
  int8x16_t n4v = vdupq_n_s8(0);
  // Number of chunks accumulated since the last reduction. Each chunk adds at
  // most 1 (in magnitude) to any int8 lane, so we must reduce before 128.
  int pending = 0;
  size_t continuations = 0;
  size_t four_byte = 0;

  simdutf_really_inline void reduce() {
    // Lanes hold negative counts; negate the widening horizontal sum.
    continuations += size_t(-int(vaddlvq_s8(contv)));
    four_byte += size_t(-int(vaddlvq_s8(n4v)));
    contv = vdupq_n_s8(0);
    n4v = vdupq_n_s8(0);
    pending = 0;
  }

  simdutf_really_inline void accumulate_chunk(const simd8<uint8_t> chunk) {
    const uint8x16_t raw = chunk;
    // Continuation byte 0b10xxxxxx == signed int8 <= -65.
    contv = vaddq_s8(
        contv, vreinterpretq_s8_u8(vcleq_s8(vreinterpretq_s8_u8(raw),
                                            vdupq_n_s8(-65))));
    // Four-byte lead >= 0xF0, i.e. unsigned > 0xEF.
    n4v = vaddq_s8(n4v,
                   vreinterpretq_s8_u8(vcgtq_u8(raw, vdupq_n_u8(0xEF))));
  }

  simdutf_really_inline void accumulate(const simd::simd8x64<uint8_t> &in) {
    static_assert(simd::simd8x64<uint8_t>::NUM_CHUNKS == 4,
                  "arm64 processes four 16-byte chunks per 64-byte block.");
    accumulate_chunk(in.chunks[0]);
    accumulate_chunk(in.chunks[1]);
    accumulate_chunk(in.chunks[2]);
    accumulate_chunk(in.chunks[3]);
    pending += 4;
    if (pending >= 124) {
      reduce();
    }
  }
};

utf8_result neon_validate_utf8_with_counts(const uint8_t *input,
                                           size_t length) {
  utf8_checker c{};
  neon_counter counter{};
  buf_block_reader<64> reader(input, length);
  size_t count{0};
  while (reader.has_full_block()) {
    simd::simd8x64<uint8_t> in(reader.full_block());
    c.check_next_input(in);
    if (simdutf_unlikely(c.errors())) {
      // Counts accumulated so far exclude this failing block (we only
      // accumulate below, after the error check), so no adjustment is needed.
      counter.reduce();
      utf8_result res = scalar::utf8::rewind_and_validate_with_counts(
          reinterpret_cast<const char *>(input),
          reinterpret_cast<const char *>(input + count), length - count);
      res.input_count += count;
      res.continuation_count += counter.continuations;
      res.four_byte_count += counter.four_byte;
      return res;
    }
    if (!is_ascii(in)) {
      counter.accumulate(in);
    }
    reader.advance();
    count += 64;
  }
  // Finalize the counts for all complete blocks processed so far.
  counter.reduce();
  // Tail block: count it separately so we can exclude it on a tail error. The
  // zero padding introduced by get_remainder is ASCII and contributes nothing.
  uint8_t block[64]{};
  reader.get_remainder(block);
  simd::simd8x64<uint8_t> in(block);
  c.check_next_input(in);
  neon_counter tail{};
  if (!is_ascii(in)) {
    tail.accumulate(in);
  }
  tail.reduce();
  reader.advance();
  c.check_eof();
  if (simdutf_unlikely(c.errors())) {
    utf8_result res = scalar::utf8::rewind_and_validate_with_counts(
        reinterpret_cast<const char *>(input),
        reinterpret_cast<const char *>(input) + count, length - count);
    res.input_count += count;
    res.continuation_count += counter.continuations;
    res.four_byte_count += counter.four_byte;
    return res;
  }
  return utf8_result(error_code::SUCCESS, length,
                     counter.continuations + tail.continuations,
                     counter.four_byte + tail.four_byte);
}
#endif // SIMDUTF_IMPLEMENTATION_ARM64

utf8_result generic_validate_utf8_with_counts(const char *input,
                                              size_t length) {
#if SIMDUTF_IMPLEMENTATION_ARM64
  return neon_validate_utf8_with_counts(
      reinterpret_cast<const uint8_t *>(input), length);
#else
  return generic_validate_utf8_with_counts<utf8_segmenter>(
      reinterpret_cast<const uint8_t *>(input), length);
#endif
}

} // namespace utf8_validation
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
