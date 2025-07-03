#include <vector>
#include <random>
#include <stdexcept>
#include <functional>
#include "benchmark_base.h"
#include "simdutf.h"

// Generates a random UTF-16 string with specified surrogate pairs and
// mismatched surrogates Parameters:
//   length: The total number of UTF-16 code units in the output (size_t)
//   surrogate_pair_percentage: Percentage of code units forming valid surrogate
//   pairs (double, 0.0 to 100.0)
//     Each pair counts as two code units (high surrogate: U+D800 to U+DBFF, low
//     surrogate: U+DC00 to U+DFFF)
//   mismatched_surrogate_percentage: Percentage of code units as isolated
//   surrogates (double, 0.0 to 100.0)
//     These are standalone high or low surrogates not forming valid pairs
// Returns: A vector of char16_t containing the random UTF-16 string
// Throws: std::invalid_argument if percentages are invalid or their sum exceeds
// 100
std::vector<char16_t>
generate_random_utf16(size_t length, double surrogate_pair_percentage,
                      double mismatched_surrogate_percentage) {
  // Validate input parameters
  if (surrogate_pair_percentage < 0.0 || surrogate_pair_percentage > 100.0 ||
      mismatched_surrogate_percentage < 0.0 ||
      mismatched_surrogate_percentage > 100.0) {
    throw std::invalid_argument("Percentages must be between 0 and 100");
  }
  if (surrogate_pair_percentage + mismatched_surrogate_percentage > 100.0) {
    throw std::invalid_argument("Sum of percentages cannot exceed 100");
  }

  // Initialize random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> basic_dist(0x0000,
                                             0xD7FF); // Basic UTF-16 characters
  std::uniform_int_distribution<> high_surrogate_dist(
      0xD800, 0xDBFF); // High surrogate range
  std::uniform_int_distribution<> low_surrogate_dist(
      0xDC00, 0xDFFF); // Low surrogate range
  std::uniform_real_distribution<> percent_dist(0.0, 100.0);

  std::vector<char16_t> result;
  result.reserve(length);

  size_t i = 0;
  while (i < length) {
    double rand_percent = percent_dist(gen);

    // Generate a valid surrogate pair
    if (rand_percent < surrogate_pair_percentage && i + 1 < length) {
      result.push_back(static_cast<char16_t>(high_surrogate_dist(gen)));
      result.push_back(static_cast<char16_t>(low_surrogate_dist(gen)));
      i += 2;
    }
    // Generate a mismatched surrogate
    else if (rand_percent <
             surrogate_pair_percentage + mismatched_surrogate_percentage) {
      // 50% chance of high or low surrogate
      if (percent_dist(gen) < 50.0) {
        result.push_back(static_cast<char16_t>(high_surrogate_dist(gen)));
      } else {
        result.push_back(static_cast<char16_t>(low_surrogate_dist(gen)));
      }
      i += 1;
    }
    // Generate a basic UTF-16 character
    else {
      result.push_back(static_cast<char16_t>(basic_dist(gen)));
      i += 1;
    }
  }

  return result;
}

struct utf16_statistics {
  double surrogate_pair_percentage;
  double mismatched_surrogate_percentage;
};

std::vector<char16_t> generate_random_utf16(size_t length, utf16_statistics s) {
  return generate_random_utf16(length, s.surrogate_pair_percentage,
                               s.mismatched_surrogate_percentage);
}

typedef std::function<void(const char16_t *, size_t, char16_t *)> utf16_fixer;

struct utf16_fixe_implementation {
  utf16_fixe_implementation(utf16_fixer f, const std::string &n)
      : fix(f), name(n) {}
  utf16_fixer fix;
  std::string name;
};

