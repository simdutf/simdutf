namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace base64_lengths {


simdutf_warn_unused size_t binary_length_from_base64(const char *input, size_t length) {
  size_t pos = 0;
  size_t count = 0;
  for (; pos + 64 <= length; pos += 64) {
    simd8x64<uint8_t> block(reinterpret_cast<const uint8_t *>(input + pos));
    uint64_t maybe_base64 = block.gt(32);
    count += count_ones(maybe_base64);
  }
  while(pos < length) {
    count += (input[pos] > 0x20) ? 1 : 0;
    pos++;
  }
  // Count padding at the end.
  size_t padding = 0;
  pos = length;
  while (pos > 0 && padding < 2) {
    char c = input[--pos];
    if (c == '=') {
      padding++;
    } else if (c > ' ') {
      break;
    }
  }
  return ((count - padding) * 3) / 4;
}


simdutf_warn_unused size_t binary_length_from_base64(const char16_t *input, size_t length) {
  size_t pos = 0;
  size_t count = 0;
  for (; pos + 32 <= length; pos += 32) {
    simd16x32<uint16_t> block(reinterpret_cast<const uint16_t *>(input + pos));
    uint64_t maybe_base64 = block.gt(32);
    count += count_ones(maybe_base64);
  }
  while(pos < length) {
    count += (input[pos] > 0x20) ? 1 : 0;
    pos++;
  }
  // Count padding at the end.
  size_t padding = 0;
  pos = length;
  while (pos > 0 && padding < 2) {
    char16_t c = input[--pos];
    if (c == '=') {
      padding++;
    } else if (c > ' ') {
      break;
    }
  }
  return ((count - padding) * 3) / 4;
}

} // namespace base64_lengths
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf