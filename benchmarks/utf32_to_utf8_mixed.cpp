#include "simdutf.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct options {
  std::string implementation{"haswell"};
  std::string workload{"mixed"};
  size_t size{32768};
  size_t iterations{10000};
};

void print_usage(std::ostream &out, std::string_view program) {
  out << "Usage: " << program
      << " [--implementation NAME]"
         " [--workload ascii|mixed|mixed-after-bmp|emoji]"
         " [--size INPUT_SCALARS] [--iterations COUNT]\n";
}

bool parse_size(std::string_view text, size_t &value) {
  if (text.empty()) {
    return false;
  }
  char *end = nullptr;
  errno = 0;
  const unsigned long long parsed = std::strtoull(text.data(), &end, 10);
  if (errno != 0 || end != text.data() + text.size() || parsed == 0 ||
      parsed > size_t(-1)) {
    return false;
  }
  value = static_cast<size_t>(parsed);
  return true;
}

bool parse_options(int argc, char *argv[], options &parsed) {
  for (int i = 1; i < argc; i++) {
    const std::string_view argument{argv[i]};
    if (argument == "--help") {
      print_usage(std::cout, argv[0]);
      return false;
    }
    if (i + 1 == argc) {
      std::cerr << "missing value for " << argument << '\n';
      return false;
    }
    const std::string_view value{argv[++i]};
    if (argument == "--implementation") {
      parsed.implementation = value;
    } else if (argument == "--workload") {
      parsed.workload = value;
    } else if (argument == "--size") {
      if (!parse_size(value, parsed.size)) {
        std::cerr << "invalid --size value: " << value << '\n';
        return false;
      }
    } else if (argument == "--iterations") {
      if (!parse_size(value, parsed.iterations)) {
        std::cerr << "invalid --iterations value: " << value << '\n';
        return false;
      }
    } else {
      std::cerr << "unknown argument: " << argument << '\n';
      return false;
    }
  }
  return true;
}

std::vector<char32_t> make_input(const options &options) {
  std::vector<char32_t> input(options.size);
  if (options.workload == "ascii") {
    for (size_t i = 0; i < input.size(); i++) {
      input[i] = char32_t(0x20 + (i % 95));
    }
  } else if (options.workload == "emoji") {
    for (size_t i = 0; i < input.size(); i++) {
      input[i] = char32_t(0x1f600 + (i % 64));
    }
  } else if (options.workload == "mixed" ||
             options.workload == "mixed-after-bmp") {
    constexpr std::array<char32_t, 4> codepoints{
        {0x41, 0x00a2, 0x20ac, 0x1f600}};
    const size_t bmp_prefix = options.workload == "mixed-after-bmp" ? 16 : 0;
    for (size_t i = 0; i < input.size(); i++) {
      if (i < bmp_prefix) {
        input[i] = codepoints[i % 3];
      } else {
        const size_t mixed_index = i - bmp_prefix;
        const size_t plan = (mixed_index / 4) & 0xff;
        const size_t width = (plan >> ((mixed_index & 3) * 2)) & 0x3;
        input[i] = codepoints[width];
      }
    }
  } else {
    throw std::invalid_argument(
        "workload must be ascii, mixed, mixed-after-bmp, or emoji");
  }
  return input;
}

std::vector<char> encode_utf8(const std::vector<char32_t> &input) {
  std::vector<char> output;
  output.reserve(input.size() * 4);
  for (const char32_t codepoint : input) {
    if (codepoint <= 0x7f) {
      output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7ff) {
      output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
      output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    } else if (codepoint <= 0xffff) {
      output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
      output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
      output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    } else {
      output.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
      output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
      output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
      output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
  }
  return output;
}

bool verify_conversion(const simdutf::implementation *implementation,
                       const std::vector<char32_t> &input,
                       const std::vector<char> &expected, char *output) {
  const simdutf::result result =
      implementation->convert_utf32_to_utf8_with_errors(input.data(),
                                                         input.size(), output);
  return result.error == simdutf::error_code::SUCCESS &&
         result.count == expected.size() &&
         std::equal(expected.begin(), expected.end(), output);
}

} // namespace

int main(int argc, char *argv[]) {
  options options;
  if (!parse_options(argc, argv, options)) {
    return argc > 1 && std::string_view(argv[1]) == "--help" ? EXIT_SUCCESS
                                                                : EXIT_FAILURE;
  }

  const simdutf::implementation *implementation =
      simdutf::get_available_implementations()[options.implementation];
  if (implementation == nullptr) {
    std::cerr << "implementation is not compiled: " << options.implementation
              << '\n';
    return EXIT_FAILURE;
  }
  if (!implementation->supported_by_runtime_system()) {
    std::cerr << "implementation is unsupported by this host: "
              << options.implementation << '\n';
    return EXIT_FAILURE;
  }

  std::vector<char32_t> input;
  try {
    input = make_input(options);
  } catch (const std::invalid_argument &error) {
    std::cerr << error.what() << '\n';
    print_usage(std::cerr, argv[0]);
    return EXIT_FAILURE;
  }
  const std::vector<char> expected = encode_utf8(input);
  const size_t output_size = expected.size();
  // The valid workloads use an exact-size allocation, rather than the common
  // 4 * input.size() upper bound, to exercise packed-store bounds.
  std::unique_ptr<char[]> output(new char[output_size]);

  if (!verify_conversion(implementation, input, expected, output.get())) {
    std::cerr << "warmup conversion failed\n";
    return EXIT_FAILURE;
  }

  volatile size_t sink = 0;
  const auto start = std::chrono::steady_clock::now();
  for (size_t iteration = 0; iteration < options.iterations; iteration++) {
    const simdutf::result result =
        implementation->convert_utf32_to_utf8_with_errors(input.data(),
                                                           input.size(),
                                                           output.get());
    sink += result.count;
  }
  const auto finish = std::chrono::steady_clock::now();
  if (sink != options.iterations * output_size ||
      !verify_conversion(implementation, input, expected, output.get())) {
    std::cerr << "timed conversion failed\n";
    return EXIT_FAILURE;
  }

  const double elapsed_ns =
      std::chrono::duration<double, std::nano>(finish - start).count();
  const double ns_per_input_scalar =
      elapsed_ns / double(options.iterations * input.size());
  const double ns_per_call = elapsed_ns / double(options.iterations);
  std::cout << std::fixed << std::setprecision(9)
            << "{\"metric\":\"ns/input_scalar\",\"value\":"
            << ns_per_input_scalar << "}" << '\n'
            << "{\"metric\":\"ns/call\",\"value\":" << ns_per_call
            << "}" << '\n';
  return EXIT_SUCCESS;
}
