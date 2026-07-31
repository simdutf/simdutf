// This benchmark measures complete Latin-1 materialization into a resizable
// UTF-8 string. The output is checked against a scalar oracle outside the
// timed section. The cold mode creates a new output string for each operation;
// reuse keeps one output string and warms its capacity before timing.
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "simdutf.h"

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__linux__)
  #include <sys/resource.h>
  #include <time.h>
#endif

#if defined(_WIN32)
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #include <psapi.h>
#endif

namespace {

enum class allocator_mode { cold, reuse };

struct config {
  std::size_t bytes{64 * 1024};
  std::size_t iterations{1000};
  std::size_t high_bit_percent{0};
  allocator_mode allocator{allocator_mode::cold};
};

struct allocation_counters {
  std::uint64_t allocation_count{0};
  std::uint64_t allocated_bytes{0};

  void reset() noexcept {
    allocation_count = 0;
    allocated_bytes = 0;
  }
};

template <typename T> class counting_allocator {
public:
  using value_type = T;

  counting_allocator() = delete;
  explicit counting_allocator(allocation_counters &counters) noexcept
      : counters_(&counters) {}

  template <typename U>
  counting_allocator(const counting_allocator<U> &other) noexcept
      : counters_(other.counters_) {}

  T *allocate(std::size_t count) {
    if (count > max_size()) {
      throw std::bad_alloc();
    }
    counters_->allocation_count++;
    counters_->allocated_bytes += count * sizeof(T);
    return std::allocator<T>{}.allocate(count);
  }

  void deallocate(T *pointer, std::size_t count) noexcept {
    std::allocator<T>{}.deallocate(pointer, count);
  }

  std::size_t max_size() const noexcept {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  template <typename U>
  bool operator==(const counting_allocator<U> &other) const noexcept {
    return counters_ == other.counters_;
  }

  template <typename U>
  bool operator!=(const counting_allocator<U> &other) const noexcept {
    return !(*this == other);
  }

  template <typename> friend class counting_allocator;

private:
  allocation_counters *counters_;
};

using output_string =
    std::basic_string<char, std::char_traits<char>, counting_allocator<char>>;

bool parse_number(std::string_view text, std::size_t &number) {
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), number);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

void print_usage(const char *program) {
  std::cout << "Usage: " << program
            << " [--bytes=N] [--iterations=N] [--high-bit-percent=N] "
               "[--allocator=cold|reuse]\n";
}

config parse_config(int argc, char *argv[]) {
  config result;
  for (int index = 1; index < argc; index++) {
    const std::string_view argument(argv[index]);
    if (argument == "--help") {
      print_usage(argv[0]);
      std::exit(EXIT_SUCCESS);
    }

    constexpr std::string_view bytes_prefix{"--bytes="};
    constexpr std::string_view iterations_prefix{"--iterations="};
    constexpr std::string_view high_bit_prefix{"--high-bit-percent="};
    constexpr std::string_view allocator_prefix{"--allocator="};

    if (argument.substr(0, bytes_prefix.size()) == bytes_prefix) {
      if (!parse_number(argument.substr(bytes_prefix.size()), result.bytes)) {
        throw std::runtime_error("invalid --bytes value");
      }
    } else if (argument.substr(0, iterations_prefix.size()) ==
               iterations_prefix) {
      if (!parse_number(argument.substr(iterations_prefix.size()),
                        result.iterations)) {
        throw std::runtime_error("invalid --iterations value");
      }
    } else if (argument.substr(0, high_bit_prefix.size()) == high_bit_prefix) {
      if (!parse_number(argument.substr(high_bit_prefix.size()),
                        result.high_bit_percent) ||
          result.high_bit_percent > 100) {
        throw std::runtime_error("--high-bit-percent must be between 0 and 100");
      }
    } else if (argument.substr(0, allocator_prefix.size()) == allocator_prefix) {
      const auto value = argument.substr(allocator_prefix.size());
      if (value == "cold") {
        result.allocator = allocator_mode::cold;
      } else if (value == "reuse") {
        result.allocator = allocator_mode::reuse;
      } else {
        throw std::runtime_error("--allocator must be cold or reuse");
      }
    } else {
      throw std::runtime_error("unknown argument: " + std::string(argument));
    }
  }

  if (result.bytes == 0) {
    throw std::runtime_error("--bytes must be non-zero");
  }
  if (result.iterations == 0) {
    throw std::runtime_error("--iterations must be non-zero");
  }
  if (result.bytes > std::string{}.max_size() / 2) {
    throw std::runtime_error("--bytes is too large for a UTF-8 result");
  }
  return result;
}

std::string make_input(std::size_t bytes, std::size_t high_bit_percent) {
  std::string input(bytes, '\0');
  std::size_t high_bit_remainder = 0;
  for (std::size_t index = 0; index < input.size(); index++) {
    high_bit_remainder += high_bit_percent;
    if (high_bit_remainder >= 100) {
      high_bit_remainder -= 100;
      input[index] = static_cast<char>(0x80 + (index % 0x80));
    } else {
      input[index] = static_cast<char>('A' + (index % 26));
    }
  }
  return input;
}

std::string latin1_to_utf8_oracle(std::string_view input) {
  std::string expected;
  expected.reserve(input.size() * 2);
  for (const unsigned char character : input) {
    if (character < 0x80) {
      expected.push_back(static_cast<char>(character));
    } else {
      expected.push_back(static_cast<char>((character >> 6) | 0xc0));
      expected.push_back(static_cast<char>((character & 0x3f) | 0x80));
    }
  }
  return expected;
}

// The C++23 resizable-string overload obtains a checked 2x bound, calls the
// dispatched converter once, and commits the converter's exact result length.
std::size_t materialize(std::string_view input, output_string &output) {
  return simdutf::convert_latin1_to_utf8(input.data(), input.size(), output);
}

void verify_materialization(std::string_view input, std::string_view expected,
                            output_string &output) {
  const auto written = materialize(input, output);
  if (written != expected.size() || output.size() != expected.size() ||
      !std::equal(output.begin(), output.end(), expected.begin())) {
    throw std::runtime_error("Latin-1 materialization did not match the oracle");
  }
}

std::uint64_t process_cpu_time_ns() noexcept {
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__linux__)
  struct timespec cpu_timestamp {};
  if (::clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu_timestamp) == 0) {
    return static_cast<std::uint64_t>(cpu_timestamp.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(cpu_timestamp.tv_nsec);
  }
#elif defined(_WIN32)
  FILETIME creation_time{};
  FILETIME exit_time{};
  FILETIME kernel_time{};
  FILETIME user_time{};
  if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time,
                      &kernel_time, &user_time) != 0) {
    ULARGE_INTEGER kernel_ticks{};
    ULARGE_INTEGER user_ticks{};
    kernel_ticks.LowPart = kernel_time.dwLowDateTime;
    kernel_ticks.HighPart = kernel_time.dwHighDateTime;
    user_ticks.LowPart = user_time.dwLowDateTime;
    user_ticks.HighPart = user_time.dwHighDateTime;
    return (kernel_ticks.QuadPart + user_ticks.QuadPart) * 100ULL;
  }
