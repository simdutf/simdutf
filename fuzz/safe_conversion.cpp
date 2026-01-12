#include <vector>

#include "simdutf.h"

void test_latin1_to_utf8(std::span<const uint8_t> input_bytes,
                         std::size_t output_size) {
  std::vector<char> output(output_size);
  const auto res = simdutf::convert_latin1_to_utf8_safe(input_bytes, output);
  if (res > output_size) {
    std::abort();
  }
}

void test_utf16_to_utf8(std::span<const char16_t> input,
                        std::size_t output_size) {
  std::vector<char> output(output_size);
  const auto res = simdutf::convert_utf16_to_utf8_safe(input, output);
  if (res > output_size) {
    std::abort();
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {

  if (size < 4) {
    return 0;
  }

  const auto action = data[0] & 0x1;
  const auto output_size = (data[1] << 8 | data[2]);
  data += 4;
  size -= 4;

  const std::span<const uint8_t> input_bytes{data, data + size};

  switch (action) {
  case 0:
    test_latin1_to_utf8(input_bytes, output_size);
    break;
  case 1: {
    const auto* ptr = reinterpret_cast<const char16_t*>(input_bytes.data());
    test_utf16_to_utf8(std::span(ptr, ptr + input_bytes.size() / 2),
                       output_size);
  } break;
  }

  return 0;
}