// copied and pasted from v8 source code prior to the inclusion of simdutf.
namespace v8 {
static inline bool IsLeadSurrogate(char16_t code) {
  return (code & 0x1ffc00) == 0xd800;
}
static inline bool IsTrailSurrogate(char16_t code) {
  return (code & 0x1ffc00) == 0xdc00;
}
void ReplaceUnpairedSurrogates(const char16_t *source_code_units, size_t length,
                               char16_t *dest_code_units) {
  // U+FFFD (REPLACEMENT CHARACTER)
  constexpr char16_t kReplacement = 0xFFFD;

  for (size_t i = 0; i < length; i++) {
    const char16_t source_code_unit = source_code_units[i];
    const size_t copy_index = i;
    char16_t dest_code_unit = source_code_unit;
    if (IsLeadSurrogate(source_code_unit)) {
      // The current code unit is a leading surrogate. If it's not followed by a
      // trailing surrogate, replace it with the replacement character.
      if (i == length - 1 || !IsTrailSurrogate(source_code_units[i + 1])) {
        dest_code_unit = kReplacement;
      } else {
        // Copy the paired trailing surrogate. The paired leading surrogate will
        // be copied below.
        ++i;
        dest_code_units[i] = source_code_units[i];
      }
    } else if (IsTrailSurrogate(source_code_unit)) {
      // All paired trailing surrogates are skipped above, so this branch is
      // only for those that are unpaired.
      dest_code_unit = kReplacement;
    }
    dest_code_units[copy_index] = dest_code_unit;
  }
}
} // namespace v8

event_aggregate bench(const utf16_fixe_implementation &f, const char16_t *input,
                      size_t length, char16_t *output,
                      size_t iterations = 100) {
  event_collector collector;
  event_aggregate all{};
  // Some inputs are just too small to measure accurately, so we need to scale
  // them up.
  size_t multiplier = 1;
  while (true) {
    event_aggregate test{};
    collector.start();
    for (size_t i = 0; i < multiplier; i++) {
      f.fix(input, length, output);
    }
    event_count allocate_count = collector.end();
    test << allocate_count;
    if (test.best.elapsed_ns() < 1000000) {
      multiplier *= 2;
    } else {
      break;
    }
  }
  for (size_t i = 0; i < iterations; i++) {
    collector.start();
    for (size_t i = 0; i < multiplier; i++) {
      f.fix(input, length, output);
    }
    event_count allocate_count = collector.end();
    all << allocate_count / multiplier;
  }
  all.has_events = collector.has_events();
  return all;
}

// https://lemire.me/blog/2025/05/22/dividing-an-array-into-fair-sized-chunks/
// Returns the start index and length of the i-th chunk when dividing an array
// of size N into M nearly equal parts. Parameters:
//   N: total size of the array
//   M: number of chunks
//   i: index of the chunk (0-based)
std::pair<size_t, size_t> get_chunk_range_simple(size_t N, size_t M, size_t i) {
  // Calculate the quotient and remainder
  size_t quotient = N / M;
  size_t remainder = N % M;
  size_t start = quotient * i + (i < remainder ? i : remainder);
  size_t length = quotient + (i < remainder ? 1 : 0);
  return {start, length};
}

struct data_point {
  // Constructor
  // Parameters:
  //   size: input size for this data point
  //   e: event_aggregate with measured events
  data_point(size_t size, event_aggregate e)
      : input_size(size), events(std::move(e)) {}
  size_t input_size;
  event_aggregate events;
};

// Prints the table header for all tested fixers.
// Parameters:
//   fixers: vector of all tested UTF-16 fixer implementations
void print_header(std::vector<utf16_fixe_implementation> &fixers) {
  printf("size ");
  for (const auto &fixer : fixers) {
    const char *name = fixer.name.c_str();
    printf("%sGB_s %serror_percent ", name, name);
    event_collector collector;
    if (collector.has_events()) {
      printf("%sins_byte %scycle_byte  %sGHz %sins_cycle ", name, name, name,
             name);
    }
  }
  printf("\n");
}

