[![Alpine Linux](https://github.com/lemire/simdutf/actions/workflows/alpine.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/alpine.yml)
[![MSYS2-CI](https://github.com/lemire/simdutf/actions/workflows/msys2.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/msys2.yml)
[![MSYS2-CLANG-CI](https://github.com/lemire/simdutf/actions/workflows/msys2-clang.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/msys2-clang.yml)
[![Ubuntu 20.04 CI (GCC 9)](https://github.com/lemire/simdutf/actions/workflows/ubuntu20sani.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/ubuntu20sani.yml)
[![VS16-ARM-CI](https://github.com/lemire/simdutf/actions/workflows/vs16-arm-ci.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/vs16-arm-ci.yml)
[![VS16-CI](https://github.com/lemire/simdutf/actions/workflows/vs16-ci.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/vs16-ci.yml)

simdutf: insanely fast Unicode validation and transcoding
===============================================

Most modern software relies on the [Unicode standard](https://en.wikipedia.org/wiki/Unicode). In memory, Unicode strings are represented using either
UTF-8 or UTF-16. The UTF-8 format is the de facto standard on the web (JSON, HTML, etc.) and it has been adopted as the default in many popular
programming languages (Go, Rust, Swift, etc.). The UTF-16 format is standard in Java, C# and in many Windows technologies.

Not all sequences of bytes are valid Unicode strings. It is unsafe to use Unicode strings in UTF-8 and UTF-16 without first validating them. Furthermore, we often need to convert strings from one encoding to another, by a process called [transcoding](https://en.wikipedia.org/wiki/Transcoding). For security purposes, such transcoding should be validating: it should refuse to transcode incorrect strings.

This library provide fast Unicode functions such as

- UTF-8 and UTF-16 validation
- UTF-8 to UTF-16 transcoding, with or without validation.
- UTF-16 to UTF-8 transcoding, with or without validation.
- UTF-8 and UTF-16 character counting

The functions are accelerated using SIMD instructions (e.g., ARM NEON, SSE, AVX, etc.). When your strings contain hundreds of characters, we can often transcode them at speeds exceeding a billion caracters per second. You should expect high speeds not only with English strings (ASCII) but also Chinese, Japanese, Arabic, and so forth. We handle the full character range (including, for example, emojis).

The library compiles down to tens of kilobytes. Our functions are exception-free and non allocating. We have extensive tests.

Requirements
-------

- C++11 compatible compiler. We support LLVM clang, GCC, Visual Studio. (Our optional benchmark tool requires C++17.)
- Recent CMake (at least 3.15)

Usage
-------

```
cmake -B build
cmake --build build
cd build
ctest .
```

To run benchmarks, execute the `benchmark` command. You can get help on its
usage by first building it and then calling it with the `--help` flag.
E.g., under Linux you may do the following:

```
cmake -B build
cmake --build build
./build/benchmarks/benchmark --help
```

Single-header version
----------------------

You can create a single-header version of the library where
all of the code is put into two files (`simdutf.h` and `simdutf.cpp`).

```
python3 ./singleheader/amalgamate.py
```

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
#include "simdutf.h"
#include "simdutf.cpp"

int main(int argc, char *argv[]) {
   const char * ascii = "1234";
   bool validutf8 = simdutf::validate_utf8(ascii, 4);
   if(validutf8) {
       std::cout << "valid UTF-8" << std::endl;
   } else {
       std::cout << "invalid UTF-8" << std::endl;
       return EXIT_FAILURE;
   }
   char16_t utf16_output[4];
   // convert to UTF-16LE
   size_t utf16words = simdutf::convert_utf8_to_utf16(ascii, 4, utf16_output);
   std::cout << "wrote " << utf16words << " UTF-16 words." << std::endl;
   // It wrote utf16words * sizeof(char16_t) bytes.
   //
   // convert it back:
   char buffer[4];
   size_t utf8words = simdutf::convert_utf16_to_utf8(utf16_output, utf16words, buffer);
   std::cout << "wrote " << utf8words << " UTF-8 words." << std::endl;
   return EXIT_SUCCESS;
}
```

API
-----

```C++
namespace simdutf {

/**
 * Validate the UTF-8 string.
 *
 * @param input the string to validate.
 * @param length the length of the string in bytes.
 * @return true if the string is valid UTF-8.
 */
simdutf_warn_unused bool validate_utf8(const char * input, size_t length) noexcept;

/**
 * Validate the UTF-16 string.
 *
 * @param buf the string to validate.
 * @param len the length of the string in bytes.
 * @return true if the string is valid UTF-16.
 */
simdutf_warn_unused bool validate_utf16(const char16_t * buf, size_t len) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written bytes; 0 if the input was not valid UTF-8 string
 */
simdutf_warn_unused size_t convert_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert valid UTF-8 string into UTF-16 string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t words
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-16 string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the string to convert
 * @param length        the length of the string in bytes
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written char16_t words; 0 if input is not a valid UTF-16 string
 */
simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-16 string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16.
 *
 * @param input         the string to convert
 * @param length        the length of the string in bytes
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written bytes; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept;


/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16.
 *
 * This function is not BOM-aware.
 *
 * @param input         the string to process
 * @param length        the length of the string in words
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf16(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the string to process
 * @param length        the length of the string in bytes
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf8(const char * input, size_t length) noexcept;

}
```

Testing data
------------

We recommend the following data repository.

https://github.com/lemire/unicode_lipsum

License
-------

This code is made available under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0.html) as well as the MIT license.

We include a few competitive solutions under the benchmarks/competition directory. They are provided for
research purposes only.
