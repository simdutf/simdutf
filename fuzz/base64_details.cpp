// Fuzz tests for base64_to_binary_details (both char and char16_t variants).
//
// base64_to_binary_details returns a full_result that carries both input_count
// and output_count even on errors, unlike base64_to_binary which only returns
// one of them. This provides richer checking opportunities.
//
// Invariants checked:
// 1. All implementations agree on full_result (differential testing).
// 2. input_count <= input length and output_count <= maximal_binary_length.
// 3. Consistency: full_result cast to result matches base64_to_binary result.
// 4. On success, the output bytes from both functions are identical.
// 5. padding_error is consistent across implementations.

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

#include "helpers/common.h"
#include "simdutf.h"

constexpr std::array options = {
    simdutf::base64_default,
    simdutf::base64_url,
    simdutf::base64_default_no_padding,
    simdutf::base64_url_with_padding,
};

constexpr std::array last_chunk = {
    simdutf::last_chunk_handling_options::loose,
    simdutf::last_chunk_handling_options::strict,
    simdutf::last_chunk_handling_options::stop_before_partial,
};

struct comparable_full_result {
  simdutf::error_code error{};
  std::size_t input_count{};
  std::size_t output_count{};
  bool padding_error{};
  std::string output_hash; // hash of output, only set on SUCCESS

  auto operator<=>(const comparable_full_result&) const = default;
};

std::ostream& operator<<(std::ostream& os, const comparable_full_result& r) {
  os << "[error=" << r.error << " in=" << r.input_count
     << " out=" << r.output_count << " pad_err=" << r.padding_error
     << " hash=" << r.output_hash << "]";
  return os;
}

template <typename FromChar>
static void test_details(std::span<const FromChar> input,
                         simdutf::base64_options opt,
                         simdutf::last_chunk_handling_options lco) {
  const auto implementations = get_supported_implementations();
  if (implementations.empty()) {
    return;
  }

  std::vector<comparable_full_result> results;
  results.reserve(implementations.size());

  for (auto impl : implementations) {
    // Use a fresh buffer for base64_to_binary_details.
    const std::size_t maxbinary =
        impl->maximal_binary_length_from_base64(input.data(), input.size());
    std::vector<char> out_details(maxbinary);

    const simdutf::full_result fr = impl->base64_to_binary_details(
        input.data(), input.size(), out_details.data(), opt, lco);

    // Bounds checks.
    if (fr.input_count > input.size()) {
      std::cerr << "base64_to_binary_details: input_count=" << fr.input_count
                << " > input.size()=" << input.size()
                << " impl=" << impl->name() << "\n";
      std::abort();
    }
    if (fr.output_count > maxbinary) {
      std::cerr << "base64_to_binary_details: output_count=" << fr.output_count
                << " > maxbinary=" << maxbinary << " impl=" << impl->name()
                << "\n";
      std::abort();
    }

    // Consistency check: full_result cast to result must equal
    // base64_to_binary.
    std::vector<char> out_plain(maxbinary);
    const simdutf::result r = impl->base64_to_binary(
        input.data(), input.size(), out_plain.data(), opt, lco);
    const simdutf::result r_from_fr = static_cast<simdutf::result>(fr);

    if (r != r_from_fr) {
      std::cerr << "base64_to_binary_details vs base64_to_binary inconsistency"
                << " impl=" << impl->name()
                << " base64_to_binary=[error=" << r.error
                << " count=" << r.count << "]"
                << " full_result cast=[error=" << r_from_fr.error
                << " count=" << r_from_fr.count << "]\n";
      std::abort();
    }

    // On success, output bytes from both functions must match.
    if (fr.error == simdutf::error_code::SUCCESS) {
      if (fr.output_count != r.count) {
        std::cerr << "base64_to_binary_details: SUCCESS but output_count="
                  << fr.output_count << " != r.count=" << r.count
                  << " impl=" << impl->name() << "\n";
        std::abort();
      }
      const auto span_details =
          std::span(out_details).subspan(0, fr.output_count);
      const auto span_plain = std::span(out_plain).subspan(0, r.count);
      if (!std::ranges::equal(span_details, span_plain)) {
        std::cerr << "base64_to_binary_details: output bytes differ from "
                     "base64_to_binary on SUCCESS"
                  << " impl=" << impl->name() << "\n";
        std::abort();
      }
    }

    // Store result for cross-implementation comparison.
    // output_count on error is implementation-defined: SIMD implementations
    // process input in different chunk widths and may write different numbers
    // of output bytes before detecting an invalid character. Only compare
    // output_count (and output_hash) when the conversion succeeds.
    comparable_full_result cfr;
    cfr.error = fr.error;
    cfr.input_count = fr.input_count;
    cfr.padding_error = fr.padding_error;
    if (fr.error == simdutf::error_code::SUCCESS) {
      cfr.output_count = fr.output_count;
      cfr.output_hash = FNV1A_hash::as_str(
          std::span(out_details).subspan(0, fr.output_count));
    }
    results.push_back(std::move(cfr));
  }

  // All implementations must agree on the full_result.
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "base64_to_binary_details: implementations disagree\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "  " << implementations[i]->name() << ": " << results[i]
                << "\n";
    }
    std::abort();
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  constexpr auto optionbytes = 4u;
  static_assert(optionbytes % 2 == 0,
                "optionbytes must be even to avoid misaligned char16 pointers");
  if (size < optionbytes) {
    return 0;
  }

  constexpr auto Ncases = 2u;
  constexpr auto actionmask = std::bit_ceil(Ncases) - 1;
  const auto action = data[0] & actionmask;

  const auto opt = [](auto index) {
    if (index >= options.size()) {
      return options[0];
    }
    return options[index];
  }(data[1] & (std::bit_ceil(options.size()) - 1));

  const auto lco =
      (opt == simdutf::base64_url || opt == simdutf::base64_default_no_padding)
          ? simdutf::last_chunk_handling_options::loose
          : [](auto index) {
              if (index >= last_chunk.size()) {
                return last_chunk[0];
              }
              return last_chunk[index];
            }(data[2] & (std::bit_ceil(last_chunk.size()) - 1));

  // data[3] reserved for future use / alignment.

  data += optionbytes;
  size -= optionbytes;

  switch (action) {
  case 0: {
    const std::span<const char> chardata{reinterpret_cast<const char*>(data),
                                         size};
    test_details(chardata, opt, lco);
  } break;
  case 1: {
    const std::span<const char16_t> chardata{
        reinterpret_cast<const char16_t*>(data), size / sizeof(char16_t)};
    test_details(chardata, opt, lco);
  } break;
  }

  return 0;
}
