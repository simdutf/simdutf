#include "src/cmdline.h"
#include "src/benchmark.h"

#include <iostream>

void info_message() {
  std::cout
      << "We define the number of bytes to be the number of *input* bytes.\n";
  std::cout
      << "We define a 'char' to be a code point (between 1 and 4 bytes).\n";
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
