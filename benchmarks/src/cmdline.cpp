#include "cmdline.h"

#include <vector>
#include <stdexcept>

constexpr size_t DEFAULT_ITERATIONS =
    30000; // Such a high number is helpful for stable measures.

namespace {

using simdutf::benchmarks::CommandLine;
using simdutf::benchmarks::ListingMode;

CommandLine parse_arguments(int argc, char *argv[]) {
  CommandLine cmdline;

  std::vector<std::string> arguments;
  for (int i = 1; i < argc; i++) {
    std::string arg{argv[i]};
    if ((arg == "--help") || (arg == "-h")) {
      cmdline.show_help = true;
      return cmdline;
    } else if (arg == "--show-procedures") {
      cmdline.show_procedures = ListingMode::HumanReadable;
    } else if (arg == "-l") {
      cmdline.show_procedures = ListingMode::PlainLines;
    } else {
      arguments.push_back(std::move(arg));
    }
  }

  if (cmdline.show_procedures != ListingMode::None || cmdline.show_help) {
    return cmdline;
  }

  enum class Target {
    None,
    Procedure,
    File,
  };

  auto target = Target::None;

  // the presence of '--', so we can have files/names started with '-'
  bool seen_arg_escape = false;

  for (size_t i = 0; i < arguments.size(); /**/) {
    const std::string &arg = arguments[i];

    if ((arg == "-F") || (arg == "--input-file")) {
      target = Target::File;
      seen_arg_escape = false;
      i += 1;
    } else if ((arg == "-P") || (arg == "--procedure")) {
      seen_arg_escape = false;
      target = Target::Procedure;
      i += 1;
    } else if ((arg == "-I") || (arg == "--iterations")) {
      seen_arg_escape = false;
      target = Target::None;
      const std::string &value = arguments.at(i + 1);
      const long iterations = std::stoi(value);
      if (iterations <= 0) {
        throw std::invalid_argument("Iterations must be greater than zero");
      }

      cmdline.iterations.insert(iterations);

      i += 2;
    } else if (arg == "--random-utf8") {
      seen_arg_escape = false;
      target = Target::None;
      const std::string &value = arguments.at(i + 1);
      const long size = std::stoi(value);
      if (size <= 0) {
        throw std::invalid_argument("Input size must be greater than zero");
      }
      cmdline.random_size.insert(size);

      i += 2;
    } else {
      if (arg == "--") {
        seen_arg_escape = true;
        i += 1;
        continue;
      }

      if (!seen_arg_escape && !arg.empty() && arg[0] == '-') {
        throw std::invalid_argument("Unknown option '" + arg + "'");
      }

      switch (target) {
      case Target::None:
        throw std::invalid_argument("Unexpected argument '" + arg + "'");

      case Target::Procedure:
        cmdline.procedures.insert(arg);
        i += 1;
        break;

      case Target::File:
        cmdline.files.insert(arg);
        i += 1;
        break;
      }
    }
  } // for

  return cmdline;
}

void fixup_and_validate(CommandLine &cmdline) {
  if (cmdline.iterations.empty()) {
    const bool default_needed =
        (!cmdline.procedures.empty() || !cmdline.random_size.empty() ||
         !cmdline.files.empty());
    if (default_needed)
      cmdline.iterations.insert(DEFAULT_ITERATIONS);
  }

  for (const auto &path : cmdline.files) {
    if (!std::filesystem::exists(path))
      throw std::runtime_error("File " + path.string() + " does not exist");
  }
}

} // namespace

namespace simdutf::benchmarks {

CommandLine CommandLine::parse_arguments(int argc, char *argv[]) {
  CommandLine cmdline{::parse_arguments(argc, argv)};

  fixup_and_validate(cmdline);

  return cmdline;
}

bool CommandLine::empty() const {
  return procedures.empty() && random_size.empty() && files.empty() &&
         iterations.empty() && (show_procedures == ListingMode::None);
}

void CommandLine::print_help() { print_help(stdout); }

void CommandLine::print_help(FILE *file) {
  fputs(R"txt(
Benchmark utility for simdutf

Usage:

    -h, --help                      show help
    -F [PATH], --input-file [PATH]  set dataset path (may be used many times)
    -P [NAME], --procedure [NAME]   choose procedure(s) to test (may be used many times, a substring match suffices)
    -I --iterations                 number of iterations (default: 3000)
    --random-utf8 [size]            use random UTF8 data of given size
    --show-procedures               list all known procedures in a human-readable way
    -l                              list all known procedures in a machine-friendly format

Examples:

    # test all known UTF8 procedures against 10k random input (in 100 iterations)
    $ benchmark --random-utf8 10240 -I 100

    # test procedures implemented with the haswell kernel against two custom files
    $ benchmark -P haswell -F ~/plain_ascii.txt -F ~/chinese_huge.txt

    # test two selected procedures against all files matching a pattern (POSIX)
    $ benchmark -P convert_utf8_to_utf16+llvm convert_utf8_to_utf16+u8u16 -F *.utf8.txt
)txt",
        file);
}

} // namespace simdutf::benchmarks
