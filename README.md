[![Alpine Linux](https://github.com/lemire/simdutf/actions/workflows/alpine.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/alpine.yml)
[![MSYS2-CI](https://github.com/lemire/simdutf/actions/workflows/msys2.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/msys2.yml)
[![MSYS2-CLANG-CI](https://github.com/lemire/simdutf/actions/workflows/msys2-clang.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/msys2-clang.yml)
[![Ubuntu 20.04 CI (GCC 9)](https://github.com/lemire/simdutf/actions/workflows/ubuntu20sani.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/ubuntu20sani.yml)

simdutf: Unicode validation and transcoding at billions of characters per second
===============================================

Most modern software relies on the [Unicode standard](https://en.wikipedia.org/wiki/Unicode). In memory, Unicode strings are represented using either
UTF-8 or UTF-16. The UTF-8 format is the de facto standard on the web (JSON, HTML, etc.) and it has been adopted as the default in many popular
programming languages (Go, Rust, Swift, etc.). The UTF-16 format is standard in Java, C# and in many Windows technologies.

Not all sequences of bytes are valid Unicode strings. It is unsafe to use Unicode strings in UTF-8 and UTF-16LE without first validating them. Furthermore, we often need to convert strings from one encoding to another, by a process called [transcoding](https://en.wikipedia.org/wiki/Transcoding). For security purposes, such transcoding should be validating: it should refuse to transcode incorrect strings.

This library provide fast Unicode functions such as

- ASCII, UTF-8, UTF-16LE and UTF-32LE validation,
- UTF-8 to UTF-16LE transcoding, with or without validation,
- UTF-8 to UTF-32LE transcoding, with or without validation,
- UTF-16LE to UTF-8 transcoding, with or without validation,
- UTF-32LE to UTF-8 transcoding, with or without validation,
- UTF-32LE to UTF-16LE transcoding, with or without validation,
- UTF-16LE to UTF-32LE transcoding, with or without validation,
- From an UTF-8 string, compute the size of the UTF-16 equivalent string,
- From an UTF-8 string, compute the size of the UTF-32 equivalent string (equivalent to UTF-8 character counting),
- From an UTF-16 string, compute the size of the UTF-8 equivalent string,
- From an UTF-32 string, compute the size of the UTF-8 or UTF-16LE equivalent string,
- From an UTF-16 string, compute the size of the UTF-32 equivalent string (equivalent to UTF-16 character counting),
- UTF-8 and UTF-16LE character counting.

The functions are accelerated using SIMD instructions (e.g., ARM NEON, SSE, AVX, etc.). When your strings contain hundreds of characters, we can often transcode them at speeds exceeding a billion characters per second. You should expect high speeds not only with English strings (ASCII) but also Chinese, Japanese, Arabic, and so forth. We handle the full character range (including, for example, emojis).

The library compiles down to tens of kilobytes. Our functions are exception-free and non allocating. We have extensive tests.

How fast is it?
-----------------

Over a wide range of realistic data sources, we transcode a billion characters per second or more. Our approach can be 3 to 10 times faster than the popular ICU library on difficult (non-ASCII) strings. We can be 20x faster than ICU when processing easy strings (ASCII). Our good results apply to both recent x64 and ARM processors.


To illustrate, we present a benchmark result with values are in billions of characters processed by second. Consider the following figures.



<img src="doc/utf8utf16.png" width="70%" />

<img src="doc/utf16utf8.png" width="70%" />


Datasets: https://github.com/lemire/unicode_lipsum

Please refer to our benchmarking tool for a proper interpretation of the numbers. Our results are reproducible.



Requirements
-------

- C++11 compatible compiler. We support LLVM clang, GCC, Visual Studio. (Our optional benchmark tool requires C++17.)
- For high speed, you should have a recent 64-bit system (e.g., ARM or x64).
- If you rely on CMake, you should use a recent CMake (at least 3.15) ; otherwise you may use the [single header version](#single-header-version). The library is also available from Microsoft's vcpkg.


Usage (Usage)
-------


We made a video to help you get started with the library.

[![the simdutf library](http://img.youtube.com/vi/H9NZtb7ykYs/0.jpg)](https://www.youtube.com/watch?v=H9NZtb7ykYs)<br />



Usage (CMake)
-------

```
cmake -B build
cmake --build build
cd build
ctest .
```

Visual Studio users must specify whether they want to build the Release or Debug version.

To run benchmarks, execute the `benchmark` command. You can get help on its
usage by first building it and then calling it with the `--help` flag.
E.g., under Linux you may do the following:

```
cmake -B build
cmake --build build
./build/benchmarks/benchmark --help
```

Instructions are similar for Visual Studio users.

Since ICU is so common and popular, we assume that you may have it already on your system. When
it is not found, it is simply omitted from the benchmarks. Thus, to benchmark against ICU, make
sure you have ICU installed on your machine and that cmake can find it. For macOS, you may
install it with brew using `brew install icu4c`. If you have ICU on your system but cmake cannot
find it, you may need to provide cmake with a path to ICU, such as `ICU_ROOT=/usr/local/opt/icu4c cmake -B build`.

Single-header version
----------------------

You can create a single-header version of the library where
all of the code is put into two files (`simdutf.h` and `simdutf.cpp`).
We publish a zip archive containing these files, e.g., see
https://github.com/simdutf/simdutf/releases/download/v1.0.1/singleheader.zip

You may generate it on your own using a Python script.

```
python3 ./singleheader/amalgamate.py
```

We require Python 3 or better.

Under Linux and macOS, you may test it as follows:

```
cd singleheader
c++ -o amalgamation_demo amalgamation_demo.cpp -std=c++17
./amalgamation_demo
```

Example
---------

Using the single-header version, you could compile the following program.

```C++
#include <iostream>
#include <memory>

#include "simdutf.cpp"
#include "simdutf.h"

int main(int argc, char *argv[]) {
  const char *source = "1234";
  // 4 == strlen(source)
  bool validutf8 = simdutf::validate_utf8(source, 4);
  if (validutf8) {
    std::cout << "valid UTF-8" << std::endl;
  } else {
    std::cerr << "invalid UTF-8" << std::endl;
    return EXIT_FAILURE;
  }
  // We need a buffer of size where to write the UTF-16LE words.
  size_t expected_utf16words = simdutf::utf16_length_from_utf8(source, 4);
  std::unique_ptr<char16_t[]> utf16_output{new char16_t[expected_utf16words]};
  // convert to UTF-16LE
  size_t utf16words =
      simdutf::convert_utf8_to_utf16(source, 4, utf16_output.get());
  std::cout << "wrote " << utf16words << " UTF-16LE words." << std::endl;
  // It wrote utf16words * sizeof(char16_t) bytes.
  bool validutf16 = simdutf::validate_utf16(utf16_output.get(), utf16words);
  if (validutf16) {
    std::cout << "valid UTF-16LE" << std::endl;
  } else {
    std::cerr << "invalid UTF-16LE" << std::endl;
    return EXIT_FAILURE;
  }
  // convert it back:
  // We need a buffer of size where to write the UTF-8 words.
  size_t expected_utf8words =
      simdutf::utf8_length_from_utf16(utf16_output.get(), utf16words);
  std::unique_ptr<char[]> utf8_output{new char[expected_utf8words]};
  // convert to UTF-8
  size_t utf8words = simdutf::convert_utf16_to_utf8(
      utf16_output.get(), utf16words, utf8_output.get());
  std::cout << "wrote " << utf8words << " UTF-8 words." << std::endl;
  std::string final_string(utf8_output.get(), utf8words);
  std::cout << final_string << std::endl;
  if (final_string != source) {
    std::cerr << "bad conversion" << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cerr << "perfect round trip" << std::endl;
  }
  return EXIT_SUCCESS;
}
```

API
-----

Our API is made of a few non-allocating function. They typically take a pointer and a length as a parameter,
and they sometimes take a pointer to an output buffer. Users are responsible for memory allocation.

```C++
namespace simdutf {

/**
 * Validate the ASCII string.
 *
 * Overridden by each implementation.
 *
 * @param buf the ASCII string to validate.
 * @param len the length of the string in bytes.
 * @return true if and only if the string is valid ASCII.
 */
simdutf_warn_unused bool validate_ascii(const char *buf, size_t len) noexcept;

/**
 * Validate the UTF-8 string.
 *
 * Overridden by each implementation.
 *
 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return true if and only if the string is valid UTF-8.
 */
simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) noexcept;

/**
 * Validate the UTF-16LE string.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16LE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return true if and only if the string is valid UTF-16LE.
 */
simdutf_warn_unused bool validate_utf16(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-32LE string.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-32LE string to validate.
 * @param len the length of the string in number of 4-byte words (char32_t).
 * @return true if and only if the string is valid UTF-32LE.
 */
simdutf_warn_unused bool validate_utf32(const char32_t *buf, size_t len) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16LE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
 */
simdutf_warn_unused size_t convert_utf8_to_utf16(const char * input, size_t length, char16_t* utf8_output) noexcept;

/**
 * Convert valid UTF-8 string into UTF-16LE string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Compute the number of 2-byte words that this UTF-8 string would require in UTF-16LE format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return the number of char16_t words required to encode the UTF-8 string as UTF-16LE
 */
simdutf_warn_unused size_t utf16_length_from_utf8(const char * input, size_t length) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-32LE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char32_t; 0 if the input was not valid UTF-8 string
 */
simdutf_warn_unused size_t convert_utf8_to_utf32(const char * input, size_t length, char32_t* utf32_output) noexcept;

/**
 * Convert valid UTF-8 string into UTF-32LE string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char32_t
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf32(const char * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Compute the number of 4-byte words that this UTF-8 string would require in UTF-32LE format.
 *
 * This function is equivalent to count_utf8
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return the number of char32_t words required to encode the UTF-8 string as UTF-32LE
 */
simdutf_warn_unused size_t utf32_length_from_utf8(const char * input, size_t length) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-16LE string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Compute the number of bytes that this UTF-16LE string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-8
 */
simdutf_warn_unused size_t utf8_length_from_utf16(const char16_t * input, size_t length) noexcept;

/**
 * Convert possibly broken UTF-32LE string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32LE string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32LE string
 */
simdutf_warn_unused size_t convert_utf32_to_utf8(const char32_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-32LE string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-32LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32LE string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf32_to_utf8(const char32_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-32LE string into UTF-16LE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32LE string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32LE string
 */
simdutf_warn_unused size_t convert_utf32_to_utf16(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-32LE string into UTF-16LE string.
 *
 * This function assumes that the input string is valid UTF-32LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32LE string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf32_to_utf16(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Compute the number of bytes that this UTF-32LE string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-32LE string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @return the number of bytes required to encode the UTF-32LE string as UTF-8
 */
simdutf_warn_unused size_t utf8_length_from_utf32(const char32_t * input, size_t length) noexcept;

/**
 * Compute the number of two-byte words that this UTF-32LE string would require in UTF-16 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-32LE string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @return the number of bytes required to encode the UTF-32LE string as UTF-16
 */
simdutf_warn_unused size_t utf16_length_from_utf32(const char32_t * input, size_t length) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-32LE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
simdutf_warn_unused size_t convert_utf16_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert valid UTF-16LE string into UTF-32LE string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Compute the number of bytes that this UTF-16LE string would require in UTF-32LE format.
 *
 * This function is equivalent to count_utf16.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-32LE
 */
simdutf_warn_unused size_t utf32_length_from_utf16(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf16(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf8(const char * input, size_t length) noexcept;


}
```

Usage
-----

The library used by [haskell/text](https://github.com/haskell/text).

References
-----------

* Daniel Lemire, Wojciech Muła,  [Transcoding Billions of Unicode Characters per Second with SIMD Instructions](https://arxiv.org/abs/2109.10433), Software: Practice and Experience (to appear)

License
-------

This code is made available under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0.html) as well as the MIT license.

We include a few competitive solutions under the benchmarks/competition directory. They are provided for
research purposes only.
