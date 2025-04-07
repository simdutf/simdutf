#include "benchmark.h"
#include "simdutf.h"

#include <cassert>
#include <array>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#ifdef __x86_64__
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
SIMDUTF_TARGET_WESTMERE
namespace {
  #include "benchmarks/competition/utf8lut/src/utf8lut.h"
}
SIMDUTF_UNTARGET_REGION

  /**
   * Bob Steagall, CppCon2018
   * https://github.com/BobSteagall/CppCon2018/
   *
   * Fast Conversion From UTF-8 with C++, DFAs, and SSE Intrinsics
   * https://www.youtube.com/watch?v=5FQ87-Ecb-A
   */
  #include "benchmarks/competition/CppCon2018/utf_utils.cpp"
#endif

/**
 * Bjoern Hoehrmann
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */
#include "benchmarks/competition/hoehrmann/hoehrmann.h"
/**
 * LLVM relies on code from the Unicode Consortium
 * https://en.wikipedia.org/wiki/Unicode_Consortium
 */
#include "benchmarks/competition/llvm/ConvertUTF.cpp"
#ifdef __x86_64__
  /**
   * Olivier Goffart, UTF-8 processing using SIMD (SSE4), 2012.
   * https://woboq.com/blog/utf-8-processing-using-simd.html
   */
  #include "benchmarks/competition/utf8sse4/fromutf8-sse.cpp"
#endif

#ifdef __x86_64__
  /**
   * benchmarks/competition/u8u16 contains an open source version of u8u16,
   * referenced in Cameron, Robert D, A case study in SIMD text processing with
   * parallel bit streams: UTF-8 to UTF-16 transcoding, Proceedings of the 13th
   * ACM SIGPLAN Symposium on Principles and practice of parallel programming,
   * 91--98.
   */
  // It seems that u8u16 is not good at scoping macros.
  #undef LITTLE_ENDIAN
  #undef BYTE_ORDER
  #undef BIG_ENDIAN
  #include "benchmarks/competition/u8u16/config/p4_config.h"
  #include "benchmarks/competition/u8u16/src/libu8u16.c"
#endif

/**
 * Nemanja Trifunovic, UTF8-CPP: UTF-8 with C++ in a Portable Way
 * https://github.com/nemtrif/utfcpp/releases/tag/v3.2.2
 */
#include "benchmarks/competition/utfcpp/source/utf8.h"

