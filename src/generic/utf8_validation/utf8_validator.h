namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_validation {

// Checks the bytes from idx on, after the last full block. When the bytes
// before the window (the last 64 bytes of the input) fill a register, the
// window is checked in place: the bytes that the last full block already
// checked are checked again with the same preceding bytes, so they give the
// same result. An ASCII tail cannot start or finish a multibyte character, so
// it only needs check_eof(), which reports a character that the last full
// block left incomplete. Shorter inputs copy the tail to the stack.
template <class checker>
simdutf_really_inline void check_remainder(checker &c, const uint8_t *input,
                                           size_t length, size_t idx) {
  constexpr size_t block = sizeof(simd::simd8x64<uint8_t>);
  constexpr size_t width = sizeof(simd::simd8<uint8_t>);
  static_assert(block == 8 * sizeof(uint64_t), "one mask bit per byte");
  if (length >= block + width) {
    // The window starts at or before idx: buf_block_reader leaves 1 to 64
    // bytes, so 0 <= idx - start < 64.
    const size_t start = length - block;
    const simd::simd8x64<uint8_t> in(input + start);
    // Bit i is set if byte start + i is not ASCII, so the bytes from idx on
    // are the top length - idx bits. is_ascii() goes first because GCC, up
    // to at least GCC 17, builds gteq_unsigned(0x80) from min and compare
    // instead of one movemask, which costs ASCII tails about 0.5 ns.
    if (!in.is_ascii() && (in.gteq_unsigned(0x80) >> (idx - start)) != 0) {
      c.check_overlapping_input(in,
                                simd::simd8<uint8_t>(input + start - width));
    }
  } else if (idx != length) {
    // Only an empty input leaves no bytes. Its pointer may be null, and
    // memcpy from a null pointer is undefined.
    uint8_t buffer[block]{};
    std::memcpy(buffer, input + idx, length - idx);
    c.check_next_input(simd::simd8x64<uint8_t>(buffer));
  }
  c.check_eof();
}

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
  check_remainder(c, input, length, reader.block_index());
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
      SIMDUTF_VZEROUPPER();
      result res = scalar::utf8::rewind_and_validate_with_errors(
          reinterpret_cast<const char *>(input),
          reinterpret_cast<const char *>(input + count), length - count);
      res.count += count;
      return res;
    }
    reader.advance();
    count += 64;
  }
  check_remainder(c, input, length, reader.block_index());
  if (c.errors()) {
    if (count != 0) {
      count--;
    } // Sometimes the error is only detected in the next chunk
    SIMDUTF_VZEROUPPER();
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
      SIMDUTF_VZEROUPPER();
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
    SIMDUTF_VZEROUPPER();
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

simdutf_really_inline utf8_result
generic_validate_utf8_with_counts(const char *input, size_t length) {
  return generic_validate_utf8_with_counts<utf8_segmenter>(
      reinterpret_cast<const uint8_t *>(input), length);
}

} // namespace utf8_validation
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
