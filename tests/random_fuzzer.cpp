#include <memory>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "simdutf.h"

std::vector<char> input;

extern "C" {
void dump_case() {
  printf("Found a problem: ");
  for (size_t i = 0; i < input.size(); i++) {
    printf("\\x%02x", input[0] & 0xFF);
  }
  printf("\n");
  std::string name = "random_fuzzer_log.txt";
  std::cout << "Dumping to: " << name << std::endl;
  std::fstream log;
  log.open(name, std::ios::app);
  const size_t buf_size = 4 * input.size() + 3;
  char *buffer = new char[buf_size];
  for (int i = 0; i < input.size(); i++) {
    SIMDUTF_PUSH_DISABLE_WARNINGS
    SIMDUTF_DISABLE_DEPRECATED_WARNING
    sprintf(buffer + 4 * i + 1, "\\x%02x", input[i]);
    SIMDUTF_POP_DISABLE_WARNINGS
  }
  buffer[0] = '"';
  buffer[buf_size - 2] = '"';
  buffer[buf_size - 1] = '\0';
  log << buffer;
  log << std::endl;
  delete[] buffer;
  log.close();
}

void __asan_on_error() { dump_case(); }
}

size_t valid_utf8 = 0;
size_t valid_utf16 = 0;

/**
 * Returns false on error.
 */
bool fuzz_this(const char *data, size_t size) {
  valid_utf8 += simdutf::validate_utf8(data, size);
  valid_utf16 += simdutf::validate_utf16((const char16_t *)data, size / 2);

  for (auto &e : simdutf::get_available_implementations()) {
    if (!e->supported_by_runtime_system()) {
      continue;
    }
    /**
     * Transcoding from UTF-8 to UTF-16LE.
     */
    bool validutf8 = e->validate_utf8(data, size);
    if (validutf8) {
      // We need a buffer of size where to write the UTF-16LE words.
      size_t expected_utf16words = e->utf16_length_from_utf8(data, size);
      std::unique_ptr<char16_t[]> utf16_output{
          new char16_t[expected_utf16words]};
      // convert to UTF-16LE
      size_t utf16words =
          e->convert_utf8_to_utf16le(data, size, utf16_output.get());
      // It wrote utf16words * sizeof(char16_t) bytes.
      bool validutf16 = e->validate_utf16le(utf16_output.get(), utf16words);
      if (!validutf16) {
        return false;
      }
      // convert it back:
      // We need a buffer of size where to write the UTF-8 words.
      size_t expected_utf8words =
          e->utf8_length_from_utf16le(utf16_output.get(), utf16words);
      std::unique_ptr<char[]> utf8_output{new char[expected_utf8words]};
      // convert to UTF-8
      size_t utf8words = e->convert_utf16le_to_utf8(
          utf16_output.get(), utf16words, utf8_output.get());
      for (size_t k = 0; k < utf8words; k++) {
        if (data[k] != utf8_output.get()[k]) {
          return false;
        }
      }
    } else {
      // invalid input!!!
      // We need a buffer of size where to write the UTF-16LE words.
      size_t expected_utf16words = e->utf16_length_from_utf8(data, size);
      std::unique_ptr<char16_t[]> utf16_output{
          new char16_t[expected_utf16words]};
      // convert to UTF-16LE
      size_t utf16words =
          e->convert_utf8_to_utf16le(data, size, utf16_output.get());
      if (utf16words != 0) {
        return false;
      }
    }
    /**
     * Transcoding from UTF-8 to UTF-32.
     */
    if (validutf8) {
      // We need a buffer of size where to write the UTF-32 words.
      size_t expected_utf32words = e->utf32_length_from_utf8(data, size);
      std::unique_ptr<char32_t[]> utf32_output{
          new char32_t[expected_utf32words]};
      // convert to UTF-32
      size_t utf32words =
          e->convert_utf8_to_utf32(data, size, utf32_output.get());
      // It wrote utf32words * sizeof(char32_t) bytes.
      bool validutf32 = e->validate_utf32(utf32_output.get(), utf32words);
      if (!validutf32) {
        return false;
      }
      // convert it back:
      // We need a buffer of size where to write the UTF-8 words.
      size_t expected_utf8words =
          e->utf8_length_from_utf32(utf32_output.get(), utf32words);
      std::unique_ptr<char[]> utf8_output{new char[expected_utf8words]};
      // convert to UTF-8
      size_t utf8words = e->convert_utf32_to_utf8(
          utf32_output.get(), utf32words, utf8_output.get());
      for (size_t k = 0; k < utf8words; k++) {
        if (data[k] != utf8_output.get()[k]) {
          return false;
        }
      }
    } else {
      // invalid input!!!
      size_t expected_utf32words = e->utf32_length_from_utf8(data, size);
      std::unique_ptr<char32_t[]> utf32_output{
          new char32_t[expected_utf32words]};
      // convert to UTF-32
      size_t utf32words =
          e->convert_utf8_to_utf32(data, size, utf32_output.get());
      if (utf32words != 0) {
        return false;
      }
    }
    /**
     * Transcoding from UTF-16 to UTF-8.
     */
    bool validutf16 = e->validate_utf16le((char16_t *)data, size / 2);
    if (validutf16) {
      // We need a buffer of size where to write the UTF-16 words.
      size_t expected_utf8words =
          e->utf8_length_from_utf16le((char16_t *)data, size / 2);
      std::unique_ptr<char[]> utf8_output{new char[expected_utf8words]};
      size_t utf8words = e->convert_utf16le_to_utf8((char16_t *)data, size / 2,
                                                    utf8_output.get());
      // It wrote utf16words * sizeof(char16_t) bytes.
      bool validutf8 = e->validate_utf8(utf8_output.get(), utf8words);
      if (!validutf8) {
        return false;
      }
      // convert it back:
      // We need a buffer of size where to write the UTF-16 words.
      size_t expected_utf16words =
          e->utf16_length_from_utf8(utf8_output.get(), utf8words);
      std::unique_ptr<char16_t[]> utf16_output{
          new char16_t[expected_utf16words]};
      // convert to UTF-8
      size_t utf16words = e->convert_utf8_to_utf16le(
          utf8_output.get(), utf8words, utf16_output.get());
      for (size_t i = 0; i < size / 2; i++) {
        if (utf16_output.get()[i] != ((char16_t *)data)[i]) {
          return false;
        }
      }
    } else {
      // invalid input!!!
      // We need a buffer of size where to write the UTF-16 words.
      size_t expected_utf8words =
          e->utf8_length_from_utf16le((char16_t *)data, size / 2);
      std::unique_ptr<char[]> utf8_output{new char[expected_utf8words]};
      size_t utf8words = e->convert_utf16le_to_utf8((char16_t *)data, size / 2,
                                                    utf8_output.get());
      if (utf8words != 0) {
        return false;
      }
    }
  } // for (auto &e : simdutf::get_available_implementations()) {

  return true;
}

