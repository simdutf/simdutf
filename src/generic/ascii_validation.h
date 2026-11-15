namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace ascii_validation {

template <typename T> simdutf_really_inline T load_bytes_as(const uint8_t *p) {
  T v;
  std::memcpy(&v, p, sizeof(T));
  return v;
}

// Returns true if the n bytes at p are all ASCII, reading only those bytes.
// The validators use it on the last, partial block instead of copying that
// block to the stack, which would stall the vector load of the copy.
simdutf_really_inline bool tail_is_ascii(const uint8_t *p, size_t n) {
  uint64_t acc = 0;
  if (n >= 16) {
    for (size_t i = 0; i + 16 < n; i += 16) {
      acc |=
          load_bytes_as<uint64_t>(p + i) | load_bytes_as<uint64_t>(p + i + 8);
    }
    acc |= load_bytes_as<uint64_t>(p + n - 16) |
           load_bytes_as<uint64_t>(p + n - 8);
  } else if (n >= 8) {
    acc = load_bytes_as<uint64_t>(p) | load_bytes_as<uint64_t>(p + n - 8);
  } else if (n >= 4) {
    acc = load_bytes_as<uint32_t>(p) | load_bytes_as<uint32_t>(p + n - 4);
  } else {
    for (size_t i = 0; i < n; i++) {
      acc |= p[i];
    }
  }
  return (acc & 0x8080808080808080) == 0;
}

// Like tail_is_ascii(buf + idx, len - idx), for the remainder that
// buf_block_reader leaves at idx (1 to 64 bytes, unless len is 0). The caller
// must already know that the bytes before idx are ASCII. Inputs of at least
// one block are checked with one load of their last block instead, which
// overlaps bytes that were already checked: a non-ASCII byte there can only be
// at or after idx.
simdutf_really_inline bool remainder_is_ascii(const uint8_t *buf, size_t len,
                                              size_t idx) {
  constexpr size_t block = sizeof(simd::simd8x64<uint8_t>);
  if (len >= block) {
    return simd::simd8x64<uint8_t>(buf + len - block).is_ascii();
  }
  return tail_is_ascii(buf + idx, len - idx);
}

result generic_validate_ascii_with_errors(const char *input, size_t length) {
  buf_block_reader<64> reader(reinterpret_cast<const uint8_t *>(input), length);
  size_t count{0};
  while (reader.has_full_block()) {
    simd::simd8x64<uint8_t> in(reader.full_block());
    if (!in.is_ascii()) {
      SIMDUTF_VZEROUPPER();
      result res = scalar::ascii::validate_with_errors(
          reinterpret_cast<const char *>(input + count), length - count);
      return result(res.error, count + res.count);
    }
    reader.advance();

    count += 64;
  }
  if (remainder_is_ascii(reinterpret_cast<const uint8_t *>(input), length,
                         reader.block_index())) {
    return result(error_code::SUCCESS, length);
  }
  SIMDUTF_VZEROUPPER();
  result res = scalar::ascii::validate_with_errors(
      reinterpret_cast<const char *>(input + count), length - count);
  return result(res.error, count + res.count);
}

bool generic_validate_ascii(const char *input, size_t length) {
  buf_block_reader<64> reader(reinterpret_cast<const uint8_t *>(input), length);
  while (reader.has_full_block()) {
    simd::simd8x64<uint8_t> in(reader.full_block());
    if (!in.is_ascii()) {
      return false;
    }
    reader.advance();
  }
  return remainder_is_ascii(reinterpret_cast<const uint8_t *>(input), length,
                            reader.block_index());
}

} // namespace ascii_validation
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
