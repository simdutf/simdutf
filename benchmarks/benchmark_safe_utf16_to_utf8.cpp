// Benchmark capacity-limited UTF-16 to UTF-8 conversion.
#include "simdutf.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string_view>
#include <vector>

namespace {

constexpr size_t guard_length = 32;
constexpr char marker = char(0xa5);

enum class input_kind {
  ascii,
  one_to_three_byte,
  three_to_four_byte,
  late_three_byte,
};

struct options {
  size_t input_length{256};
  size_t iterations{1000000};
  size_t output_capacity{0};
  const char *implementation_name{"haswell"};
  input_kind kind{input_kind::ascii};
  bool capacity_specified{false};
  bool json{false};
};

enum class parse_result { success, help, error };

void print_usage(const char *program) {
  std::printf(
      "Usage: %s [--size N] [--iterations N] [--capacity N] "
      "[--implementation NAME] "
      "[--input ascii|one-to-three|three-to-four|late-three] [--json]\n",
      program);
}

bool parse_size(const char *text, size_t &value) {
  size_t parsed = 0;
  if (*text == '\0') {
    return false;
  }
  for (const char *p = text; *p != '\0'; ++p) {
    if (*p < '0' || *p > '9') {
      return false;
    }
    const size_t digit = size_t(*p - '0');
    if (parsed > (std::numeric_limits<size_t>::max() - digit) / 10) {
      return false;
    }
    parsed = parsed * 10 + digit;
  }
  value = parsed;
  return true;
}

bool parse_input_kind(const char *text, input_kind &kind) {
  const std::string_view name(text);
  if (name == "ascii") {
    kind = input_kind::ascii;
  } else if (name == "one-to-three") {
    kind = input_kind::one_to_three_byte;
  } else if (name == "three-to-four") {
    kind = input_kind::three_to_four_byte;
  } else if (name == "late-three") {
    kind = input_kind::late_three_byte;
  } else {
    return false;
  }
  return true;
}

parse_result parse_options(int argc, char *argv[], options &result) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
      if (!parse_size(argv[++i], result.input_length) ||
          result.input_length == 0) {
        return parse_result::error;
      }
    } else if (std::strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
      if (!parse_size(argv[++i], result.iterations) || result.iterations == 0) {
        return parse_result::error;
      }
    } else if (std::strcmp(argv[i], "--capacity") == 0 && i + 1 < argc) {
      if (!parse_size(argv[++i], result.output_capacity)) {
        return parse_result::error;
      }
      result.capacity_specified = true;
    } else if (std::strcmp(argv[i], "--implementation") == 0 &&
               i + 1 < argc) {
      result.implementation_name = argv[++i];
    } else if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
      if (!parse_input_kind(argv[++i], result.kind)) {
        return parse_result::error;
      }
    } else if (std::strcmp(argv[i], "--json") == 0) {
      result.json = true;
    } else if (std::strcmp(argv[i], "--help") == 0 ||
               std::strcmp(argv[i], "-h") == 0) {
      return parse_result::help;
    } else {
      return parse_result::error;
    }
  }
  return parse_result::success;
}

const char *input_kind_name(input_kind kind) {
  switch (kind) {
  case input_kind::ascii:
    return "ascii";
  case input_kind::one_to_three_byte:
    return "one-to-three";
  case input_kind::three_to_four_byte:
    return "three-to-four";
  case input_kind::late_three_byte:
    return "late-three";
  }
  return "unknown";
}

void make_input(input_kind kind, std::vector<char16_t> &input) {
  switch (kind) {
  case input_kind::ascii:
    for (size_t i = 0; i < input.size(); ++i) {
      input[i] = char16_t((i * 37 + 13) & 0x7f);
    }
    break;
  case input_kind::one_to_three_byte:
    for (size_t i = 0; i < input.size(); ++i) {
      switch (i % 3) {
      case 0:
        input[i] = u'A';
        break;
      case 1:
        input[i] = char16_t(0x00e9);
        break;
      default:
        input[i] = char16_t(0x4e00);
        break;
      }
    }
    break;
  case input_kind::three_to_four_byte:
    for (size_t i = 0; i < input.size();) {
      input[i++] = char16_t(0x4e00);
      if (i + 1 < input.size()) {
        input[i++] = char16_t(0xd83d);
        input[i++] = char16_t(0xde00);
      }
    }
    break;
  case input_kind::late_three_byte:
    std::fill(input.begin(), input.end(), u'A');
    input.back() = char16_t(0x4e00);
    break;
  }
}

