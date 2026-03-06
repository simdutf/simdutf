#include <cassert>
#include <vector>

#include "simdutf.h"

void test_latin1_to_utf8(std::span<const uint8_t> input_bytes,
                         std::size_t output_size) {
  std::vector<char> output(output_size);
  const auto written_bytes_safe =
      simdutf::convert_latin1_to_utf8_safe(input_bytes, output);
  if (written_bytes_safe > output_size) {
    std::abort();
  }
  const auto needed_size = simdutf::utf8_length_from_latin1(input_bytes);
  std::vector<char> reference(needed_size);
  const auto written_bytes_unsafe =
      simdutf::convert_latin1_to_utf8(input_bytes, reference);
  if (written_bytes_unsafe != needed_size) {
    std::abort();
  }
  if (written_bytes_safe > needed_size) {
    // convert_latin1_to_utf8_safe wrote more output buffer than the unsafe
    // version needed!
    std::abort();
  }
  // ensure output is equal to the beginning of reference
  if (!std::ranges::equal(
          std::span(output).subspan(0, written_bytes_safe),
          std::span(reference).subspan(0, written_bytes_safe))) {
    std::abort();
  }
}

void test_utf16_to_utf8(std::span<const char16_t> input,
                        std::size_t output_size) {
  std::vector<char> output(output_size);
  const auto written_bytes_safe =
      simdutf::convert_utf16_to_utf8_safe(input, output);
  if (written_bytes_safe > output_size) {
    std::abort();
  }
  // result is implementation defined in case of garbage input
  const auto unreliable_needed_size = simdutf::utf8_length_from_utf16(input);
  std::vector<char> reference(unreliable_needed_size);
  const auto written_bytes_unsafe =
      simdutf::convert_utf16_to_utf8(input, reference);

  // ensure output is equal to the beginning of reference
  const auto Ncompare = std::min(written_bytes_safe, written_bytes_unsafe);
  const auto matches =
      std::ranges::equal(std::span(output).subspan(0, Ncompare),
                         std::span(reference).subspan(0, Ncompare));
  assert(matches);
  if (!matches) {
    std::abort();
  }
}

void select_implementation(auto index) {
  static const auto implementations = []() {
    const auto list = simdutf::get_available_implementations();
    using Impl = std::decay_t<decltype(*list.begin())>;
    std::vector<Impl> ret;
    for (auto& e : list) {
      if (e->supported_by_runtime_system()) {
        ret.push_back(e);
      }
    }
    return ret;
  }();
  assert(!implementations.empty());
  simdutf::get_active_implementation() =
      implementations.at(index % implementations.size());
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {

  if (size < 4) {
    return 0;
  }

  const auto action = data[0] & 0x1;
  const auto output_size = (data[1] << 8 | data[2]);
  const auto implementation_index = data[3] & 0b0111;
  data += 4;
  size -= 4;

  const std::span<const uint8_t> input_bytes{data, data + size};

  select_implementation(implementation_index);

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