#endif

  const std::clock_t cpu_ticks = std::clock();
  if (cpu_ticks == static_cast<std::clock_t>(-1)) {
    return 0;
  }
  return static_cast<std::uint64_t>(cpu_ticks) * 1000000000ULL /
         CLOCKS_PER_SEC;
}

std::uint64_t peak_rss_bytes() noexcept {
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__linux__)
  struct rusage usage {};
  if (::getrusage(RUSAGE_SELF, &usage) != 0) {
    return 0;
  }
  #if defined(__APPLE__)
  return static_cast<std::uint64_t>(usage.ru_maxrss);
  #else
  return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ULL;
  #endif
#elif defined(_WIN32)
  PROCESS_MEMORY_COUNTERS counters{};
  counters.cb = sizeof(counters);
  if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)) ==
      0) {
    return 0;
  }
  return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
#else
  return 0;
#endif
}

void observe_output(const output_string &output, std::size_t written,
                    volatile std::size_t &sink) {
  const auto middle = output.empty()
                          ? std::size_t{0}
                          : static_cast<unsigned char>(
                                output[output.size() / 2]);
  sink = sink + written + middle;
}

struct measurement {
  double ns_per_op;
  double cpu_ns_per_op;
  double allocations_per_op;
  double allocated_bytes_per_op;
  std::uint64_t peak_rss;
};