bool matches_reference(const std::vector<char> &output,
                       const std::vector<char> &reference, size_t written,
                       size_t reference_written, size_t output_capacity) {
  if (written != reference_written) {
    return false;
  }
  for (size_t i = 0; i < written; ++i) {
    if (output[i] != reference[i]) {
      return false;
    }
  }
  for (size_t i = output_capacity; i < output.size(); ++i) {
    if (output[i] != marker) {
      return false;
    }
  }
  return true;
}

} // namespace

int main(int argc, char *argv[]) {
  options options{};
  switch (parse_options(argc, argv, options)) {
  case parse_result::success:
    break;
  case parse_result::help:
    print_usage(argv[0]);
    return 0;
  case parse_result::error:
    std::fprintf(stderr, "invalid benchmark arguments\n");
    print_usage(argv[0]);
    return 1;
  }
  if (options.iterations >
      std::numeric_limits<size_t>::max() / options.input_length) {
    std::fprintf(stderr, "iteration count is too large\n");
    return 1;
  }

  const simdutf::implementation *const implementation =
      simdutf::get_available_implementations()[options.implementation_name];
  const simdutf::implementation *const fallback =
      simdutf::get_available_implementations()["fallback"];
  if (implementation == nullptr ||
      !implementation->supported_by_runtime_system() || fallback == nullptr) {
    std::fprintf(stderr, "the requested implementation is unavailable\n");
    return 1;
  }

  std::vector<char16_t> input(options.input_length);
  make_input(options.kind, input);
  if (!options.capacity_specified) {
    options.output_capacity =
        simdutf::utf8_length_from_utf16(input.data(), input.size());
  }
  if (options.output_capacity >
      std::numeric_limits<size_t>::max() - guard_length) {
    std::fprintf(stderr, "output capacity is too large\n");
    return 1;
  }

  std::vector<char> reference(options.output_capacity + guard_length, marker);
  std::vector<char> output(options.output_capacity + guard_length, marker);
  simdutf::get_active_implementation() = fallback;
  const size_t reference_written = simdutf::convert_utf16_to_utf8_safe(
      input.data(), input.size(), reference.data(), options.output_capacity);

  simdutf::get_active_implementation() = implementation;
  constexpr size_t warmup_iterations = 10000;
  size_t warmup_written = 0;
  for (size_t i = 0; i < warmup_iterations; ++i) {
    warmup_written = simdutf::convert_utf16_to_utf8_safe(
        input.data(), input.size(), output.data(), options.output_capacity);
  }
  if (!matches_reference(output, reference, warmup_written, reference_written,
                         options.output_capacity)) {
    std::fprintf(stderr, "warmup conversion did not match fallback\n");
    return 1;
  }
  if (reference_written >
      std::numeric_limits<size_t>::max() / options.iterations) {
    std::fprintf(stderr, "iteration count is too large\n");
    return 1;
  }

  std::fill(output.begin(), output.end(), marker);
  size_t total_written = 0;
  const auto start = std::chrono::steady_clock::now();
  for (size_t i = 0; i < options.iterations; ++i) {
    total_written += simdutf::convert_utf16_to_utf8_safe(
        input.data(), input.size(), output.data(), options.output_capacity);
  }
  const auto end = std::chrono::steady_clock::now();

  if (total_written != options.iterations * reference_written ||
      !matches_reference(output, reference, reference_written, reference_written,
                         options.output_capacity)) {
    std::fprintf(stderr, "timed conversion did not match fallback\n");
    return 1;
  }

  const double elapsed_ns =
      std::chrono::duration<double, std::nano>(end - start).count();
  const double ns_per_operation = elapsed_ns / double(options.iterations);
  if (!std::isfinite(ns_per_operation) || ns_per_operation <= 0) {
    std::fprintf(stderr, "invalid elapsed time\n");
    return 1;
  }

  if (options.json) {
    std::printf("{\"metric\":\"ns/op\",\"value\":%.9f}\n",
                ns_per_operation);
  } else {
    std::printf(
        "%s UTF-16 to UTF-8, input: %s, size: %zu, capacity: %zu, "
        "iterations: %zu\n",
        options.implementation_name, input_kind_name(options.kind),
        options.input_length, options.output_capacity, options.iterations);
    std::printf("%.3f ns/op\n", ns_per_operation);
  }
  return 0;
}
