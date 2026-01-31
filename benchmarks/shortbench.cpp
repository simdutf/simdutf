// This benchmnark program seeks to measure the performance of very short
// functions calls to various simdutf functions.
// You can process the result with the script at scripts/shortinputplots.py
// to generate plots. See the instructions in that script for more details.
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <atomic>
#include <array>
#include <cstring>
#include <cmath>
#include <functional>
#include <span>

#include "simdutf.h"

#include "event_counter.h"

struct BenchmarkFunc {
  std::string name;
  std::function<std::function<void()>(std::span<const char>, std::span<char>)>
      maker;
};

// Benchmarked functions.
std::vector<BenchmarkFunc> available_functions = {
    {"validate_ascii",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         bool valid = simdutf::validate_ascii(input.data(), input.size());
         (void)valid;
       };
     }},
    {"validate_utf8",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         bool valid = simdutf::validate_utf8(input.data(), input.size());
         (void)valid;
       };
     }},
    {"utf8_length_from_latin1",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t len =
             simdutf::utf8_length_from_latin1(input.data(), input.size());
         (void)len;
       };
     }},
    {"utf16_length_from_utf8",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t len =
             simdutf::utf16_length_from_utf8(input.data(), input.size());
         (void)len;
       };
     }},
    {"utf32_length_from_utf8",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t len =
             simdutf::utf32_length_from_utf8(input.data(), input.size());
         (void)len;
       };
     }},
    {"count_utf8",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t count = simdutf::count_utf8(input.data(), input.size());
         (void)count;
       };
     }},
    {"count_utf16",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t count = simdutf::count_utf16(
             reinterpret_cast<const char16_t *>(input.data()),
             input.size() / 2);
         (void)count;
       };
     }},
    {"utf8_length_from_utf16",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t len = simdutf::utf8_length_from_utf16(
             reinterpret_cast<const char16_t *>(input.data()),
             input.size() / 2);
         (void)len;
       };
     }},
    {"utf16_length_from_utf32",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t len = simdutf::utf16_length_from_utf32(
             reinterpret_cast<const char32_t *>(input.data()),
             input.size() / 4);
         (void)len;
       };
     }},
    {"utf32_length_from_utf16",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t len = simdutf::utf32_length_from_utf16(
             reinterpret_cast<const char16_t *>(input.data()),
             input.size() / 2);
         (void)len;
       };
     }},
    {"utf8_length_from_utf32",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         size_t len = simdutf::utf8_length_from_utf32(
             reinterpret_cast<const char32_t *>(input.data()),
             input.size() / 4);
         (void)len;
       };
     }},
    {"convert_latin1_to_utf8",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_latin1_to_utf8(
             input.data(), input.size(), output.data());
         (void)len;
       };
     }},
    {"convert_latin1_to_utf16le",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_latin1_to_utf16le(
             input.data(), input.size(),
             reinterpret_cast<char16_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_latin1_to_utf16be",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_latin1_to_utf16be(
             input.data(), input.size(),
             reinterpret_cast<char16_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_latin1_to_utf32",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_latin1_to_utf32(
             input.data(), input.size(),
             reinterpret_cast<char32_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_utf8_to_latin1",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf8_to_latin1(
             input.data(), input.size(), output.data());
         (void)len;
       };
     }},
    {"convert_utf8_to_utf16le",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf8_to_utf16le(
             input.data(), input.size(),
             reinterpret_cast<char16_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_utf8_to_utf16be",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf8_to_utf16be(
             input.data(), input.size(),
             reinterpret_cast<char16_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_utf8_to_utf32",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf8_to_utf32(
             input.data(), input.size(),
             reinterpret_cast<char32_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_utf16le_to_latin1",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf16le_to_latin1(
             reinterpret_cast<const char16_t *>(input.data()), input.size() / 2,
             output.data());
         (void)len;
       };
     }},
    {"convert_utf16le_to_utf8",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf16le_to_utf8(
             reinterpret_cast<const char16_t *>(input.data()), input.size() / 2,
             output.data());
         (void)len;
       };
     }},
    {"convert_utf16le_to_utf32",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf16le_to_utf32(
             reinterpret_cast<const char16_t *>(input.data()), input.size() / 2,
             reinterpret_cast<char32_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_utf16be_to_latin1",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf16be_to_latin1(
             reinterpret_cast<const char16_t *>(input.data()), input.size() / 2,
             output.data());
         (void)len;
       };
     }},
    {"convert_utf16be_to_utf8",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf16be_to_utf8(
             reinterpret_cast<const char16_t *>(input.data()), input.size() / 2,
             output.data());
         (void)len;
       };
     }},
    {"convert_utf16be_to_utf32",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf16be_to_utf32(
             reinterpret_cast<const char16_t *>(input.data()), input.size() / 2,
             reinterpret_cast<char32_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_utf32_to_latin1",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf32_to_latin1(
             reinterpret_cast<const char32_t *>(input.data()), input.size() / 4,
             output.data());
         (void)len;
       };
     }},
    {"convert_utf32_to_utf8",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf32_to_utf8(
             reinterpret_cast<const char32_t *>(input.data()), input.size() / 4,
             output.data());
         (void)len;
       };
     }},
    {"convert_utf32_to_utf16le",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf32_to_utf16le(
             reinterpret_cast<const char32_t *>(input.data()), input.size() / 4,
             reinterpret_cast<char16_t *>(output.data()));
         (void)len;
       };
     }},
    {"convert_utf32_to_utf16be",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::convert_utf32_to_utf16be(
             reinterpret_cast<const char32_t *>(input.data()), input.size() / 4,
             reinterpret_cast<char16_t *>(output.data()));
         (void)len;
       };
     }},
    {"binary_to_base64",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         size_t len = simdutf::binary_to_base64(input.data(), input.size(),
                                                output.data());
         (void)len;
       };
     }},
    {"base64_to_binary",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         auto result = simdutf::base64_to_binary(input.data(), input.size(),
                                                 output.data());
         (void)result;
       };
     }},
    {"base64_to_binary_safe",
     [](std::span<const char> input, std::span<char> output) {
       return [input, output]() {
         auto result = simdutf::base64_to_binary_safe(input, output);
         (void)result;
       };
     }},
    {"find_equal",
     [](std::span<const char> input, [[maybe_unused]] std::span<char> output) {
       return [input]() {
         auto it = std::find(input.begin(), input.end(), '=');
         (void)it;
       };
     }},
};