// Prints the results for a single data point (performance and hardware counters
// if available). Parameters:
//   dp: the data_point to print
void print_data_point(const data_point &dp) {
  const double best_time = dp.events.best.elapsed_ns();
  const double avg_time = dp.events.total.elapsed_ns() / dp.events.iterations;
  const double gbs = dp.input_size / best_time;
  const double error_margin = (avg_time / best_time - 1) * 100;
  printf("%8.3f %.1f ", gbs, error_margin);
  if (dp.events.has_events) {
    const double _1GHz = 1000000000.0;
    const double freq =
        (dp.events.best.cycles() / dp.events.best.elapsed_sec()) / _1GHz;
    const double insperbyte = dp.events.best.instructions() / dp.input_size;
    const double cycleperbyte = dp.events.best.cycles() / dp.input_size;

    const double inspercycle =
        dp.events.best.instructions() / dp.events.best.cycles();

    printf("%8.3f %8.3f %8.3f %8.3f ", insperbyte, cycleperbyte, freq,
           inspercycle);
  }
}
// Prints a row of results for a set of data points (one per fixer for a given
// input size). Parameters:
//   dp: vector of data_point for a given input size
void print_data_point(const std::vector<data_point> &dp) {
  if (dp.empty()) {
    return;
  }
  printf("%zu ", dp[0].input_size);
  for (const auto &d : dp) {
    print_data_point(d);
  }
  printf("\n");
}
// Runs the benchmarks for each fixer on subarrays of increasing size, returns
// the results. Parameters:
//   fixers: vector of all tested UTF-16 fixer implementations
//   s: statistics describing the surrogate/mismatched surrogate ratio
//   max_size: maximum input size to test (default: 1,000,000)
//   output_datapoints: number of different input sizes to test (default: 128)
// Returns: a 2D vector of data_point, indexed by [input size][fixer]
std::vector<std::vector<data_point>>
run_from_utf16(std::vector<utf16_fixe_implementation> &fixers,
               utf16_statistics s, size_t max_size = 1000'000,
               size_t output_datapoints = 128) {
  const std::vector<char16_t> data = generate_random_utf16(max_size, s);
  std::vector<char16_t> output_data(data.begin(), data.end());
  std::vector<std::vector<data_point>> data_points;
  data_points.reserve(output_datapoints);
  for (size_t i = 0; i < output_datapoints; i++) {
    auto [start, length] =
        get_chunk_range_simple(max_size, output_datapoints, i);
    std::vector<char16_t> input_data(data.begin() + start,
                                     data.begin() + start + length);
    std::vector<data_point> current_data_points;
    for (const auto &fixer : fixers) {
      // We spend ample time warming up the processor and the cache, for
      // reproducible and stable results
      for (size_t i = 0; i < 10; i++)
        bench(fixer, data.data(), length + start, output_data.data());
      current_data_points.emplace_back(
          start + length,
          bench(fixer, data.data(), length + start, output_data.data()));
    }
    data_points.push_back(current_data_points);
  }
  return data_points;
}

int main(int argc, char *argv[]) {
  // Default number of data points
  size_t num_datapoints = 128;
  // Parse optional --datapoints flag
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.rfind("--datapoints=", 0) == 0) {
      num_datapoints = std::stoul(arg.substr(13));
      // Remove this argument from argv for further parsing
      for (int j = i; j < argc - 1; ++j)
        argv[j] = argv[j + 1];
      --argc;
      break;
    }
  }
  printf("Running with %zu data points\n", num_datapoints);
  if (argc < 4) {
    //  ./build/benchmarks/benchmark_to_well_formed_utf16 1000000 0.1 0.1
    //  [--datapoints=256]
    fprintf(stderr,
            "Usage: %s <length> <surrogate_pair_percentage> "
            "<mismatched_surrogate_percentage> [--datapoints=N]\n",
            argv[0]);
    return 1;
  }
  std::vector<utf16_fixe_implementation> fixers;
  fixers.emplace_back(v8::ReplaceUnpairedSurrogates, "v8");

  for (auto &e : simdutf::get_available_implementations()) {
    if (!e->supported_by_runtime_system()) {
      continue;
    }

    utf16_fixer fix = [&e](const char16_t *input, size_t length,
                           char16_t *output) {
      e->to_well_formed_utf16le(input, length, output);
    };
    std::string name = e->name();
    utf16_fixe_implementation fix_implementation{fix, name};

    fixers.push_back(fix_implementation);
  }
  size_t length = std::stoul(argv[1]);
  double surrogate_pair_percentage = std::stod(argv[2]);
  double mismatched_surrogate_percentage = std::stod(argv[3]);

  if (num_datapoints == 0 || num_datapoints > length) {
    fprintf(stderr,
            "Error: [--datapoints=N] must be > 0 and <= <length> (got %zu, "
            "length=%zu)\n",
            num_datapoints, length);
    return 1;
  }

  print_header(fixers);
  std::vector<std::vector<data_point>> dp = run_from_utf16(
      fixers, {surrogate_pair_percentage, mismatched_surrogate_percentage},
      length, num_datapoints);
  for (const std::vector<data_point> &data_points : dp) {
    print_data_point(data_points);
  }

  return EXIT_SUCCESS;
}
