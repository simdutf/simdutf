namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_validation {

/**
 * Validates that the string is actual UTF-8.
 */
template <class checker>
bool generic_validate_utf8(const uint8_t *input, size_t length) {
  if (length >= 64) {
    checker c{};
    buf_block_reader<64> reader(input, length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      c.check_next_input(in);
      reader.advance();
    }
    if (c.errors()) {
      return false;
    }
    size_t count = reader.block_index();
    if (c.has_incomplete()) {
      size_t location = count - 1;
      while ((input[location] & 0xC0) == 0x80) {
        location--;
      }
      length -= location;
      input += location;
    } else {
      length -= count;
      input += count;
    }
  }
  return scalar::utf8::validate(reinterpret_cast<const char *>(input), length);
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
  size_t count = 0;
  if (length >= 64) {
    checker c{};
    buf_block_reader<64> reader(input, length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      c.check_next_input(in);
      if (c.errors()) {
        count = reader.block_index();
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
    }
    count = reader.block_index();
    if (c.has_incomplete()) {
      size_t location = count - 1;
      while ((input[location] & 0xC0) == 0x80) {
        location--;
      }
      count = location;
    }
    length -= count;
    input += count;
  }
  auto r = scalar::utf8::validate_with_errors(
      reinterpret_cast<const char *>(input), length);
  r.count += count;
  return r;
}

result generic_validate_utf8_with_errors(const char *input, size_t length) {
  return generic_validate_utf8_with_errors<utf8_checker>(
      reinterpret_cast<const uint8_t *>(input), length);
}

} // namespace utf8_validation
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