struct time_stats {
  double m_instructions;
  double m_cycles;
  double m_nanoseconds;
  double m_error; // relative error estimate
  double elapsed_ns() const { return m_nanoseconds; }
  double cycles() const { return m_cycles; }
  double instructions() const { return m_instructions; }
  double error() const { return m_error; }
};

event_collector collector;

// We expect the functions to be really short, so we do not want too much
// overhead. Thus we benchmark with multiple inner repeats.
template <class function_type>
time_stats bench(const function_type &function, size_t min_inner_repeat = 1000,
                 size_t min_repeat = 10, size_t min_time_ns = 1000000000,
                 size_t max_repeat = 1000000) {
  event_aggregate aggregate{};
  size_t N = min_repeat;
  if (N == 0) {
    N = 1;
  }
  for (size_t i = 0; i < N; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    for (size_t j = 0; j < min_inner_repeat; j++) {
      function();
    }
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    aggregate << allocate_count;
    if ((i + 1 == N) && (aggregate.total_elapsed_ns() < min_time_ns) &&
        (N < max_repeat)) {
      N *= 10;
    }
  }
  // As an estimate of the timing error, we report the difference between the
  // average run and the best run. Note that distributions are expected to be
  // *log-normal*, and not normal, so this is a good measure of variability.
  double error = (aggregate.elapsed_ns() - aggregate.best.elapsed_ns()) /
                 aggregate.worst.elapsed_ns();
  return time_stats{aggregate.instructions() / min_inner_repeat,
                    aggregate.cycles() / min_inner_repeat,
                    aggregate.elapsed_ns() / min_inner_repeat, error};
}

std::vector<char> read_file(const char *filename) {
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  file.open(filename);
  std::vector<char> input_data;
  input_data.assign(std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>());
  return input_data;
}

void print_table_header(bool has_events) {
  if (has_events) {
    printf("%-10s %-18s %-18s %6s %-15s %-15s %-15s\n", "Size",
           "Total Time (ns)", "Time/Byte (ns)", "Err%", "Cycles/Byte",
           "Ins/Byte", "Ins/Cycle");
    printf("-------------------------------------------------------------------"
           "---------------------------------------------\n");
  } else {
    printf("%-10s %-18s %-18s %6s\n", "Size", "Total Time (ns)",
           "Time/Byte (ns)", "Err%");
    printf("---------------------------------------------------------------\n");
  }
}

void print_table_row(size_t size, time_stats stats, bool has_events) {
  double total_ns = stats.elapsed_ns();
  double time_per_byte_ns = (size == 0) ? 0.0 : total_ns / (double)size;
  int error_pct = int(std::lround(stats.error() * 100.0));
  if (error_pct < 0)
    error_pct = 0;
  if (error_pct > 100)
    error_pct = 100;

  if (has_events) {
    double cycles_per_byte = (size == 0) ? 0.0 : stats.cycles() / (double)size;
    double Ins_per_byte =
        (size == 0) ? 0.0 : stats.instructions() / (double)size;
    double Ins_per_cycle = (stats.cycles() == 0)
                               ? 0.0
                               : stats.instructions() / (double)stats.cycles();
    printf("%-10zu %-18.1f   %-15.1f   %5d  %-15.1f %-15.1f   %-15.1f\n", size,
           total_ns, time_per_byte_ns, error_pct, cycles_per_byte, Ins_per_byte,
           Ins_per_cycle);
  } else {
    printf("%-10zu %-18.1f   %-15.1f   %5d\n", size, total_ns, time_per_byte_ns,
           error_pct);
  }
}

