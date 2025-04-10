#include "src/cmdline.h"
#include "src/benchmark.h"

#include <iostream>

void info_message() {
  std::cout
      << "We define the number of bytes to be the number of *input* bytes.\n";
  std::cout
      << "We define a 'char' to be a code point (between 1 and 4 bytes).\n";
#ifdef ICU_AVAILABLE
  std::cout << "Using ICU version " << U_ICU_VERSION << std::endl;
#endif
#ifdef _LIBICONV_VERSION
  std::cout << "Using iconv version " << _LIBICONV_VERSION << std::endl;
#endif
#if defined(__clang__)
  std::cout << "Compiler: Clang " << __clang_major__ << "." << __clang_minor__
            << "." << __clang_patchlevel__ << "\n";
#elif defined(__GNUC__)
  std::cout << "Compiler: GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "."
            << __GNUC_PATCHLEVEL__ << "\n";
#elif defined(_MSC_VER)
  std::cout << "Compiler: MSVC " << _MSC_VER << "\n";
#endif
  std::cout << "SIMDUTF version: " << SIMDUTF_VERSION << "\n";
  std::cout << "System: " << simdutf::get_active_implementation()->name()
            << "\n";
  std::cout << "===========================\n";
}

int main(int argc, char *argv[]) {
#ifdef INOUE2008
  inoue2008::inoue_test(); // minimal testing
#endif
  using simdutf::benchmarks::CommandLine;
  CommandLine cmdline;
  try {
    cmdline = CommandLine::parse_arguments(argc, argv);
    if (cmdline.show_help) {
      CommandLine::print_help();
      return EXIT_SUCCESS;
    }

    if (cmdline.empty()) {
      CommandLine::print_help();
      return EXIT_FAILURE;
    }
  } catch (const std::exception &e) {
    printf("%s\n", e.what());
    return EXIT_FAILURE;
  }

  using simdutf::benchmarks::Benchmark;
  using simdutf::benchmarks::ListingMode;

  Benchmark benchmark{Benchmark::create(cmdline)};
  if (cmdline.show_procedures != ListingMode::None) {
    benchmark.list_procedures(cmdline.show_procedures);
    return EXIT_SUCCESS;
  }

  info_message();
  return benchmark.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
