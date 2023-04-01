#include "simdutf.cpp"
#include "simdutf.h"
#include <cassert>
#include <memory>
#include <span>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  for (const auto &implementation : simdutf::get_available_implementations()) {
    if (!implementation->supported_by_runtime_system()) {
      continue;
    }

    simdutf::get_active_implementation() = implementation;
    const char16_t *source = reinterpret_cast<const char16_t *>(data);
    size_t source_size = size / sizeof(char16_t);

    bool validutf16 = simdutf::validate_utf16(source, source_size);
    if (!validutf16) {
      return -1;
    }
    // We need a buffer of size where to write the UTF-8 words.
    size_t expected_utf8words =
        simdutf::utf8_length_from_utf16(source, source_size);
    std::unique_ptr<char[]> utf8_output{new char[expected_utf8words]};

    // convert to UTF-8
    size_t utf8words =
        simdutf::convert_utf16_to_utf8(source, source_size, utf8_output.get());

    // It wrote utf8_words * sizeof(char) bytes.
    if (simdutf::validate_utf8(utf8_output.get(), utf8words)) {
      return -1;
    }

    // convert it back:
    // We need a buffer of size where to write the UTF-16 words.
    size_t expected_utf16words =
        simdutf::utf16_length_from_utf8(utf8_output.get(), utf8words);
    std::unique_ptr<char16_t[]> utf16_output{new char16_t[expected_utf16words]};
    // convert to UTF-8
    size_t utf16words = simdutf::convert_utf8_to_utf16(
        utf8_output.get(), utf8words, utf16_output.get());

    // We need a buffer of size where to write the UTF-32 words.
    size_t expected_utf32words =
        simdutf::utf32_length_from_utf16(source, source_size);
    std::unique_ptr<char32_t[]> utf32_output{new char32_t[expected_utf32words]};
    // convert to UTF-32
    size_t utf32words = simdutf::convert_utf16_to_utf32(source, source_size,
                                                        utf32_output.get());
    // It wrote utf32words * sizeof(char32_t) bytes.
    if (!simdutf::validate_utf32(utf32_output.get(), utf16words)) {
      return -1;
    }
    // convert it back:
    // We need a buffer of size where to write the UTF-16 words.
    expected_utf16words =
        simdutf::utf16_length_from_utf32(utf32_output.get(), utf32words);
    std::unique_ptr<char16_t[]> utf16_from_utf32_output{
        new char16_t[expected_utf16words]};
    // convert to UTF-16
    utf16words = simdutf::convert_utf32_to_utf16(utf32_output.get(), utf32words,
                                                 utf16_from_utf32_output.get());
  }
  return 0;
}