measurement measure(const config &configuration, std::string_view input,
                    std::string_view expected) {
  allocation_counters counters;
  const counting_allocator<char> allocator(counters);
  volatile std::size_t sink = 0;

  if (configuration.allocator == allocator_mode::reuse) {
    output_string output(allocator);
    verify_materialization(input, expected, output);
    counters.reset();

    const auto cpu_start = process_cpu_time_ns();
    const auto wall_start = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < configuration.iterations; index++) {
      const auto written = materialize(input, output);
      observe_output(output, written, sink);
    }
    const auto wall_stop = std::chrono::steady_clock::now();
    const auto cpu_stop = process_cpu_time_ns();

    const auto allocation_count = counters.allocation_count;
    const auto allocated_bytes = counters.allocated_bytes;
    verify_materialization(input, expected, output);

    const auto elapsed_ns =
        std::chrono::duration<double, std::nano>(wall_stop - wall_start).count();
    return {elapsed_ns / configuration.iterations,
            static_cast<double>(cpu_stop - cpu_start) / configuration.iterations,
            static_cast<double>(allocation_count) / configuration.iterations,
            static_cast<double>(allocated_bytes) / configuration.iterations,
            peak_rss_bytes()};
  }

  {
    output_string output(allocator);
    verify_materialization(input, expected, output);
  }
  counters.reset();

  const auto cpu_start = process_cpu_time_ns();
  const auto wall_start = std::chrono::steady_clock::now();
  for (std::size_t index = 0; index < configuration.iterations; index++) {
    output_string output(allocator);
    const auto written = materialize(input, output);
    observe_output(output, written, sink);
  }
  const auto wall_stop = std::chrono::steady_clock::now();
  const auto cpu_stop = process_cpu_time_ns();

  const auto allocation_count = counters.allocation_count;
  const auto allocated_bytes = counters.allocated_bytes;
  {
    output_string output(allocator);
    verify_materialization(input, expected, output);
  }

  const auto elapsed_ns =
      std::chrono::duration<double, std::nano>(wall_stop - wall_start).count();
  return {elapsed_ns / configuration.iterations,
          static_cast<double>(cpu_stop - cpu_start) / configuration.iterations,
          static_cast<double>(allocation_count) / configuration.iterations,
          static_cast<double>(allocated_bytes) / configuration.iterations,
          peak_rss_bytes()};
}

void print_metric(std::string_view name, double value) {
  std::cout << "{\"metric\":\"" << name << "\",\"value\":"
            << std::setprecision(17) << value << "}\n";
}

void print_metric(std::string_view name, std::uint64_t value) {
  std::cout << "{\"metric\":\"" << name << "\",\"value\":" << value
            << "}\n";
}

} // namespace

int main(int argc, char *argv[]) {
  try {
    const auto configuration = parse_config(argc, argv);
    const auto input = make_input(configuration.bytes,
                                  configuration.high_bit_percent);
    const auto expected = latin1_to_utf8_oracle(input);
    const auto result = measure(configuration, input, expected);

    print_metric("ns/op", result.ns_per_op);
    print_metric("cpu_ns/op", result.cpu_ns_per_op);
    print_metric("allocations/op", result.allocations_per_op);
    print_metric("allocated_bytes/op", result.allocated_bytes_per_op);
    print_metric("peak_rss_bytes", result.peak_rss);
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