bool fuzz_running(size_t N) {
  std::mt19937 generator{std::random_device{}()};
  std::uniform_int_distribution<int> distribution{0, 255};
  std::uniform_int_distribution<int> length_distribution{0, 2048};

  for (size_t i = 0; i < N; i++) {
    if ((i % 1000) == 0) {
      printf(".");
      fflush(NULL);
    }
    size_t size = length_distribution(generator);
    input.resize(size);
    for (size_t k = 0; k < size; k++) {
      input[k] = char(distribution(generator));
    }
    if (!fuzz_this(input.data(), size)) {
      dump_case();
      return false;
    }
  }
  printf("\n");
  return true;
}

int main(int argc, char*argv[]) {
  std::cout << "testing the library on 'random garbage'" << std::endl;
  for (auto &e : simdutf::get_available_implementations()) {
    if (!e->supported_by_runtime_system()) {
      continue;
    }
    std::cout << "testing: " << e->name() << std::endl;
  }
  size_t N = 100000;
  if (argc == 2) {
    try {
      N = std::stoi(argv[1]);
    } catch (const std::exception& e) {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }
  }
  std::cout << "Number of strings: " << N << std::endl;
  if (fuzz_running(N)) {
    std::cout << "valid UTF8 = " << valid_utf8 << std::endl;
    std::cout << "valid UTF16 = " << valid_utf16 << std::endl;
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
