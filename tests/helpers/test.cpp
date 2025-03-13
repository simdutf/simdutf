#include "test.h"

#include <stdexcept>
#include <cstdio>

bool starts_with(const std::string &s, const std::string &prefix) {
  if (s.size() < prefix.size()) {
    return false;
  }

  for (size_t i = 0; i < prefix.size(); i++) {
    if (s[i] != prefix[i]) {
      return false;
    }
  }

  return true;
}

struct split_result_t {
  bool valid;
  std::string before;
  std::string after;
};

split_result_t split(const std::string &s, char ch) {
  const auto pos = s.find(ch);

  if (pos == std::string::npos) {
    return {false, s, ""};
  }

  return {true, s.substr(0, pos), s.substr(pos + 1)};
}

auto simdutf::test::CommandLine::parse(int argc, char *argv[])
    -> simdutf::test::CommandLine {
  CommandLine cmdline;
  cmdline.seed = 42;

  cmdline.exe_name = argv[0];
  {
    const auto pos = cmdline.exe_name.rfind('/');
    if (pos != std::string::npos) {
      cmdline.exe_name = cmdline.exe_name.substr(pos + 1);
    }
  }

  std::list<std::string> args;
  for (int i = 1; i < argc; i++) {
    std::string arg{argv[i]};
    if ((arg == "-h") || (arg == "--help")) {
      cmdline.show_help = true;
      return cmdline;
    }

    if ((arg == "--show-architectures") or (arg == "-A")) {
      cmdline.show_architectures = true;
      continue;
    }

    if ((arg == "--show-tests") or (arg == "-l")) {
      cmdline.show_tests = true;
      continue;
    }

    if (arg == "--gtest_list_tests") {
      cmdline.gtest_list_tests = true;
      continue;
    }

    if (arg == "--gtest_also_run_disabled_tests") {
      continue;
    }

    args.push_back(std::move(arg));
  }

  if (cmdline.show_tests or cmdline.show_architectures) {
    return cmdline;
  }

  while (not args.empty()) {
    const auto arg = args.front();
    args.pop_front();

    if ((arg == "-a") or (arg == "--arch")) {
      if (args.empty()) {
        throw std::invalid_argument("Expected architecture name after " + arg);
      }

      cmdline.architectures.insert(args.front());
      args.pop_front();
    } else if ((arg == "-t") or (arg == "--test")) {
      if (args.empty()) {
        throw std::invalid_argument("Expected test name after " + arg);
      }

      cmdline.tests.push_back(args.front());
      args.pop_front();
    } else if ((arg == "-s") or (arg == "--seed")) {
      if (args.empty()) {
        throw std::invalid_argument("Expected seed value " + arg);
      }

      try {
        cmdline.seed = std::stoi(args.front());
      } catch (const std::exception &e) {
        throw std::invalid_argument("Wrong number after " + arg);
      }
      args.pop_front();
    } else if (starts_with(arg, "--gtest_filter=")) {
      const auto s1 = split(arg, '=');
      const auto s2 = split(s1.after, '.');
      if (not s2.valid) {
        std::invalid_argument("Missing test suite name '" + arg + "'");
      }

      if (s2.before != cmdline.exe_name) {
        std::invalid_argument("Expected suite name '" + cmdline.exe_name +
                              "', got '" + s2.before + "'");
      }

      cmdline.tests.push_back(s2.after);
    } else {
      throw std::invalid_argument("Unknown argument '" + arg + "'");
    }
  }

  return cmdline;
}

void print_help(FILE *file) {
  fputs(R"txt(
Test utility for simdutf

Usage:

    -h, --help                      show help
    -A, --show-architectures        show available architectures
    -l, --show-tests                show name of available tests
    -a [ARCH], --arch [ARCH]        run tests only for selected architecture(s)
    -t [TEST], --test [TEST]        run tests matching all given strings
    -s [SEED], --seed [SEED]        set the random seed

Examples:

    # Test haswell implementations with tests having strings 'convert'
    # and 'utf8' in their name -- for instance 'convert_utf8_to_utf16' and
    # 'convert_utf16_to_utf8'.

    $ test --arch haswell --test convert --test utf8
)txt",
        file);
}

void print_help() { print_help(stdout); }

void print_architectures(FILE *file) {
#if SIMDUTF_IS_BIG_ENDIAN
  fprintf(file, "Big-endian system detected.\n");
#else
  fprintf(file, "Little-endian system detected.\n");
#endif
  fprintf(file, "Available implementations:\n");
  for (const auto &implementation : simdutf::get_available_implementations()) {
    if (implementation == nullptr) {
      puts("implementation is null which is unexpected.");
      abort();
    }
    if (implementation->supported_by_runtime_system()) {
      fprintf(file, "- %s\n", implementation->name().c_str());
    } else {
      fprintf(file, "- %s [unsupported by current processor]\n",
              implementation->name().c_str());
    }
  }
}

void print_architectures() { print_architectures(stdout); }

void print_tests(FILE *file) {
  fprintf(file, "Available tests:\n");
  for (const auto &test : simdutf::test::test_procedures()) {
    fprintf(file, "- %s\n", test.name.c_str());
  }
}

void print_tests() { print_tests(stdout); }

void gtest_list_tests(const std::string &exe_name) {
  printf("%s.\n", exe_name.c_str());
  for (const auto &test : simdutf::test::test_procedures()) {
    printf("  %s\n", test.name.c_str());
  }
}

namespace simdutf {
namespace test {

void run(const CommandLine &cmdline) {
  if (cmdline.show_help) {
    print_help();
    return;
  }

  if (cmdline.show_architectures) {
    print_architectures();
    return;
  }

  if (cmdline.show_tests) {
    print_tests();
    return;
  }

  if (cmdline.gtest_list_tests) {
    gtest_list_tests(cmdline.exe_name);
    return;
  }

  size_t matching_implementation{0};

  for (const auto &implementation : simdutf::get_available_implementations()) {
    if (implementation == nullptr) {
      puts("implementation is null which is unexpected");
      abort();
    }
    if (!implementation->supported_by_runtime_system()) {
      printf("Implementation %s is unsupported by the current processor.\n",
             implementation->name().c_str());
      continue;
    }
    if (not cmdline.architectures.empty()) {
      if (cmdline.architectures.count(implementation->name()) == 0) {
        continue;
      }
    }
    matching_implementation++;

    printf("Checking implementation %s\n", implementation->name().c_str());

    auto filter = [&cmdline](const simdutf::test::test_entry &test) -> bool {
      if (cmdline.tests.empty())
        return true;

      for (const auto &str : cmdline.tests) {
        if (test.name.find(str) != std::string::npos)
          return true;
      }

      return false;
    };

    for (auto &test : simdutf::test::test_procedures()) {
      if (filter(test)) {
        printf("Running %s...", test.title.c_str());
        fflush(stdout);
        test(*implementation);
        puts(" OK");
      }
    }
  }

  if (matching_implementation == 0) {
    puts("not a single compatible implementation found, this is an error");
    abort();
  }
}

std::list<test_entry> &test_procedures() {
  static std::list<test_entry> singleton;

  return singleton;
}

register_test::register_test(const char *name, test_procedure proc) {
  std::string title = name;
  std::replace(title.begin(), title.end(), '_', ' ');
  test_procedures().push_back({name, title, proc});
}

int main(int argc, char *argv[], bool use_threads) {
  auto cmdline = CommandLine::parse(argc, argv);

  run(cmdline);

  return 0;
}

} // namespace test
} // namespace simdutf
