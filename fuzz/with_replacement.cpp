// Fuzz tests for convert_utf16le_to_utf8_with_replacement and
// convert_utf16be_to_utf8_with_replacement, along with their companion
// length functions utf8_length_from_utf16le_with_replacement and
// utf8_length_from_utf16be_with_replacement.
//
// These functions handle UTF-16 input that may contain unpaired surrogates,
// replacing them with U+FFFD (0xEF 0xBF 0xBD in UTF-8). They always succeed.
//
// Invariants checked:
// 1. All implementations agree on length function output (count and error
// field)
// 2. All implementations agree on conversion output
// 3. The length function's count equals the number of bytes written
// 4. The output is always valid UTF-8
// 5. When the input has no surrogates, the output matches regular conversion

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

#include "helpers/common.h"
#include "simdutf.h"

static void test_utf16le_with_replacement(std::span<const char16_t> input) {
  const auto implementations = get_supported_implementations();
  if (implementations.empty()) {
    return;
  }

  // Step 1: Collect length predictions from all implementations and check
  // agreement.
  std::vector<simdutf::result> len_results;
  len_results.reserve(implementations.size());
  for (auto impl : implementations) {
    len_results.push_back(impl->utf8_length_from_utf16le_with_replacement(
        input.data(), input.size()));
  }
  {
    auto neq = [](const auto& a, const auto& b) { return a != b; };
    if (std::ranges::adjacent_find(len_results, neq) != len_results.end()) {
      std::cerr << "utf8_length_from_utf16le_with_replacement: implementations "
                   "disagree\n";
      for (std::size_t i = 0; i < implementations.size(); ++i) {
        std::cerr << "  " << implementations[i]->name()
                  << ": count=" << len_results[i].count
                  << " error=" << len_results[i].error << "\n";
      }
      std::abort();
    }
  }

  const std::size_t expected_len = len_results[0].count;
  // error == SUCCESS means no surrogates encountered; SURROGATE means at least
  // one.
  const bool has_surrogates = (len_results[0].error != simdutf::SUCCESS);

  // Step 2: Run conversion across all implementations and verify written ==
  // expected_len.
  std::vector<std::vector<char>> outputs;
  outputs.reserve(implementations.size());
  for (auto impl : implementations) {
    std::vector<char> out(expected_len);
    const auto written = impl->convert_utf16le_to_utf8_with_replacement(
        input.data(), input.size(), out.data());
    if (written != expected_len) {
      std::cerr << "convert_utf16le_to_utf8_with_replacement:" << " written="
                << written << " but length predicted=" << expected_len
                << " impl=" << impl->name() << "\n";
      std::abort();
    }
    outputs.push_back(std::move(out));
  }

  // Step 3: All implementations must agree on the output bytes.
  {
    auto neq = [](const auto& a, const auto& b) { return a != b; };
    if (std::ranges::adjacent_find(outputs, neq) != outputs.end()) {
      std::cerr << "convert_utf16le_to_utf8_with_replacement: outputs differ "
                   "between implementations\n";
      for (std::size_t i = 0; i < implementations.size(); ++i) {
        std::cerr << "  " << implementations[i]->name()
                  << ": hash=" << FNV1A_hash::as_str(outputs[i]) << "\n";
      }
      std::abort();
    }
  }

  // Step 4: Output must always be valid UTF-8.
  for (std::size_t i = 0; i < implementations.size(); ++i) {
    if (!implementations[i]->validate_utf8(outputs[i].data(),
                                           outputs[i].size())) {
      std::cerr << "convert_utf16le_to_utf8_with_replacement: output is not "
                   "valid UTF-8"
                << " impl=" << implementations[i]->name() << "\n";
      std::abort();
    }
  }

  // Step 5: When no surrogates were found, match the regular (non-replacement)
  // length.
  if (!has_surrogates) {
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      auto impl = implementations[i];
      const auto regular_len =
          impl->utf8_length_from_utf16le(input.data(), input.size());
      if (regular_len != expected_len) {
        std::cerr
            << "utf16le_with_replacement: no surrogates but length mismatch:"
            << " with_replacement=" << expected_len
            << " regular=" << regular_len << " impl=" << impl->name() << "\n";
        std::abort();
      }
    }
  }
}