int main(int argc, char *argv[]) {
  const bool has_events = collector.has_events();
  if (!has_events) {
    std::cerr << "# Warning: Performance events not available on this system. "
                 "Under macOS and Linux, you may need to run with sudo or "
                 "configure performance counters."
              << std::endl;
  }
  size_t max_size = 128;
  const char *filename = nullptr;
  std::string selected_function = "validate_utf8";
  bool list_functions = false;
  bool all_functions = false;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--max-size") == 0 && i + 1 < argc) {
      max_size = std::stoull(argv[++i]);
    } else if (strcmp(argv[i], "--function") == 0 && i + 1 < argc) {
      selected_function = argv[++i];
    } else if (strcmp(argv[i], "--list") == 0) {
      list_functions = true;
    } else if (strcmp(argv[i], "--all") == 0) {
      all_functions = true;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      std::cout << "Usage: " << argv[0] << " [options] [<filename>]"
                << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  --max-size <size>    Max size to benchmark (default 128)"
                << std::endl;
      std::cout << "  --function <name>    Function to benchmark (default "
                   "validate_utf8)"
                << std::endl;
      std::cout << "  --all                Run all available functions"
                << std::endl;
      std::cout << "  --list               List available functions"
                << std::endl;
      std::cout << "  --help               Show this help" << std::endl;
      return EXIT_SUCCESS;
    } else if (!filename) {
      filename = argv[i];
    } else {
      std::cerr << "Usage: " << argv[0] << " [options] [<filename>]"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (list_functions) {
    std::cout << "Available functions:" << std::endl;
    for (const auto &func : available_functions) {
      std::cout << "  " << func.name << std::endl;
    }
    return EXIT_SUCCESS;
  }

  const BenchmarkFunc *selected_func = nullptr;
  if (!all_functions) {
    // Find the selected function
    auto it = std::find_if(
        available_functions.begin(), available_functions.end(),
        [&](const BenchmarkFunc &f) { return f.name == selected_function; });
    if (it == available_functions.end()) {
      std::cerr << "Unknown function: " << selected_function << std::endl;
      std::cerr << "Use --list to see available functions" << std::endl;
      return EXIT_FAILURE;
    }
    selected_func = &(*it);
  }

  try {
    std::vector<char> data;
    size_t file_size;
    size_t actual_max;
    std::string input_desc;

    if (!filename) {
      data = std::vector<char>(max_size, 0);
      file_size = max_size;
      actual_max = max_size;
      input_desc = "default zero input";
    } else {
      data = read_file(filename);
      file_size = data.size();
      actual_max = std::min(max_size, file_size);
      input_desc = std::string("file: ") + filename;
    }

    std::vector<char> output(4 * max_size);

    if (all_functions) {
      for (const auto &func : available_functions) {
        printf("# Benchmarking %s on %s\n", func.name.c_str(),
               input_desc.c_str());
        printf("# Input size: %zu bytes\n", file_size);
        printf("# Max benchmark size: %zu bytes\n", actual_max);
        printf("# Current system: %s\n",
               simdutf::get_active_implementation()->name().c_str());
        printf("\n");

        print_table_header(has_events);

        for (size_t size = 1; size <= actual_max; ++size) {
          auto benchmark_lambda =
              func.maker(std::span<const char>(data.data(), size),
                         std::span<char>(output.data(), output.size()));
          time_stats stats = bench(benchmark_lambda);

          print_table_row(size, stats, has_events);
        }
        printf("\n");
      }
    } else {
      printf("# Benchmarking %s on %s\n", selected_func->name.c_str(),
             input_desc.c_str());
      printf("# Input size: %zu bytes\n", file_size);
      printf("# Max benchmark size: %zu bytes\n", actual_max);
      printf("# Current system: %s\n",
             simdutf::get_active_implementation()->name().c_str());
      printf("\n");

      print_table_header(has_events);

      for (size_t size = 1; size <= actual_max; ++size) {
        auto benchmark_lambda =
            selected_func->maker(std::span<const char>(data.data(), size),
                                 std::span<char>(output.data(), output.size()));
        time_stats stats = bench(benchmark_lambda);

        print_table_row(size, stats, has_events);
      }
    }

    return EXIT_SUCCESS;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
