namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace ascii_validation {


bool generic_validate_ascii(const char *in, size_t length) {
  constexpr size_t N = simd8<uint8_t>::SIZE;
  auto counters1 = simd8<uint8_t>::zero();
  auto counters2 = simd8<uint8_t>::zero();
  auto counters3 = simd8<uint8_t>::zero();
  auto counters4 = simd8<uint8_t>::zero();
  size_t pos = 0;
  for (; pos + 4*N <= length; pos += 4*N) {
    const auto input1 =
        simd8<uint8_t>::load(reinterpret_cast<const uint8_t *>(in + pos));
    counters1 |= input1;
    const auto input2 =
        simd8<uint8_t>::load(reinterpret_cast<const uint8_t *>(in + pos + N));
    counters2 |= input2;
    const auto input3 =
        simd8<uint8_t>::load(reinterpret_cast<const uint8_t *>(in + pos + 2*N));
    counters3 |= input3;
    const auto input4 =
        simd8<uint8_t>::load(reinterpret_cast<const uint8_t *>(in + pos + 3*N));
    counters4 |= input4;
  }
  auto counters = counters1 | counters2 | counters3 | counters4;
  for (; pos + N <= length; pos += N) {
    const auto input =
        simd8<uint8_t>::load(reinterpret_cast<const uint8_t *>(in + pos));
    counters |= input;
  }
  if(pos != length) { // if so, we are now at the end with less than N bytes left
    if(length > N) {
      const auto input =
          simd8<uint8_t>::load(reinterpret_cast<const uint8_t *>(in + length - N));
      counters |= input;
    } else {
      // process remaining bytes one by one (it is fine, we are at the end)
      for(size_t i=pos;i<length;i++) {
        if(uint8_t(in[i]) > 0x7F) {
          return false;
        }
      }
    }
  }
  return counters.is_ascii();
}


result generic_validate_ascii_with_errors(const char *input, size_t length) {
  buf_block_reader<64> reader(reinterpret_cast<const uint8_t *>(input), length);
  size_t count{0};
  while (reader.has_full_block()) {
    simd::simd8x64<uint8_t> in(reader.full_block());
    if (!in.is_ascii()) {
      result res = scalar::ascii::validate_with_errors(
          reinterpret_cast<const char *>(input + count), length - count);
      return result(res.error, count + res.count);
    }
    reader.advance();

    count += 64;
  }
  uint8_t block[64]{};
  reader.get_remainder(block);
  simd::simd8x64<uint8_t> in(block);
  if (!in.is_ascii()) {
    result res = scalar::ascii::validate_with_errors(
        reinterpret_cast<const char *>(input + count), length - count);
    return result(res.error, count + res.count);
  } else {
    return result(error_code::SUCCESS, length);
  }
}

} // namespace ascii_validation
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