static void test_utf16be_with_replacement(std::span<const char16_t> input) {
  const auto implementations = get_supported_implementations();
  if (implementations.empty()) {
    return;
  }

  // Step 1: Collect length predictions from all implementations and check
  // agreement.
  std::vector<simdutf::result> len_results;
  len_results.reserve(implementations.size());
  for (auto impl : implementations) {
    len_results.push_back(impl->utf8_length_from_utf16be_with_replacement(
        input.data(), input.size()));
  }
  {
    auto neq = [](const auto& a, const auto& b) { return a != b; };
    if (std::ranges::adjacent_find(len_results, neq) != len_results.end()) {
      std::cerr << "utf8_length_from_utf16be_with_replacement: implementations "
                   "disagree\n";
      for (std::size_t i = 0; i < implementations.size(); ++i) {
        std::cerr << "  " << implementations[i]->name()
                  << ": count=" << len_results[i].count
                  << " error=" << len_results[i].error << "\n";
      }
      std::abort();
    }
  }

  const std::size_t expected_len = len_results[0].count;
  const bool has_surrogates = (len_results[0].error != simdutf::SUCCESS);

  // Step 2: Run conversion across all implementations and verify written ==
  // expected_len.
  std::vector<std::vector<char>> outputs;
  outputs.reserve(implementations.size());
  for (auto impl : implementations) {
    std::vector<char> out(expected_len);
    const auto written = impl->convert_utf16be_to_utf8_with_replacement(
        input.data(), input.size(), out.data());
    if (written != expected_len) {
      std::cerr << "convert_utf16be_to_utf8_with_replacement:" << " written="
                << written << " but length predicted=" << expected_len
                << " impl=" << impl->name() << "\n";
      std::abort();
    }
    outputs.push_back(std::move(out));
  }

  // Step 3: All implementations must agree on the output bytes.
  {
    auto neq = [](const auto& a, const auto& b) { return a != b; };
    if (std::ranges::adjacent_find(outputs, neq) != outputs.end()) {
      std::cerr << "convert_utf16be_to_utf8_with_replacement: outputs differ "
                   "between implementations\n";
      for (std::size_t i = 0; i < implementations.size(); ++i) {
        std::cerr << "  " << implementations[i]->name()
                  << ": hash=" << FNV1A_hash::as_str(outputs[i]) << "\n";
      }
      std::abort();
    }
  }

  // Step 4: Output must always be valid UTF-8.
  for (std::size_t i = 0; i < implementations.size(); ++i) {
    if (!implementations[i]->validate_utf8(outputs[i].data(),
                                           outputs[i].size())) {
      std::cerr << "convert_utf16be_to_utf8_with_replacement: output is not "
                   "valid UTF-8"
                << " impl=" << implementations[i]->name() << "\n";
      std::abort();
    }
  }

  // Step 5: When no surrogates were found, match the regular (non-replacement)
  // length.
  if (!has_surrogates) {
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      auto impl = implementations[i];
      const auto regular_len =
          impl->utf8_length_from_utf16be(input.data(), input.size());
      if (regular_len != expected_len) {
        std::cerr
            << "utf16be_with_replacement: no surrogates but length mismatch:"
            << " with_replacement=" << expected_len
            << " regular=" << regular_len << " impl=" << impl->name() << "\n";
        std::abort();
      }
    }
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // First 4 bytes: action selector + alignment padding.
  if (size < 4) {
    return 0;
  }
  constexpr auto Ncases = 2u;
  constexpr auto actionmask = std::bit_ceil(Ncases) - 1;
  const auto action = data[0] & actionmask;

  // Advance by 4 so the remaining data is aligned to char16_t.
  data += 4;
  size -= 4;

  const std::span<const char16_t> u16data{
      reinterpret_cast<const char16_t*>(data), size / sizeof(char16_t)};

  switch (action) {
  case 0:
    test_utf16le_with_replacement(u16data);
    break;
  case 1:
    test_utf16be_with_replacement(u16data);
    break;
  }

  return 0;
}
