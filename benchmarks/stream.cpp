/**
 * This simple program is meant to simply decode substrings of a given
 * input file.
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>

#ifdef __has_include
  #if __has_include(<iconv.h>)
    #ifndef ICONV_AVAILABLE
      #define ICONV_AVAILABLE 1
    #endif
    #include <iconv.h>
  #endif //__has_include (<iconv.h>)
#endif   // defined __has_include

#if ICU_AVAILABLE
  #include <unicode/uconfig.h>
  #include <unicode/platform.h>
  #include <unicode/unistr.h>
  #include <unicode/ucnv.h>
#endif

#include "simdutf.h"

uint64_t nano() {
  return std::chrono::duration_cast<::std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

template <typename PROCEDURE>
double bench(PROCEDURE f, uint64_t threshold = 200'000'000) {
  uint64_t start = nano();
  uint64_t finish = start;
  size_t times{0};
  for (; finish - start < threshold; times++) {
    size_t output_size = f();
    if (output_size == 0) {
      throw std::runtime_error("bad input?");
    }
    finish = nano();
  }
  return double(finish - start) / times;
}

size_t trim_partial_utf8(const uint8_t *buf, size_t len) {
  if (len < 3) {
    switch (len) {
    case 2:
      if (buf[len - 1] >= 0b11000000) {
        return len - 1;
      } // 2-, 3- and 4-byte characters with only 1 byte left
      if (buf[len - 2] >= 0b11100000) {
        return len - 2;
      } // 3- and 4-byte characters with only 2 bytes left
      return len;
    case 1:
      if (buf[len - 1] >= 0b11000000) {
        return len - 1;
      } // 2-, 3- and 4-byte characters with only 1 byte left
      return len;
    case 0:
      return len;
    }
  }
  if (buf[len - 1] >= 0b11000000) {
    return len - 1;
  } // 2-, 3- and 4-byte characters with only 1 byte left
  if (buf[len - 2] >= 0b11100000) {
    return len - 2;
  } // 3- and 4-byte characters with only 1 byte left
  if (buf[len - 3] >= 0b11110000) {
    return len - 3;
  } // 4-byte characters with only 3 bytes left
  return len;
}

size_t trim_partial_utf16(const uint16_t *buf, size_t len) {
  if (len == 0) {
    return 0;
  }
  if ((buf[len - 1] >= 0xD800) && (buf[len - 1] <= 0xDBFF)) {
    return len - 1;
  }
  return len;
}

void run_from_utf8(const std::vector<char> &input_data,
                   size_t approx_output_datapoints = 128) {
  std::vector<char16_t> buffer(
      simdutf::utf16_length_from_utf8(input_data.data(), input_data.size()));
  size_t min_len = 64;
  if (input_data.size() < min_len) {
    return;
  }
#if ICONV_AVAILABLE
  iconv_t cv = iconv_open("UTF-16", "ISO-8859-1");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr,
            "[iconv] cannot initialize ISO-8859-1 to UTF-16 converter\n");
  } else {
    std::cout << "# [iconv] initialized ISO-8859-1 to UTF-16 converter\n";
  }
  std::cout << "n,\tsimdutf-speed\ticonv-speed\n";
#else
  std::cout << "n,\tsimdutf-speed\n";
#endif
  size_t offset = size_t(
      round(double(input_data.size() - min_len) / approx_output_datapoints));
  for (size_t len = min_len; len <= input_data.size();
       len += (len < offset ? offset / 50 + 1 : offset)) {
    size_t effective_length = trim_partial_utf8(
        reinterpret_cast<const uint8_t *>(input_data.data()), len);
    size_t utf8count = simdutf::count_utf8(input_data.data(), effective_length);
    std::cout << utf8count << ",\t";
    auto size_procedure = [effective_length, &input_data, &buffer]() -> size_t {
      return simdutf::convert_utf8_to_utf16le(input_data.data(),
                                              effective_length, buffer.data());
    };

#if ICONV_AVAILABLE
    auto iconv_size_procedure = [&cv, effective_length, &input_data,
                                 &buffer]() -> size_t {
      size_t inbytes = effective_length;
      size_t outbytes = buffer.size();
  #ifdef WINICONV_CONST
      WINICONV_CONST char *inptr =
          reinterpret_cast<WINICONV_CONST char *>(input_data.data());
  #else
      char *inptr = const_cast<char *>(input_data.data());
  #endif
      char *outptr = reinterpret_cast<char *>(buffer.data());
      size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
      (void)result;
      return outbytes;
    };
    std::cout << utf8count / bench(size_procedure) << "\t"
              << utf8count / bench(iconv_size_procedure) << "\n";
#else
    std::cout << utf8count / bench(size_procedure) << "\n";
#endif
  }
}

void run_from_utf16(const std::vector<char> &input_data,
                    size_t approx_output_datapoints = 128) {
  std::vector<char> buffer(simdutf::utf8_length_from_utf16le(
      reinterpret_cast<const char16_t *>(input_data.data()),
      input_data.size() / 2));
  size_t min_len = 64;
  if (input_data.size() < min_len) {
    return;
  }
  std::cout << "n,\tsimdutf-speed\n";
  size_t offset = size_t(
      round(double(input_data.size() - min_len) / approx_output_datapoints));
  for (size_t len = min_len; len <= input_data.size();
       len += (len < offset ? offset / 50 + 1 : offset)) {
    size_t effective_length = trim_partial_utf16(
        reinterpret_cast<const uint16_t *>(input_data.data()), len / 2);
    size_t utf16count = simdutf::count_utf16le(
        reinterpret_cast<const char16_t *>(input_data.data()),
        effective_length);
    std::cout << utf16count << ",\t";
    auto size_procedure = [effective_length, &input_data, &buffer]() -> size_t {
      return simdutf::convert_utf16le_to_utf8(
          reinterpret_cast<const char16_t *>(input_data.data()),
          effective_length, buffer.data());
    };
    std::cout << utf16count / bench(size_procedure) << "\n";
  }
}

int main(int argc, char **argv) {
  printf("# current system detected as %s.\n",
         simdutf::get_active_implementation()->name().c_str());
  if (argc < 2) {
    std::cerr << "Please provide a file argument." << std::endl;
    std::cerr << "The file should contain either UTF-8 or UTF-16 text."
              << std::endl;
    std::cerr << "We then process prefixes of increasing length." << std::endl;
    return EXIT_FAILURE;
  }
  const char *filename = argv[1];
  printf("# loading file %s\n", filename);
  std::filesystem::path path(filename);
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  file.open(path);
  std::vector<char> input_data;
  input_data.assign(std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>());
  auto detected_encoding =
      simdutf::autodetect_encoding(input_data.data(), input_data.size());
  printf("# input detected as %s.\n",
         simdutf::to_string(detected_encoding).c_str());
  if (detected_encoding == simdutf::encoding_type::UTF16_LE) {
    run_from_utf16(input_data);
  } else if (detected_encoding == simdutf::encoding_type::UTF8) {
    run_from_utf8(input_data);
  }
  return EXIT_SUCCESS;
}