namespace simdutf::benchmarks {

template <typename Fn>
void Benchmark::register_function(std::string name, Fn function,
                                  std::set<simdutf::encoding_type> set) {

  if (name.find('+') == std::string::npos) {
    // adding simdutf benchmark, populate for all known architectures
    for (const auto &impl : simdutf::get_available_implementations()) {
      const auto full_name = name + '+' + impl->name();
      benchmarks.insert({full_name, std::make_pair(function, set)});
    }
  } else {
    benchmarks.insert({name, std::make_pair(function, set)});
  }
}

template <typename Fn>
void Benchmark::register_function(std::string name, Fn function,
                                  simdutf::encoding_type enc1) {
  std::set<simdutf::encoding_type> set{enc1};
  register_function(name, function, set);
}

template <typename Fn>
void Benchmark::register_function(std::string name, Fn function,
                                  simdutf::encoding_type enc1,
                                  simdutf::encoding_type enc2) {
  std::set<simdutf::encoding_type> set{enc1, enc2};
  register_function(name, function, set);
}

template <typename Fn>
void Benchmark::register_function(std::string name, Fn function,
                                  simdutf::encoding_type enc1,
                                  simdutf::encoding_type enc2,
                                  simdutf::encoding_type enc3) {
  std::set<simdutf::encoding_type> set{enc1, enc2, enc3};
  register_function(name, function, set);
}

Benchmark::Benchmark(std::vector<input::Testcase> &&testcases)
    : BenchmarkBase(std::move(testcases)) {

  register_function("validate_utf8", &Benchmark::run_validate_utf8,
                    simdutf::encoding_type::UTF8);
  register_function("validate_utf8_with_errors",
                    &Benchmark::run_validate_utf8_with_errors,
                    simdutf::encoding_type::UTF8);
  register_function("validate_utf16", &Benchmark::run_validate_utf16,
                    simdutf::encoding_type::UTF16_LE);
  register_function("validate_utf16_with_errors",
                    &Benchmark::run_validate_utf16_with_errors,
                    simdutf::encoding_type::UTF16_LE);
  register_function("validate_utf32", &Benchmark::run_validate_utf32,
                    simdutf::encoding_type::UTF32_LE);
  register_function("validate_utf32_with_errors",
                    &Benchmark::run_validate_utf32_with_errors,
                    simdutf::encoding_type::UTF32_LE);

  register_function("count_utf8", &Benchmark::run_count_utf8,
                    simdutf::encoding_type::UTF8);
  register_function("count_utf16", &Benchmark::run_count_utf16,
                    simdutf::encoding_type::UTF16_LE);

  register_function("utf8_length_from_latin1",
                    &Benchmark::run_utf8_length_from_latin1,
                    simdutf::encoding_type::Latin1);
  register_function("utf8_length_from_utf16le",
                    &Benchmark::run_utf8_length_from_utf16le,
                    simdutf::encoding_type::UTF16_LE);
  register_function("utf8_length_from_utf16be",
                    &Benchmark::run_utf8_length_from_utf16be,
                    simdutf::encoding_type::UTF16_BE);
  register_function("utf8_length_from_utf32",
                    &Benchmark::run_utf8_length_from_utf32,
                    simdutf::encoding_type::UTF32_LE);
  register_function("utf16_length_from_utf8",
                    &Benchmark::run_utf16_length_from_utf8,
                    simdutf::encoding_type::UTF8);
  register_function("convert_latin1_to_utf8",
                    &Benchmark::run_convert_latin1_to_utf8,
                    simdutf::encoding_type::Latin1);
  register_function("convert_latin1_to_utf16",
                    &Benchmark::run_convert_latin1_to_utf16,
                    simdutf::encoding_type::Latin1);
  register_function("convert_latin1_to_utf32",
                    &Benchmark::run_convert_latin1_to_utf32,
                    simdutf::encoding_type::Latin1);

  register_function("convert_utf8_to_latin1",
                    &Benchmark::run_convert_utf8_to_latin1,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_latin1_with_errors",
                    &Benchmark::run_convert_utf8_to_latin1_with_errors,
                    simdutf::encoding_type::UTF8);
  register_function("convert_valid_utf8_to_latin1",
                    &Benchmark::run_convert_valid_utf8_to_latin1,
                    simdutf::encoding_type::UTF8);

  register_function("convert_utf8_to_utf16",
                    &Benchmark::run_convert_utf8_to_utf16,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf16_with_errors",
                    &Benchmark::run_convert_utf8_to_utf16_with_errors,
                    simdutf::encoding_type::UTF8);
  register_function(
      "convert_utf8_to_utf16_with_dynamic_allocation",
      &Benchmark::run_convert_utf8_to_utf16_with_dynamic_allocation,
      simdutf::encoding_type::UTF8);
  register_function("convert_valid_utf8_to_utf16",
                    &Benchmark::run_convert_valid_utf8_to_utf16,
                    simdutf::encoding_type::UTF8);

  register_function("convert_utf8_to_utf32",
                    &Benchmark::run_convert_utf8_to_utf32,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf32_with_errors",
                    &Benchmark::run_convert_utf8_to_utf32_with_errors,
                    simdutf::encoding_type::UTF8);
  register_function(
      "convert_utf8_to_utf32_with_dynamic_allocation",
      &Benchmark::run_convert_utf8_to_utf32_with_dynamic_allocation,
      simdutf::encoding_type::UTF8);
  register_function("convert_valid_utf8_to_utf32",
                    &Benchmark::run_convert_valid_utf8_to_utf32,
                    simdutf::encoding_type::UTF8);

  register_function("convert_utf16_to_latin1",
                    &Benchmark::run_convert_utf16_to_latin1,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf16_to_latin1_with_errors",
                    &Benchmark::run_convert_utf16_to_latin1_with_errors,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_valid_utf16_to_latin1",
                    &Benchmark::run_convert_valid_utf16_to_latin1,
                    simdutf::encoding_type::UTF16_LE);

  register_function("convert_utf16_to_utf8",
                    &Benchmark::run_convert_utf16_to_utf8,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf16_to_utf8_with_errors",
                    &Benchmark::run_convert_utf16_to_utf8_with_errors,
                    simdutf::encoding_type::UTF16_LE);
  register_function(
      "convert_utf16_to_utf8_with_dynamic_allocation",
      &Benchmark::run_convert_utf16_to_utf8_with_dynamic_allocation,
      simdutf::encoding_type::UTF16_LE);
  register_function("convert_valid_utf16_to_utf8",
                    &Benchmark::run_convert_valid_utf16_to_utf8,
                    simdutf::encoding_type::UTF16_LE);

  register_function("convert_utf16_to_utf32",
                    &Benchmark::run_convert_utf16_to_utf32,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf16_to_utf32_with_errors",
                    &Benchmark::run_convert_utf16_to_utf32_with_errors,
                    simdutf::encoding_type::UTF16_LE);
  register_function(
      "convert_utf16_to_utf32_with_dynamic_allocation",
      &Benchmark::run_convert_utf16_to_utf32_with_dynamic_allocation,
      simdutf::encoding_type::UTF16_LE);
  register_function("convert_valid_utf16_to_utf32",
                    &Benchmark::run_convert_valid_utf16_to_utf32,
                    simdutf::encoding_type::UTF16_LE);

  register_function("convert_utf32_to_latin1",
                    &Benchmark::run_convert_utf32_to_latin1,
                    simdutf::encoding_type::UTF32_LE);
  register_function("convert_utf32_to_latin1_with_errors",
                    &Benchmark::run_convert_utf32_to_latin1_with_errors,
                    simdutf::encoding_type::UTF32_LE);
  register_function("convert_valid_utf32_to_latin1",
                    &Benchmark::run_convert_valid_utf32_to_latin1,
                    simdutf::encoding_type::UTF32_LE);

  register_function("convert_utf32_to_utf8",
                    &Benchmark::run_convert_utf32_to_utf8,
                    simdutf::encoding_type::UTF32_LE);
  register_function("convert_utf32_to_utf8_with_errors",
                    &Benchmark::run_convert_utf32_to_utf8_with_errors,
                    simdutf::encoding_type::UTF32_LE);
  register_function("convert_valid_utf32_to_utf8",
                    &Benchmark::run_convert_valid_utf32_to_utf8,
                    simdutf::encoding_type::UTF32_LE);

  register_function("convert_utf32_to_utf16le",
                    &Benchmark::run_convert_utf32_to_utf16<endianness::LITTLE>,
                    simdutf::encoding_type::UTF32_LE);
  register_function("convert_utf32_to_utf16be",
                    &Benchmark::run_convert_utf32_to_utf16<endianness::BIG>,
                    simdutf::encoding_type::UTF32_LE);
  register_function(
      "convert_utf32_to_utf16le_with_errors",
      &Benchmark::run_convert_utf32_to_utf16_with_errors<endianness::LITTLE>,
      simdutf::encoding_type::UTF32_LE);
  register_function(
      "convert_utf32_to_utf16be_with_errors",
      &Benchmark::run_convert_utf32_to_utf16_with_errors<endianness::BIG>,
      simdutf::encoding_type::UTF32_LE);
  register_function(
      "convert_valid_utf32_to_utf16le",
      &Benchmark::run_convert_valid_utf32_to_utf16<endianness::LITTLE>,
      simdutf::encoding_type::UTF32_LE);
  register_function(
      "convert_valid_utf32_to_utf16be",
      &Benchmark::run_convert_valid_utf32_to_utf16<endianness::BIG>,
      simdutf::encoding_type::UTF32_LE);

  register_function("detect_encodings", &Benchmark::run_detect_encodings,
                    simdutf::encoding_type::UTF8,
                    simdutf::encoding_type::UTF16_LE,
                    simdutf::encoding_type::UTF32_LE);

#ifdef ICU_AVAILABLE
  register_function("convert_latin1_to_utf8+icu",
                    &Benchmark::run_convert_latin1_to_utf8_icu,
                    simdutf::encoding_type::Latin1);
  register_function("convert_latin1_to_utf16+icu",
                    &Benchmark::run_convert_latin1_to_utf16_icu,
                    simdutf::encoding_type::Latin1);
  register_function("convert_latin1_to_utf32+icu",
                    &Benchmark::run_convert_latin1_to_utf32_icu,
                    simdutf::encoding_type::Latin1);
  register_function("convert_utf8_to_latin1+icu",
                    &Benchmark::run_convert_utf8_to_latin1_icu,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf16+icu",
                    &Benchmark::run_convert_utf8_to_utf16_icu,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf16_to_utf8+icu",
                    &Benchmark::run_convert_utf16_to_utf8_icu,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf16_to_latin1+icu",
                    &Benchmark::run_convert_utf16_to_latin1_icu,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf32_to_latin1+icu",
                    &Benchmark::run_convert_utf32_to_latin1_icu,
                    simdutf::encoding_type::UTF32_LE);
#endif
#ifdef ICONV_AVAILABLE
  register_function("convert_latin1_to_utf8+iconv",
                    &Benchmark::run_convert_latin1_to_utf8_iconv,
                    simdutf::encoding_type::Latin1);
  register_function("convert_latin1_to_utf16+iconv",
                    &Benchmark::run_convert_latin1_to_utf16_iconv,
                    simdutf::encoding_type::Latin1);
  register_function("convert_latin1_to_utf32+iconv",
                    &Benchmark::run_convert_latin1_to_utf32_iconv,
                    simdutf::encoding_type::Latin1);
  register_function("convert_utf8_to_latin1+iconv",
                    &Benchmark::run_convert_utf8_to_latin1_iconv,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf16+iconv",
                    &Benchmark::run_convert_utf8_to_utf16_iconv,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf16_to_utf8+iconv",
                    &Benchmark::run_convert_utf16_to_utf8_iconv,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf16_to_latin1+iconv",
                    &Benchmark::run_convert_utf16_to_latin1_iconv,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf32_to_latin1+iconv",
                    &Benchmark::run_convert_utf32_to_latin1_iconv,
                    simdutf::encoding_type::UTF32_LE);
#endif
#ifdef INOUE2008
  register_function("convert_valid_utf8_to_utf16+inoue2008",
                    &Benchmark::run_convert_valid_utf8_to_utf16_inoue2008,
                    simdutf::encoding_type::UTF8);
#endif
#ifdef __x86_64__
  register_function("convert_utf8_to_utf16+u8u16",
                    &Benchmark::run_convert_utf8_to_utf16_u8u16,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf16_to_utf8+utf8lut",
                    &Benchmark::run_convert_valid_utf8_to_utf16_utf8lut,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_valid_utf16_to_utf8+utf8lut",
                    &Benchmark::run_convert_valid_utf16_to_utf8_utf8lut,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf8_to_utf16+utf8lut",
                    &Benchmark::run_convert_valid_utf8_to_utf16_utf8lut,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf32+utf8lut",
                    &Benchmark::run_convert_utf8_to_utf32_utf8lut,
                    simdutf::encoding_type::UTF8);
  register_function("convert_valid_utf8_to_utf16+utf8lut",
                    &Benchmark::run_convert_valid_utf8_to_utf16_utf8lut,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf32_to_utf8+utf8lut",
                    &Benchmark::run_convert_valid_utf32_to_utf8_utf8lut,
                    simdutf::encoding_type::UTF32_LE);
  register_function("convert_valid_utf32_to_utf8+utf8lut",
                    &Benchmark::run_convert_valid_utf32_to_utf8_utf8lut,
                    simdutf::encoding_type::UTF32_BE);
  register_function("convert_valid_utf8_to_utf32+utf8lut",
                    &Benchmark::run_convert_utf8_to_utf32_utf8lut,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf16+utf8sse4",
                    &Benchmark::run_convert_utf8_to_utf16_utf8sse4,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf16+cppcon2018",
                    &Benchmark::run_convert_utf8_to_utf16_cppcon2018,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf32+cppcon2018",
                    &Benchmark::run_convert_utf8_to_utf32_cppcon2018,
                    simdutf::encoding_type::UTF8);
#endif
  register_function("convert_utf8_to_utf16+hoehrmann",
                    &Benchmark::run_convert_utf8_to_utf16_hoehrmann,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf32+hoehrmann",
                    &Benchmark::run_convert_utf8_to_utf32_hoehrmann,
                    simdutf::encoding_type::UTF8);

  register_function("convert_utf8_to_utf16+llvm",
                    &Benchmark::run_convert_utf8_to_utf16_llvm,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf32+llvm",
                    &Benchmark::run_convert_utf8_to_utf32_llvm,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf16_to_utf8+llvm",
                    &Benchmark::run_convert_utf16_to_utf8_llvm,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf32_to_utf8+llvm",
                    &Benchmark::run_convert_utf32_to_utf8_llvm,
                    simdutf::encoding_type::UTF32_LE);
  register_function("convert_utf32_to_utf16+llvm",
                    &Benchmark::run_convert_utf32_to_utf16_llvm,
                    simdutf::encoding_type::UTF32_LE);
  register_function("convert_utf16_to_utf32+llvm",
                    &Benchmark::run_convert_utf16_to_utf32_llvm,
                    simdutf::encoding_type::UTF16_LE);

  register_function("convert_utf8_to_utf16+utfcpp",
                    &Benchmark::run_convert_utf8_to_utf16_utfcpp,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf8_to_utf32+utfcpp",
                    &Benchmark::run_convert_utf8_to_utf32_utfcpp,
                    simdutf::encoding_type::UTF8);
  register_function("convert_utf16_to_utf8+utfcpp",
                    &Benchmark::run_convert_utf16_to_utf8_utfcpp,
                    simdutf::encoding_type::UTF16_LE);
  register_function("convert_utf32_to_utf8+utfcpp",
                    &Benchmark::run_convert_utf32_to_utf8_utfcpp,
                    simdutf::encoding_type::UTF32_LE);

  register_function("utf8_length_from_latin1+node",
                    &Benchmark::run_utf8_length_from_latin1_node,
                    simdutf::encoding_type::Latin1);
}

// static
Benchmark Benchmark::create(const CommandLine &cmdline) {
  std::vector<input::Testcase> testcases;

  using input::File;
  using input::random_utf8;
  using input::Testcase;

  for (const size_t iterations : cmdline.iterations) {
    for (const auto &path : cmdline.files) {
      testcases.emplace_back(
          Testcase{cmdline.procedures, iterations, File{path}});
    }

    for (const size_t size : cmdline.random_size) {
      testcases.emplace_back(
          Testcase{cmdline.procedures, iterations, random_utf8{size}});
    }
  }

  return Benchmark{std::move(testcases)};
}

void Benchmark::list_procedures(ListingMode lm) const {
  switch (lm) {
  case ListingMode::None:
    break;

  case ListingMode::HumanReadable: {
    const auto &known_procedures = all_procedures();
    printf("Available procedures (%zu)\n", size_t(known_procedures.size()));
    for (const auto &name : known_procedures) {
      printf("- %s\n", name.c_str());
    }
  } break;

  case ListingMode::PlainLines: {
    const auto &known_procedures = all_procedures();
    for (const auto &name : known_procedures) {
      puts(name.c_str());
    }
    break;
  }

  case ListingMode::Json: {
    printf("[\n");
    auto first = true;
    for (const auto &item : benchmarks) {
      const auto &name = item.first;
      const auto &entry = item.second;
      if (!first) {
        putchar(',');
      }
      first = false;

      printf(" {\n");
      printf("    \"name\": \"%s\",\n", name.c_str());
      if (std::holds_alternative<thirdparty_fn>(entry.first)) {
        printf("    \"simdutf\": false,\n");
      } else if (std::holds_alternative<simdutf_fn>(entry.first)) {
        printf("    \"simdutf\": true,\n");
      }

      {
        printf("    \"encodings\": [");
        bool first = true;
        for (const auto &enc : entry.second) {
          if (!first) {
            putchar(',');
          }
          first = false;

          switch (enc) {
          case simdutf::UTF8:
            printf("\"utf8\"");
            break;
          case simdutf::UTF16_LE:
            printf("\"utf16le\"");
            break;
          case simdutf::UTF16_BE:
            printf("\"utf16be\"");
            break;
          case simdutf::UTF32_LE:
            printf("\"utf32le\"");
            break;
          case simdutf::UTF32_BE:
            printf("\"utf32be\"");
            break;
          case simdutf::Latin1:
            printf("\"latin1\"");
            break;
          default:
            printf("\"unknown\"");
            break;
          }
        }
        printf("]\n");
      } // encodings
      printf(" }");
    } // for
    printf("]\n");
    break;
  }
  }
}

void Benchmark::run(const std::string &procedure_name, size_t iterations) {
  const auto item = benchmarks.find(procedure_name);
  if (item == benchmarks.end()) {
    std::cerr << "Unsupported procedure: " << procedure_name << '\n';
    std::cerr << "Report the issue.\n";
    std::cerr << " Aborting ! " << '\n';
    exit(1);
  }

#ifdef ICU_AVAILABLE
  if (icu_show_version) {
    std::cout << "Using ICU version " << U_ICU_VERSION << std::endl;
    icu_show_version = false;
  }
#endif
#ifdef _LIBICONV_VERSION
  if (iconv_show_version) {
    std::cout << "Using iconv version " << _LIBICONV_VERSION << std::endl;
    iconv_show_version = false;
  }
#endif

  const auto &entry = item->second;
  if (std::holds_alternative<thirdparty_fn>(entry.first)) {
    const auto fn = std::get<thirdparty_fn>(entry.first);

    (this->*fn)(iterations);
  } else if (std::holds_alternative<simdutf_fn>(entry.first)) {
    const auto p = procedure_name.find('+');
    const std::string name{procedure_name.substr(0, p)};
    const std::string impl{procedure_name.substr(p + 1)};

    auto implementation = simdutf::get_available_implementations()[impl];
    if (implementation == nullptr) {
      throw std::runtime_error("Wrong implementation " + impl);
    }
    // If you want to skip the CPU feature checks, you can set
    // a variable when calling the benchmark program. E.g.,
    // SIMDUTF_SKIP_CPU_CHECK=ON benchmark -F myfile.txt
    // This might result in a crash (E.g., Illegal instruction).
    SIMDUTF_PUSH_DISABLE_WARNINGS
    SIMDUTF_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC:
                                       // manually verified this is safe
        static const char *skip_check = getenv("SIMDUTF_SKIP_CPU_CHECK");
    SIMDUTF_POP_DISABLE_WARNINGS
    if (!skip_check && !implementation->supported_by_runtime_system()) {
      std::cout << procedure_name << ": unsupported by the system\n";
      return;
    }

    const auto fn = std::get<simdutf_fn>(entry.first);
    (this->*fn)(*implementation, iterations);
  } else {
    throw std::logic_error("The entry for '" + procedure_name +
                           "' is not valid. Please report an issue.");
  }

  // We pause after each call to make sure
  // that other benchmarks are not affected by frequency throttling.
  // This was initially introduced for AVX-512 only, but it is probably
  // wise to have it always.
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Benchmark::run_validate_utf8(const simdutf::implementation &implementation,
                                  size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile bool sink{false};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.validate_utf8(data, size);
  };

  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_validate_utf8_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile bool sink{false};

  auto proc = [&implementation, data, size, &sink]() {
    result res = implementation.validate_utf8_with_errors(data, size);
    sink = !(res.error);
  };

  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_validate_utf16(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  volatile bool sink{false};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.validate_utf16le(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_validate_utf16_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  volatile bool sink{false};

  auto proc = [&implementation, data, size, &sink]() {
    result res = implementation.validate_utf16le_with_errors(data, size);
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_validate_utf32(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  volatile bool sink{false};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.validate_utf32(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_validate_utf32_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  volatile bool sink{false};

  auto proc = [&implementation, data, size, &sink]() {
    result res = implementation.validate_utf32_with_errors(data, size);
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_latin1_to_utf8(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char[]> output_buffer{new char[size * 2]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink =
        implementation.convert_latin1_to_utf8(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_latin1_to_utf16(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_latin1_to_utf16le(data, size,
                                                    output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_latin1_to_utf32(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink =
        implementation.convert_latin1_to_utf32(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, size, char_count);
}

void Benchmark::run_utf8_length_from_latin1(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.utf8_length_from_latin1(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_utf8_length_from_utf16le(
    const simdutf::implementation &implementation, size_t iterations) {
  const char16_t *data = reinterpret_cast<const char16_t *>(input_data.data());
  const size_t size = input_data.size() / 2;
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.utf8_length_from_utf16le(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  print_summary(result, size, size);
}

void Benchmark::run_utf8_length_from_utf16be(
    const simdutf::implementation &implementation, size_t iterations) {
  const char16_t *data = reinterpret_cast<const char16_t *>(input_data.data());
  const size_t size = input_data.size() / 2;
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.utf8_length_from_utf16be(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  print_summary(result, size, size);
}

void Benchmark::run_utf8_length_from_utf32(
    const simdutf::implementation &implementation, size_t iterations) {
  const char32_t *data = reinterpret_cast<const char32_t *>(input_data.data());
  const size_t size = input_data.size() / 4;
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.utf8_length_from_utf32(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  print_summary(result, size, size);
}

void Benchmark::run_utf16_length_from_utf8(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size() / 4;
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.utf16_length_from_utf8(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  print_summary(result, size, size);
}

static inline uint32_t portable_popcount(uint64_t v) {
#ifdef __GNUC__
  return static_cast<uint32_t>(__builtin_popcountll(v));
#elif defined(_WIN64) && defined(_MSC_VER) && _MSC_VER >= 1400 &&              \
    !defined(_M_ARM64)
  return static_cast<uint32_t>(__popcnt64(static_cast<__int64>(v)));
#else
  v = v - ((v >> 1) & 0x5555555555555555);
  v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
  v = ((v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F);
  return static_cast<uint32_t>((v * (0x0101010101010101)) >> 56);
#endif
}

void Benchmark::run_utf8_length_from_latin1_node(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  auto proc = [data, size, &sink]() {
    // from https://github.com/nodejs/node/pull/54345
    uint32_t length = size;
    uint32_t result = length;
    uint32_t i = 0;
    const auto length8 = length & ~0x7;
    while (i < length8) {
      // Original PR used std::popcount, but it is not available pre-C++20.
      result += portable_popcount(
          *reinterpret_cast<const uint64_t *>(data + i) & 0x8080808080808080);
      i += 8;
    }
    while (i < length) {
      result += (data[i] >> 7);
      i++;
    }
    sink = result;
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_latin1(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink =
        implementation.convert_utf8_to_latin1(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_latin1_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile bool sink{false};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    result res = implementation.convert_utf8_to_latin1_with_errors(
        data, size, output_buffer.get());
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_valid_utf8_to_latin1(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_valid_utf8_to_latin1(data, size,
                                                       output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf16(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink =
        implementation.convert_utf8_to_utf16le(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf16_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile bool sink{false};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    result res = implementation.convert_utf8_to_utf16le_with_errors(
        data, size, output_buffer.get());
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf32(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink =
        implementation.convert_utf8_to_utf32(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf32_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  volatile bool sink{false};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    result res = implementation.convert_utf8_to_utf32_with_errors(
        data, size, output_buffer.get());
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf16_with_dynamic_allocation(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &sink]() {
    auto dyn_size = implementation.utf16_length_from_utf8(data, size);
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[dyn_size]};
    sink =
        implementation.convert_utf8_to_utf16le(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf32_with_dynamic_allocation(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &sink]() {
    auto dyn_size = implementation.utf32_length_from_utf8(data, size);
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[dyn_size]};
    sink =
        implementation.convert_utf8_to_utf32(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

#ifdef ICU_AVAILABLE

void Benchmark::run_convert_latin1_to_utf8_icu(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  // Allocate target buffer
  int32_t targetCapacity = size * 2;
  std::unique_ptr<char[]> target(new char[targetCapacity]);

  auto proc = [data, size, &sink, &target, targetCapacity]() {
    UErrorCode status = U_ZERO_ERROR;

    // Open converters for source and target encodings
    UConverter *latin1conv = ucnv_open("ISO-8859-1", &status);
    assert(U_SUCCESS(status));
    UConverter *utf8conv = ucnv_open("UTF-8", &status);
    assert(U_SUCCESS(status));

    // Pointers for source and target
    const char *source = data;
    const char *sourceLimit = data + size;
    char *targetStart = target.get();
    char *targetLimit = target.get() + targetCapacity;

    // Convert from ISO-8859-1 to UTF-8
    ucnv_convertEx(utf8conv, latin1conv, &targetStart, targetLimit, &source,
                   sourceLimit, nullptr, nullptr, nullptr, nullptr, true, true,
                   &status);
    assert(U_SUCCESS(status));

    // Calculate the output size
    sink = targetStart - target.get();

    // Clean up
    ucnv_close(utf8conv);
    ucnv_close(latin1conv);
  };

  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = size;
  std::unique_ptr<char[]> output_buffer{new char[size * 2]};
  size_t expected = get_active_implementation()->convert_latin1_to_utf8(
      data, size, output_buffer.get());
  if (expected != sink) {
    std::cerr << "The number of characters outputted does not match.\n";
    std::cout << "Expected: " << expected << ", Sink: " << sink
              << std::endl; // print values
  }

  if (memcmp(target.get(), output_buffer.get(), sink) != 0) {
    std::cerr << "The output data does not match.\n";
  }

  print_summary(result, size, char_count);
}

void Benchmark::run_convert_latin1_to_utf16_icu(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  // Allocate target buffer outside lambda
  std::unique_ptr<UChar[]> target(new UChar[size * 2]);

  auto proc = [data, size, &sink, &target]() {
    UErrorCode status = U_ZERO_ERROR;

    // Open converter for source encoding
    UConverter *latin1conv = ucnv_open("ISO-8859-1", &status);
    assert(U_SUCCESS(status));

    // Convert from ISO-8859-1 to UTF-16 directly
    int32_t actualTargetSize =
        ucnv_toUChars(latin1conv, target.get(), size * 2, data, size, &status);
    assert(U_SUCCESS(status));

    // Calculate the output size in bytes
    sink = actualTargetSize * sizeof(UChar);

    // Clean up
    ucnv_close(latin1conv);
  };

  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = size;
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  size_t expected = get_active_implementation()->convert_latin1_to_utf16le(
      data, size, output_buffer.get()); // expected char16_t units
  if (2 * expected != sink) {
    std::cerr << "The number of utf16le code units does not match.\n";
    std::cerr << "Expected: " << 2 * expected + 1 << ", Sink: " << sink
              << std::endl; // print values
  }

  if (memcmp(target.get(), output_buffer.get(), sink) != 0) {
    std::cerr << "The output data does not match.\n";
    // compare first 20 characters and print their hexadecimal values
    std::cout << "First 20 characters of target data: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(target.get()[i]) << " ";
    }
    std::cout << "\nFirst 20 characters of output buffer: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(output_buffer[i]) << " ";
    }

    // compare last 20 characters and print their hexadecimal values
    size_t num_chars = sink / sizeof(UChar);
    size_t start = num_chars < 20 ? 0 : num_chars - 20;
    std::cout << "\nLast 20 characters of target data: ";
    for (size_t i = start; i < num_chars; i++) {
      std::cout << std::hex << static_cast<int>(target.get()[i]) << " ";
    }
    std::cout << "\nLast 20 characters of output buffer: ";
    for (size_t i = start; i < num_chars; i++) {
      std::cout << std::hex << static_cast<int>(output_buffer[i]) << " ";
    }
  }

  print_summary(result, size, char_count);
}

void Benchmark::run_convert_latin1_to_utf32_icu(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  std::unique_ptr<char[]> target;

  auto proc = [&target, data, size, &sink]() {
    UErrorCode status = U_ZERO_ERROR;

    // Open converters for source and target encodings
    UConverter *latin1conv = ucnv_open("ISO-8859-1", &status);
    assert(U_SUCCESS(status));
    UConverter *utf32conv = ucnv_open("UTF-32LE", &status);
    assert(U_SUCCESS(status));

    // Allocate target buffer
    int32_t targetCapacity = size * 4; // UTF-32 takes four bytes.
    target.reset(new char[targetCapacity]);

    // Pointers for source and target
    const char *source = data;
    const char *sourceLimit = data + size;
    char *targetStart = target.get();
    char *targetLimit = target.get() + targetCapacity;

    // Convert from ISO-8859-1 to UTF-32
    ucnv_convertEx(utf32conv, latin1conv, &targetStart, targetLimit, &source,
                   sourceLimit, nullptr, nullptr, nullptr, nullptr, true, true,
                   &status);
    assert(U_SUCCESS(status));

    // Calculate the output size in bytes
    sink = targetStart - target.get();

    // Clean up
    ucnv_close(utf32conv);
    ucnv_close(latin1conv);
  };

  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = size;
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  size_t expected = get_active_implementation()->convert_latin1_to_utf32(
      data, size, output_buffer.get()); // expected is the # of UTF32 characters
  if (4 * expected != sink) {
    std::cerr
        << "The number of characters outputted does not match.\n"; // each UTF32
                                                                   // character
                                                                   // takes four
                                                                   // bytes
    std::cout << "Expected: " << expected << ", Sink: " << sink
              << std::endl; // print values
  }

  if (memcmp(target.get(), output_buffer.get(), sink) != 0) {
    std::cerr << "The output data does not match.\n";
    // compare first 20 characters and print their hexadecimal values
    std::cout << "First 20 characters of target data: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(target.get()[i]) << " ";
    }
    std::cout << "\nFirst 20 characters of output buffer: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(output_buffer[i]) << " ";
    }

    // compare last 20 characters and print their hexadecimal values
    size_t num_chars = sink / sizeof(UChar);
    size_t start = num_chars < 20 ? 0 : num_chars - 20;
    std::cout << "\nLast 20 characters of target data: ";
    for (size_t i = start; i < num_chars; i++) {
      std::cout << std::hex << static_cast<int>(target.get()[i]) << " ";
    }
    std::cout << "\nLast 20 characters of output buffer: ";
    for (size_t i = start; i < num_chars; i++) {
      std::cout << std::hex << static_cast<int>(output_buffer[i]) << " ";
    }
  }

  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_latin1_icu(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  std::unique_ptr<char[]> target;

  auto proc = [&target, data, size, &sink]() {
    UErrorCode status = U_ZERO_ERROR;

    // Open converters for source and target encodings
    UConverter *utf8conv = ucnv_open("UTF-8", &status);
    assert(U_SUCCESS(status));
    UConverter *latin1conv = ucnv_open("ISO-8859-1", &status);
    assert(U_SUCCESS(status));

    // Allocate target buffer
    int32_t targetCapacity = size * 2;
    target.reset(new char[targetCapacity]);

    // Pointers for source and target
    const char *source = data;
    const char *sourceLimit = data + size;
    char *targetStart = target.get();
    char *targetLimit = target.get() + targetCapacity;

    // Convert from ISO-8859-1 to UTF-8
    ucnv_convertEx(latin1conv, utf8conv, &targetStart, targetLimit, &source,
                   sourceLimit, nullptr, nullptr, nullptr, nullptr, true, true,
                   &status);
    assert(U_SUCCESS(status));

    // Calculate the output size
    sink = targetStart - target.get();

    // Clean up
    ucnv_close(utf8conv);
    ucnv_close(latin1conv);
  };

  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  std::unique_ptr<char[]> output_buffer{new char[size]};
  size_t expected = get_active_implementation()->convert_utf8_to_latin1(
      data, size, output_buffer.get());
  if (expected != sink) {
    std::cerr << "The number of latin1 code units does not match.\n";
  }

  if (memcmp(target.get(), output_buffer.get(), sink) != 0) {
    std::cerr << "The output data does not match.\n";
    // compare first 20 characters and print their hexadecimal values
    std::cout << "First 20 characters of target data: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(target.get()[i]) << " ";
    }
    std::cout << "\nFirst 20 characters of output buffer: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(output_buffer[i]) << " ";
    }
  }

  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf16_icu(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};
  auto proc = [data, size, &sink]() {
    auto str =
        U_ICU_NAMESPACE::UnicodeString::fromUTF8(std::string_view(data, size));
    sink = str.length();
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  // checking
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  size_t expected = convert_utf8_to_utf16le(data, size, output_buffer.get());
  if (expected != sink) {
    std::cerr << "The number of UTF-16 code units does not match.\n";
  }
  print_summary(result, size, char_count);
}
void Benchmark::run_convert_utf16_to_utf8_icu(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }
  size /= 2;
  volatile size_t sink{0};

  auto proc = [data, size, &sink]() {
    U_ICU_NAMESPACE::UnicodeString str(data, size);
    std::string out;
    out = str.toUTF8String(out);
    sink = out.size();
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_latin1_icu(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }
  size /= 2;
  volatile size_t sink{0};

  std::unique_ptr<char[]> target;

  auto proc = [&target, data, size, &sink]() {
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv =
        ucnv_open("ISO-8859-1", &status); // open a converter for ISO-8859-1
    assert(U_SUCCESS(status));

    int32_t targetCapacity = size; // adjust as needed
    target.reset(new char[targetCapacity]);
    char *targetStart = target.get();

    sink =
        ucnv_fromUChars(conv, targetStart, targetCapacity,
                        reinterpret_cast<const UChar *>(data), size, &status);
    assert(U_SUCCESS(status));

    // Clean up
    ucnv_close(conv);
  };

  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  std::unique_ptr<char[]> output_buffer{new char[size]};
  size_t expected = get_active_implementation()->convert_utf16le_to_latin1(
      data, size, output_buffer.get());
  if (expected != sink) {
    std::cerr << "The number of expected bytes does not match.\n";
    std::cout << "Expected: " << expected << ", Sink: " << sink
              << std::endl; // print values
  }

  if (memcmp(target.get(), output_buffer.get(), sink) != 0) {
    std::cerr << "The output data does not match.\n";
    // compare first 20 characters and print their hexadecimal values
    std::cout << "First 20 characters of target data: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(target.get()[i]) << " ";
    }
    std::cout << "\nFirst 20 characters of output buffer: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(output_buffer[i]) << " ";
    }
  }

  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_latin1_icu(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;
  volatile size_t sink{0};
  std::unique_ptr<char[]> target;

  auto proc = [&target, data, size, &sink]() {
    UErrorCode status = U_ZERO_ERROR;

    UConverter *utf32conv =
        ucnv_open("UTF-32LE", &status); // create a UTF-32 converter
    assert(U_SUCCESS(status));

    UConverter *latin1conv =
        ucnv_open("ISO-8859-1", &status); // create a Latin1 converter
    assert(U_SUCCESS(status));

    int32_t targetCapacity = size; // adjust as needed
    target.reset(new char[targetCapacity]);
    char *targetStart = target.get();

    const char *sourceStart = reinterpret_cast<const char *>(data);
    const char *sourceEnd = sourceStart + size * sizeof(char32_t);

    // Convert from UTF-32 to Latin1
    ucnv_convertEx(latin1conv, utf32conv, &targetStart,
                   targetStart + targetCapacity, &sourceStart, sourceEnd,
                   nullptr, nullptr, nullptr, nullptr, true, true, &status);
    assert(U_SUCCESS(status));

    // Calculate the output size
    sink = targetStart - target.get();

    // Clean up
    ucnv_close(utf32conv);
    ucnv_close(latin1conv);
  };

  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = size;
  std::unique_ptr<char[]> output_buffer{new char[size]};
  size_t expected = get_active_implementation()->convert_utf32_to_latin1(
      data, size, output_buffer.get());
  if (expected != sink) {
    std::cerr << "The number of expected bytes does not match.\n";
    std::cout << "Expected: " << expected << ", Sink: " << sink
              << std::endl; // print values
  }

  if (memcmp(target.get(), output_buffer.get(), sink) != 0) {
    std::cerr << "The output data does not match.\n";
    // compare first 20 characters and print their hexadecimal values
    std::cout << "First 20 characters of target data: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(target.get()[i]) << " ";
    }
    std::cout << "\nFirst 20 characters of output buffer: ";
    for (size_t i = 0; i < 20; i++) {
      std::cout << std::hex << static_cast<int>(output_buffer[i]) << " ";
    }
  }

  print_summary(result, input_data.size(), char_count);
}

#endif

#ifdef ICONV_AVAILABLE
void Benchmark::run_convert_latin1_to_utf8_iconv(size_t iterations) {
  iconv_t cv = iconv_open("UTF-8", "ISO-8859-1");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr,
            "[iconv] cannot initialize ISO-8859-1 to UTF-8 converter\n");
    return;
  }
  char *data = reinterpret_cast<char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char[]> output_buffer{new char[size * 2]}; // 2 for safety
  volatile size_t sink{0};
  auto proc = [&cv, data, size, &output_buffer, &sink]() {
    size_t inbytes = size;
    size_t outbytes = sizeof(uint8_t) * size * 2;
  #ifdef WINICONV_CONST
    WINICONV_CONST char *inptr = reinterpret_cast<WINICONV_CONST char *>(data);
  #else
    char *inptr = data;
  #endif
    char *outptr = reinterpret_cast<char *>(output_buffer.get());
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      sink = 0;
    } else {
      sink = (sizeof(uint8_t) * size - outbytes) / sizeof(char);
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  iconv_close(cv);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_latin1_to_utf16_iconv(size_t iterations) {
  iconv_t cv = iconv_open("UTF-16", "ISO-8859-1");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr,
            "[iconv] cannot initialize ISO-8859-1 to UTF-16 converter\n");
    return;
  }
  char *data = reinterpret_cast<char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};
  auto proc = [&cv, data, size, &output_buffer, &sink]() {
    size_t inbytes = size;
    size_t outbytes = sizeof(uint16_t) * size;
  #ifdef WINICONV_CONST
    WINICONV_CONST char *inptr = reinterpret_cast<WINICONV_CONST char *>(data);
  #else
    char *inptr = data;
  #endif
    char *outptr = reinterpret_cast<char *>(output_buffer.get());
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      sink = 0;
    } else {
      sink = (sizeof(uint16_t) * size - outbytes) / sizeof(char);
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  iconv_close(cv);
  size_t char_count = size;
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_latin1_to_utf32_iconv(size_t iterations) {
  iconv_t cv = iconv_open("UTF-32LE", "ISO-8859-1");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr,
            "[iconv] cannot initialize ISO-8859-1 to UTF-32 converter\n");
    return;
  }
  char *data = reinterpret_cast<char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  volatile size_t sink{0};
  auto proc = [&cv, data, size, &output_buffer, &sink]() {
    size_t inbytes = size;
    size_t outbytes = sizeof(uint32_t) * size;
  #ifdef WINICONV_CONST
    WINICONV_CONST char *inptr = reinterpret_cast<WINICONV_CONST char *>(data);
  #else
    char *inptr = data;
  #endif
    char *outptr = reinterpret_cast<char *>(output_buffer.get());
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      sink = 0;
    } else {
      sink = (sizeof(uint32_t) * size - outbytes) / sizeof(char);
      ;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  iconv_close(cv);
  size_t char_count = size;
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_latin1_iconv(size_t iterations) {
  iconv_t cv = iconv_open("ISO-8859-1", "UTF-8");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr, "[iconv] cannot initialize UTF-8 to Latin1 converter\n");
    return;
  }
  char *data = reinterpret_cast<char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile size_t sink{0};

  auto proc = [&cv, data, size, &output_buffer, &sink]() {
    size_t inbytes = size;
    size_t outbytes = sizeof(uint8_t) * size;
    // win-iconv includes WINICONV_CONST in its function signatures
    // https://github.com/simdutf/simdutf/pull/178
  #ifdef WINICONV_CONST
    WINICONV_CONST char *inptr = reinterpret_cast<WINICONV_CONST char *>(data);
  #else
    char *inptr = data;
  #endif
    char *outptr = reinterpret_cast<char *>(output_buffer.get());
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      sink = 0;
    } else {
      sink = (sizeof(uint8_t) * size - outbytes) / sizeof(char);
      ;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  iconv_close(cv);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf16_iconv(size_t iterations) {
  iconv_t cv = iconv_open("UTF-16LE", "UTF-8");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr, "[iconv] cannot initialize UTF-8 to UTF-16LE converter\n");
    return;
  }
  char *data = reinterpret_cast<char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};

  auto proc = [&cv, data, size, &output_buffer, &sink]() {
    size_t inbytes = size;
    size_t outbytes = sizeof(uint16_t) * size;
    // win-iconv includes WINICONV_CONST in its function signatures
    // https://github.com/simdutf/simdutf/pull/178
  #ifdef WINICONV_CONST
    WINICONV_CONST char *inptr = reinterpret_cast<WINICONV_CONST char *>(data);
  #else
    char *inptr = data;
  #endif
    char *outptr = reinterpret_cast<char *>(output_buffer.get());
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      sink = 0;
    } else {
      sink = (sizeof(uint16_t) * size - outbytes) / sizeof(char);
      ;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  iconv_close(cv);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf16_to_latin1_iconv(size_t iterations) {
  iconv_t cv = iconv_open("ISO-8859-1", "UTF-16LE");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr,
            "[iconv] cannot initialize the UTF-16LE to ISO-8859-1 converter\n");
    return;
  }
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  char16_t *data =
      reinterpret_cast<char16_t *>(input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  std::unique_ptr<char[]> output_buffer{new char[size]};

  volatile size_t sink{0};

  auto proc = [cv, data, size, &output_buffer, &sink]() {
    size_t inbytes = sizeof(uint16_t) * size;
    size_t outbytes = size;
  #ifdef WINICONV_CONST
    WINICONV_CONST char *inptr = reinterpret_cast<WINICONV_CONST char *>(data);
  #else
    char *inptr = reinterpret_cast<char *>(data);
  #endif
    char *outptr = output_buffer.get();
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      sink = 0;
    } else {
      sink = (size - outbytes) / sizeof(char16_t);
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  iconv_close(cv);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf8_iconv(size_t iterations) {
  iconv_t cv = iconv_open("UTF-8", "UTF-16LE");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr,
            "[iconv] cannot initialize the UTF-16LE to UTF-8 converter\n");
    return;
  }
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  char16_t *data =
      reinterpret_cast<char16_t *>(input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile size_t sink{0};

  auto proc = [cv, data, size, &output_buffer, &sink]() {
    size_t inbytes = sizeof(uint16_t) * size;
    size_t outbytes = 4 * size;
  #ifdef WINICONV_CONST
    WINICONV_CONST char *inptr = reinterpret_cast<WINICONV_CONST char *>(data);
  #else
    char *inptr = reinterpret_cast<char *>(data);
  #endif
    char *outptr = output_buffer.get();
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      sink = 0;
    } else {
      sink = (4 * size - outbytes) / sizeof(char16_t);
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  iconv_close(cv);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_latin1_iconv(size_t iterations) {
  iconv_t cv = iconv_open("ISO-8859-1", "UTF-32LE");
  if (cv == (iconv_t)(-1)) {
    fprintf(stderr,
            "[iconv] cannot initialize the UTF-32 to ISO-8859-1 converter\n");
    return;
  }
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  char32_t *data =
      reinterpret_cast<char32_t *>(input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  std::unique_ptr<char[]> output_buffer{new char[size]};

  volatile size_t sink{0};

  auto proc = [cv, data, size, &output_buffer, &sink]() {
    size_t inbytes = sizeof(uint32_t) * size;
    size_t outbytes = size;
  #ifdef WINICONV_CONST
    WINICONV_CONST char *inptr = reinterpret_cast<WINICONV_CONST char *>(data);
  #else
    char *inptr = reinterpret_cast<char *>(data);
  #endif
    char *outptr = output_buffer.get();
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      sink = 0;
      abort();
    } else {
      sink = (size - outbytes) / sizeof(char32_t);
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  iconv_close(cv);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}
#endif

#ifdef INOUE2008
void Benchmark::run_convert_valid_utf8_to_utf16_inoue2008(size_t iterations) {
  // Inoue2008 is only up to 3-byte UTF8 sequence.
  for (uint8_t c : input_data) {
    if (c >= 0b11110000) {
      std::cerr << "Warning: Inoue 2008 does not support 4-byte inputs!"
                << std::endl;
      break;
    }
  }
  // This is currently minimally tested. It is possible that the transcoding
  // could be wrong. It is also unsafe: it could fail in disastrous ways if the
  // input is adversarial.
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    sink = inoue2008::convert_valid(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}
#endif
/**
 * Bjoern Hoehrmann
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */
void Benchmark::run_convert_utf8_to_utf16_hoehrmann(size_t iterations) {
  uint8_t const *data = input_data.data();
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    sink = hoehrmann::toUtf16(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(
      reinterpret_cast<const char *>(data), size);
  print_summary(result, size, char_count);
}
/**
 * Bjoern Hoehrmann
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */
void Benchmark::run_convert_utf8_to_utf32_hoehrmann(size_t iterations) {
  uint8_t const *data = input_data.data();
  const size_t size = input_data.size();
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    sink = hoehrmann::toUtf32(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(
      reinterpret_cast<const char *>(data), size);
  print_summary(result, size, char_count);
}

#ifdef __x86_64__
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_utf16_to_utf8_utf8lut(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  // utf8lut requires an extra 16 bytes of padding.
  std::unique_ptr<char[]> output_buffer{new char[size * 4 + 16]};

  volatile size_t sink{0};

  auto proc = [data, size, &output_buffer, &sink]() {
    std::unique_ptr<BaseBufferProcessor> processor(
        ProcessorSelector<dfUtf16, dfUtf8>::WithOptions<cmValidate>::Create());
    ConversionResult result = ConvertInMemory(
        *processor, reinterpret_cast<const char *>(data), 2 * size,
        reinterpret_cast<char *>(output_buffer.get()), size * 4 + 16);
    if (result.status != 0) {
      sink = 0;
    } else {
      sink = result.outputSize;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_valid_utf16_to_utf8_utf8lut(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  // utf8lut requires an extra 16 bytes of padding.
  std::unique_ptr<char[]> output_buffer{new char[size * 4 + 16]};

  volatile size_t sink{0};

  auto proc = [data, size, &output_buffer, &sink]() {
    std::unique_ptr<BaseBufferProcessor> processor(
        ProcessorSelector<dfUtf16, dfUtf8>::WithOptions<cmFull>::Create());
    ConversionResult result = ConvertInMemory(
        *processor, reinterpret_cast<const char *>(data), 2 * size,
        reinterpret_cast<char *>(output_buffer.get()), size * 4 + 16);
    if (result.status != 0) {
      sink = 0;
    } else {
      sink = result.outputSize;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_utf8_to_utf16_utf8lut(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  // utf8lut requires an extra 8 bytes of padding.
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2 + 8]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    std::unique_ptr<BaseBufferProcessor> processor(
        ProcessorSelector<dfUtf8, dfUtf16>::WithOptions<cmValidate>::Create());
    ConversionResult result = ConvertInMemory(
        *processor, data, size, reinterpret_cast<char *>(output_buffer.get()),
        size * 2 + 16);
    if (result.status != 0) {
      sink = 0;
    } else {
      sink = result.outputSize / 2;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_utf8_to_utf32_utf8lut(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();

  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size + 4]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    std::unique_ptr<BaseBufferProcessor> processor(
        ProcessorSelector<dfUtf8, dfUtf32>::WithOptions<cmValidate>::Create());
    ConversionResult result = ConvertInMemory(
        *processor, data, size, reinterpret_cast<char *>(output_buffer.get()),
        size * 4 + 16);
    if (result.status != 0) {
      sink = 0;
    } else {
      sink = result.outputSize / 2;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_valid_utf8_to_utf16_utf8lut(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  // utf8lut requires an extra 8 bytes of padding.
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2 + 8]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    std::unique_ptr<BaseBufferProcessor> processor(
        ProcessorSelector<dfUtf8, dfUtf16>::WithOptions<cmFull>::Create());
    ConversionResult result = ConvertInMemory(
        *processor, data, size, reinterpret_cast<char *>(output_buffer.get()),
        size * 2 + 16);
    if (result.status != 0) {
      sink = 0;
    } else {
      sink = result.outputSize / 2;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_utf32_to_utf8_utf8lut(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: a single 32-bit word can yield up to four UTF-8 bytes. We are
  //       making a safe assumption that each 32-bit word will yield four
  //       UTF-8 bytes.
  // utf8lut requires an extra 16 bytes of padding.
  std::unique_ptr<char[]> output_buffer{new char[size * 4 + 16]};

  volatile size_t sink{0};

  auto proc = [data, size, &output_buffer, &sink]() {
    std::unique_ptr<BaseBufferProcessor> processor(
        ProcessorSelector<dfUtf32, dfUtf8>::WithOptions<cmValidate>::Create());
    ConversionResult result = ConvertInMemory(
        *processor, reinterpret_cast<const char *>(data), 4 * size,
        reinterpret_cast<char *>(output_buffer.get()), size * 4 + 16);
    if (result.status != 0) {
      sink = 0;
    } else {
      sink = result.outputSize;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_valid_utf8_to_utf32_utf8lut(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();

  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size + 4]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    std::unique_ptr<BaseBufferProcessor> processor(
        ProcessorSelector<dfUtf8, dfUtf32>::WithOptions<cmFull>::Create());
    ConversionResult result = ConvertInMemory(
        *processor, data, size, reinterpret_cast<char *>(output_buffer.get()),
        size * 4 + 16);
    if (result.status != 0) {
      sink = 0;
    } else {
      sink = result.outputSize / 2;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_valid_utf32_to_utf8_utf8lut(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: a single 32-bit word can yield up to four UTF-8 bytes. We are
  //       making a safe assumption that each 32-bit word will yield four
  //       UTF-8 bytes.
  // utf8lut requires an extra 16 bytes of padding.
  std::unique_ptr<char[]> output_buffer{new char[size * 4 + 16]};

  volatile size_t sink{0};

  auto proc = [data, size, &output_buffer, &sink]() {
    std::unique_ptr<BaseBufferProcessor> processor(
        ProcessorSelector<dfUtf32, dfUtf8>::WithOptions<cmFull>::Create());
    ConversionResult result = ConvertInMemory(
        *processor, reinterpret_cast<const char *>(data), 4 * size,
        reinterpret_cast<char *>(output_buffer.get()), size * 4 + 16);
    if (result.status != 0) {
      sink = 0;
    } else {
      sink = result.outputSize;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}
/**
 * Bob Steagall, CppCon2018
 * https://github.com/BobSteagall/CppCon2018/
 *
 * Fast Conversion From UTF-8 with C++, DFAs, and SSE Intrinsics
 * https://www.youtube.com/watch?v=5FQ87-Ecb-A
 */
void Benchmark::run_convert_utf8_to_utf16_cppcon2018(size_t iterations) {
  using char8_t = unsigned char;
  const char8_t *data = reinterpret_cast<const char8_t *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    sink = uu::UtfUtils::SseConvert(data, data + size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(
      reinterpret_cast<const char *>(data), size);
  print_summary(result, size, char_count);
}
/**
 * Bob Steagall, CppCon2018
 * https://github.com/BobSteagall/CppCon2018/
 *
 * Fast Conversion From UTF-8 with C++, DFAs, and SSE Intrinsics
 * https://www.youtube.com/watch?v=5FQ87-Ecb-A
 */
void Benchmark::run_convert_utf8_to_utf32_cppcon2018(size_t iterations) {
  using char8_t = unsigned char;
  const char8_t *data = reinterpret_cast<const char8_t *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    sink = uu::UtfUtils::SseConvert(data, data + size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(
      reinterpret_cast<const char *>(data), size);
  print_summary(result, size, char_count);
}
/**
 * Cameron, Robert D, A case study in SIMD text processing with parallel bit
 * streams: UTF-8 to UTF-16 transcoding, Proceedings of the 13th ACM SIGPLAN
 * Symposium on Principles and practice of parallel programming, 91--98.
 */
void Benchmark::run_convert_utf8_to_utf16_u8u16(size_t iterations) {
  // u8u16 wants to take mutable chars, let us hope it does not actually mutate
  // anything!
  //
  // This is currently untested. At a glance it looks fine, but
  // it is possible that the transcoding could be wrong.
  char *data = reinterpret_cast<char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    char *srcbuf_ptr = data;
    size_t inbytes_left = size;
    char *trgtbuf_ptr = reinterpret_cast<char *>(output_buffer.get());
    size_t outbytes_left = size * sizeof(char16_t);
    size_t result_code =
        u8u16(&srcbuf_ptr, &inbytes_left, &trgtbuf_ptr, &outbytes_left);
    bool is_ok = (result_code != size_t(-1));
    if (is_ok) {
      sink = (reinterpret_cast<char16_t *>(trgtbuf_ptr) - output_buffer.get());
    } else {
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

/**
 * Olivier Goffart, UTF-8 processing using SIMD (SSE4), 2012.
 * https://woboq.com/blog/utf-8-processing-using-simd.html
 */
void Benchmark::run_convert_utf8_to_utf16_utf8sse4(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    const char *srcbuf_ptr = data;
    size_t inbytes_left = size;
    char *trgtbuf_ptr = reinterpret_cast<char *>(output_buffer.get());
    size_t outbytes_left = size * sizeof(char16_t);
    size_t result_code = utf8sse4::fromUtf8(&srcbuf_ptr, &inbytes_left,
                                            &trgtbuf_ptr, &outbytes_left);
    bool is_ok = (result_code != size_t(-1));
    if (is_ok) {
      sink = (reinterpret_cast<char16_t *>(trgtbuf_ptr) - output_buffer.get());
    } else {
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}
#endif

void Benchmark::run_convert_valid_utf8_to_utf16(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_valid_utf8_to_utf16le(data, size,
                                                        output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_valid_utf8_to_utf32(
    const simdutf::implementation &implementation, size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_valid_utf8_to_utf32(data, size,
                                                      output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf16_to_latin1(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;
  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_utf16le_to_latin1(data, size,
                                                    output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_latin1_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;
  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile bool sink{false};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    result res = implementation.convert_utf16le_to_latin1_with_errors(
        data, size, output_buffer.get());
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_valid_utf16_to_latin1(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;
  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_valid_utf16le_to_latin1(data, size,
                                                          output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf8(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink =
        implementation.convert_utf16le_to_utf8(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf8_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile bool sink{false};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    result res = implementation.convert_utf16le_to_utf8_with_errors(
        data, size, output_buffer.get());
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf32(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: all code units yield 4 bytes. We are making a safe assumption that
  // all code units will be non-surrogate code units so the size would get
  // doubled (16 bits -> 32 bits).
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size * 2]};

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_utf16le_to_utf32(data, size,
                                                   output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf32_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: all code units yield 4 bytes. We are making a safe assumption that
  // all code units will be non-surrogate code units so the size would get
  // doubled (16 bits -> 32 bits).
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size * 2]};

  volatile bool sink{false};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    result res = implementation.convert_utf16le_to_utf32_with_errors(
        data, size, output_buffer.get());
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf8_with_dynamic_allocation(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &sink]() {
    auto dyn_size = implementation.utf8_length_from_utf16le(data, size);
    std::unique_ptr<char[]> output_buffer{new char[dyn_size]};
    sink =
        implementation.convert_utf16le_to_utf8(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf32_with_dynamic_allocation(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: all code units yield 4 bytes. We are making a safe assumption that
  // all code units will be non-surrogate code units so the size would get
  // doubled (16 bits -> 32 bits).

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &sink]() {
    auto dyn_size = implementation.utf32_length_from_utf16le(data, size);
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[dyn_size]};
    sink = implementation.convert_utf16le_to_utf32(data, size,
                                                   output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_valid_utf16_to_utf8(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_valid_utf16le_to_utf8(data, size,
                                                        output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_latin1(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink =
        implementation.convert_utf32_to_latin1(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}
void Benchmark::run_convert_utf32_to_latin1_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile bool sink{false};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    result res = implementation.convert_utf32_to_latin1_with_errors(
        data, size, output_buffer.get());
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}
void Benchmark::run_convert_valid_utf32_to_latin1(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  std::unique_ptr<char[]> output_buffer{new char[size]};
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_valid_utf32_to_latin1(data, size,
                                                        output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf8(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: In the "worst" case, a 32-bit word will yield 4 UTF-8 bytes. So, we
  // are making a safe assumption that each word will produce 4 bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink =
        implementation.convert_utf32_to_utf8(data, size, output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf8_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: In the "worst" case, a 32-bit word will yield 4 UTF-8 bytes. So, we
  // are making a safe assumption that each word will produce 4 bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile bool sink{false};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    result res = implementation.convert_utf32_to_utf8_with_errors(
        data, size, output_buffer.get());
    sink = !(res.error);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_valid_utf32_to_utf8(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: In the "worst" case, a 32-bit word will yield 4 UTF-8 bytes. So, we
  // are making a safe assumption that each word will produce 4 bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_valid_utf32_to_utf8(data, size,
                                                      output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_valid_utf16_to_utf32(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size * 4]};

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    sink = implementation.convert_valid_utf16le_to_utf32(data, size,
                                                         output_buffer.get());
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

template <endianness byte_order>
void Benchmark::run_convert_utf32_to_utf16(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: In the "worst" case, a 32-bit word will yield two 16-bit code units.
  // So, we are making a safe assumption that each word will produce 2 bytes.
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2]};

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    if (byte_order == endianness::LITTLE) {
      sink = implementation.convert_utf32_to_utf16le(data, size,
                                                     output_buffer.get());
    } else {
      sink = implementation.convert_utf32_to_utf16be(data, size,
                                                     output_buffer.get());
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

template <endianness byte_order>
void Benchmark::run_convert_utf32_to_utf16_with_errors(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: In the "worst" case, a 32-bit word will yield two 16-bit code units.
  // So, we are making a safe assumption that each word will produce 2 bytes.
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2]};

  volatile bool sink{false};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    if (byte_order == endianness::LITTLE) {
      result res = implementation.convert_utf32_to_utf16le_with_errors(
          data, size, output_buffer.get());
      sink = !(res.error);
    } else {
      result res = implementation.convert_utf32_to_utf16be_with_errors(
          data, size, output_buffer.get());
      sink = !(res.error);
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == false) && (iterations > 0)) {
    std::cerr << "The input was declared invalid.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

template <endianness byte_order>
void Benchmark::run_convert_valid_utf32_to_utf16(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: In the "worst" case, a 32-bit word will yield two 16-bit code units.
  // So, we are making a safe assumption that each word will produce 2 bytes.
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2]};

  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &output_buffer, &sink]() {
    if (byte_order == endianness::LITTLE) {
      sink = implementation.convert_valid_utf32_to_utf16le(data, size,
                                                           output_buffer.get());
    } else {
      sink = implementation.convert_valid_utf32_to_utf16be(data, size,
                                                           output_buffer.get());
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_count_utf8(const simdutf::implementation &implementation,
                               size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.count_utf8(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_count_utf16(const simdutf::implementation &implementation,
                                size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(input_data.data());
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }
  size /= 2;
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.count_utf16le(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_detect_encodings(
    const simdutf::implementation &implementation, size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char *data = reinterpret_cast<const char *>(input_data.data() +
                                                    BOM::bom_byte_size(bom));
  const size_t size = input_data.size() - BOM::bom_byte_size(bom);
  volatile size_t sink{0};
  auto proc = [&implementation, data, size, &sink]() {
    sink = implementation.detect_encodings(data, size);
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  size_t char_count = size;
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  } else {
    std::cout << "Detected format: ";
    if (sink & simdutf::encoding_type::UTF8) {
      char_count = get_active_implementation()->count_utf8(data, size);
      std::cout << " UTF8";
    }
    if (sink & simdutf::encoding_type::UTF16_LE) {
      std::cout << " UTF16LE";
      char_count = get_active_implementation()->count_utf16le(
          reinterpret_cast<const char16_t *>(data), size / 2);
    }
    if (sink & simdutf::encoding_type::UTF32_LE) {
      std::cout << " UTF32LE";
      char_count = size / 4;
    }
    std::cout << std::endl;
  }
  if ((bom) && (bom & ~sink)) {
    std::cerr << "[Error] BOM format     : ";
    if (bom & simdutf::encoding_type::UTF8) {
      std::cerr << " UTF8";
    } else if (bom & simdutf::encoding_type::UTF16_LE) {
      std::cerr << " UTF16LE";
    } else if (bom & simdutf::encoding_type::UTF32_LE) {
      std::cerr << " UTF32LE";
    }
    std::cerr << std::endl;
  }
  if ((sink & (sink - 1)) != 0) {
    std::cout << "More than one format possible, character count is ambiguous."
              << std::endl;
  }
  print_summary(result, size, char_count);
}

const std::set<std::string> Benchmark::all_procedures() const {
  std::set<std::string> result;
  for (const auto &item : benchmarks) {
    result.insert(item.first);
  }

  return result;
}

std::set<simdutf::encoding_type>
Benchmark::expected_encodings(const std::string &procedure) {
  return benchmarks[procedure].second;
}

/**
 * LLVM relies on code from the Unicode Consortium
 * https://en.wikipedia.org/wiki/Unicode_Consortium
 */
void Benchmark::run_convert_utf8_to_utf16_llvm(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    const unsigned char *sourceStart =
        reinterpret_cast<const unsigned char *>(data);
    const unsigned char *sourceEnd = sourceStart + size;
    short unsigned int *targetStart =
        reinterpret_cast<short unsigned int *>(output_buffer.get());
    short unsigned int *targetEnd = targetStart + size;
    bool is_ok = (llvm::conversionOK ==
                  llvm::ConvertUTF8toUTF16(
                      &sourceStart, sourceEnd, &targetStart, targetEnd,
                      llvm::ConversionFlags::strictConversion));
    if (is_ok) {
      sink = (targetStart -
              reinterpret_cast<short unsigned int *>(output_buffer.get()));
    } else {
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf32_llvm(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
  volatile size_t sink{0};
  auto proc = [data, size, &output_buffer, &sink]() {
    const unsigned char *sourceStart =
        reinterpret_cast<const unsigned char *>(data);
    const unsigned char *sourceEnd = sourceStart + size;
    unsigned int *targetStart =
        reinterpret_cast<unsigned int *>(output_buffer.get());
    unsigned int *targetEnd = targetStart + size;
    bool is_ok = (llvm::conversionOK ==
                  llvm::ConvertUTF8toUTF32(
                      &sourceStart, sourceEnd, &targetStart, targetEnd,
                      llvm::ConversionFlags::strictConversion));
    if (is_ok) {
      sink =
          (targetStart - reinterpret_cast<unsigned int *>(output_buffer.get()));
    } else {
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf16_to_utf8_llvm(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: non-surrogate code units can yield up to 3 bytes, a surrogate pair
  // yields 4 bytes,
  //       thus we're making safe assumption that each 16-bit word will be
  //       expanded to four bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile size_t sink{0};

  auto proc = [data, size, &output_buffer, &sink]() {
    const short unsigned int *sourceStart =
        reinterpret_cast<const short unsigned int *>(data);
    const short unsigned int *sourceEnd = sourceStart + size;
    unsigned char *targetStart =
        reinterpret_cast<unsigned char *>(output_buffer.get());
    unsigned char *targetEnd = targetStart + size * 4;
    bool is_ok = (llvm::conversionOK ==
                  llvm::ConvertUTF16toUTF8(
                      &sourceStart, sourceEnd, &targetStart, targetEnd,
                      llvm::ConversionFlags::strictConversion));
    if (is_ok) {
      sink = (targetStart -
              reinterpret_cast<unsigned char *>(output_buffer.get()));
    } else {
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf8_llvm(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: a single 32-bit word can yield up to four UTF-8 bytes. We are
  //       making a safe assumption that each 32-bit word will yield four
  //       UTF-8 bytes.
  std::unique_ptr<char[]> output_buffer{new char[size * 4]};

  volatile size_t sink{0};

  auto proc = [data, size, &output_buffer, &sink]() {
    const unsigned int *sourceStart =
        reinterpret_cast<const unsigned int *>(data);
    const unsigned int *sourceEnd = sourceStart + size;
    unsigned char *targetStart =
        reinterpret_cast<unsigned char *>(output_buffer.get());
    unsigned char *targetEnd = targetStart + size * 4;
    bool is_ok = (llvm::conversionOK ==
                  llvm::ConvertUTF32toUTF8(
                      &sourceStart, sourceEnd, &targetStart, targetEnd,
                      llvm::ConversionFlags::strictConversion));
    if (is_ok) {
      sink = (targetStart -
              reinterpret_cast<unsigned char *>(output_buffer.get()));
    } else {
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf32_llvm(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 2;

  // Note: all code units yield four bytes. We make the safe assumption that all
  // code units will be non surrogate code units so the size will double (16
  // bits -> 32 bits).
  std::unique_ptr<char32_t[]> output_buffer{new char32_t[size * 2]};

  volatile size_t sink{0};

  auto proc = [data, size, &output_buffer, &sink]() {
    const short unsigned int *sourceStart =
        reinterpret_cast<const short unsigned int *>(data);
    const short unsigned int *sourceEnd = sourceStart + size;
    unsigned int *targetStart =
        reinterpret_cast<unsigned int *>(output_buffer.get());
    unsigned int *targetEnd = targetStart + 2 * size;
    bool is_ok = (llvm::conversionOK ==
                  llvm::ConvertUTF16toUTF32(
                      &sourceStart, sourceEnd, &targetStart, targetEnd,
                      llvm::ConversionFlags::strictConversion));
    if (is_ok) {
      sink =
          (targetStart - reinterpret_cast<unsigned int *>(output_buffer.get()));
    } else {
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf16_llvm(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  size /= 4;

  // Note: a single 32-bit word can produce a surrogate pair, i.e. two
  //       16-bit code units. We are making a safe assumption that each 32-
  //       bit word will yield two 16-bit code units.
  std::unique_ptr<char[]> output_buffer{new char[size * 2]};

  volatile size_t sink{0};

  auto proc = [data, size, &output_buffer, &sink]() {
    const unsigned int *sourceStart =
        reinterpret_cast<const unsigned int *>(data);
    const unsigned int *sourceEnd = sourceStart + size;
    short unsigned int *targetStart =
        reinterpret_cast<short unsigned int *>(output_buffer.get());
    short unsigned int *targetEnd = targetStart + size * 2;
    bool is_ok = (llvm::conversionOK ==
                  llvm::ConvertUTF32toUTF16(
                      &sourceStart, sourceEnd, &targetStart, targetEnd,
                      llvm::ConversionFlags::strictConversion));
    if (is_ok) {
      sink = (targetStart -
              reinterpret_cast<short unsigned int *>(output_buffer.get()));
    } else {
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size;
  print_summary(result, input_data.size(), char_count);
}

/**
 * Nemanja Trifunovic, UTF8-CPP: UTF-8 with C++ in a Portable Way
 * https://github.com/nemtrif/utfcpp/releases/tag/v3.2.2
 */
void Benchmark::run_convert_utf8_to_utf16_utfcpp(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  auto proc = [data, size, &sink]() {
    try {
      std::vector<unsigned short> str;
      utf8::utf8to16(data, data + size, std::back_inserter(str));
      sink = str.size();
    } catch (const char *msg) {
      std::cout << msg << std::endl;
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  // checking
  std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
  size_t expected = convert_utf8_to_utf16le(data, size, output_buffer.get());
  if (expected != sink) {
    std::cerr << "The number of UTF-16 code units does not match.\n";
  }
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf16_to_utf8_utfcpp(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char16_t *data = reinterpret_cast<const char16_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 2 != 0) {
    printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
           size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  volatile size_t sink{0};

  auto proc = [data, size, &sink]() {
    try {
      std::string str;
      utf8::utf16to8(data, data + size, std::back_inserter(str));
      sink = str.size();
    } catch (const char *msg) {
      std::cout << msg << std::endl;
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  size /= 2;
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = get_active_implementation()->count_utf16le(data, size);
  print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf8_to_utf32_utfcpp(size_t iterations) {
  const char *data = reinterpret_cast<const char *>(input_data.data());
  const size_t size = input_data.size();
  volatile size_t sink{0};

  auto proc = [data, size, &sink]() {
    try {
      std::vector<int> str;
      utf8::utf8to32(data, data + size, std::back_inserter(str));
      sink = str.size();
    } catch (const char *msg) {
      std::cout << msg << std::endl;
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr
        << "The output is zero which might indicate a misconfiguration.\n";
  }
  size_t char_count = get_active_implementation()->count_utf8(data, size);
  print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf32_to_utf8_utfcpp(size_t iterations) {
  const simdutf::encoding_type bom =
      BOM::check_bom(input_data.data(), input_data.size());
  const char32_t *data = reinterpret_cast<const char32_t *>(
      input_data.data() + BOM::bom_byte_size(bom));
  size_t size = input_data.size() - BOM::bom_byte_size(bom);
  if (size % 4 != 0) {
    printf(
        "# The input size is not divisible by four (it is %zu + %zu for BOM)",
        size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
    printf(" Running function on truncated input.\n");
  }

  volatile size_t sink{0};

  auto proc = [data, size, &sink]() {
    try {
      std::string str;
      utf8::utf16to8(data, data + size, std::back_inserter(str));
      sink = str.size();
    } catch (const char *msg) {
      std::cout << msg << std::endl;
      sink = 0;
    }
  };
  count_events(proc, iterations); // warming up!
  const auto result = count_events(proc, iterations);
  if ((sink == 0) && (size != 0) && (iterations > 0)) {
    std::cerr << "The output is zero which might indicate an error.\n";
  }
  size_t char_count = size / 4;
  print_summary(result, input_data.size(), char_count);
}

} // namespace simdutf::benchmarks
