#include <fuzzer/FuzzedDataProvider.h>
#include <memory>
#include "simdutf.cpp"
#include "simdutf.h"
#include <string>
#include <cassert>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  std::string source = fdp.ConsumeRandomLengthString(1024);
  bool validutf8 = simdutf::validate_utf8(source.c_str(), source.size());
  if (!validutf8) {
    return -1;
  }   
  // We need a buffer of size where to write the UTF-16LE words.
  size_t expected_utf16words = simdutf::utf16_length_from_utf8(source.c_str(), source.size());
  std::unique_ptr<char16_t[]> utf16_output{new char16_t[expected_utf16words]};
  // convert to UTF-16LE
  size_t utf16words =
      simdutf::convert_utf8_to_utf16le(source.c_str(), source.size(), utf16_output.get());
  // It wrote utf16words * sizeof(char16_t) bytes.
  bool validutf16 = simdutf::validate_utf16le(utf16_output.get(), utf16words);
  if (!validutf16) {
    return -1;
  }   
  // convert it back:
  // We need a buffer of size where to write the UTF-8 words.
  size_t expected_utf8words =
      simdutf::utf8_length_from_utf16le(utf16_output.get(), utf16words);
  std::unique_ptr<char[]> utf8_output{new char[expected_utf8words]};
  // convert to UTF-8
  size_t utf8words = simdutf::convert_utf16le_to_utf8(
      utf16_output.get(), utf16words, utf8_output.get());
  std::string final_string(utf8_output.get(), utf8words);
  return 0;
}
