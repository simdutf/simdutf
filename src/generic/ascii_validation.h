namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace ascii_validation {

result generic_validate_ascii_with_errors(const char *input, size_t length) {
  size_t count = 0;
  if (length >= 64) {
    buf_block_reader<64> reader(reinterpret_cast<const uint8_t *>(input),
                                length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      if (!in.is_ascii()) {
        count = reader.block_index();
        result res = scalar::ascii::validate_with_errors(
            reinterpret_cast<const char *>(input + count), length - count);
        return result(res.error, count + res.count);
      }
      reader.advance();
    }
    count = reader.block_index();
    input += count;
    length -= count;
  }

  result res = scalar::ascii::validate_with_errors(
      reinterpret_cast<const char *>(input), length);
  return result(res.error, res.count + count);
}

bool generic_validate_ascii(const char *input, size_t length) {
  if (length >= 64) {
    buf_block_reader<64> reader(reinterpret_cast<const uint8_t *>(input),
                                length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      if (!in.is_ascii()) {
        return false;
      }
      reader.advance();
    }
    size_t count = reader.block_index();
    input += count;
    length -= count;
  }
  return scalar::ascii::validate(reinterpret_cast<const char *>(input), length);
}

} // namespace ascii_validation
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
