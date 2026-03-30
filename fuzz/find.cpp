// Fuzz tests for simdutf::find() for both char and char16_t.
//
// find() is used internally by base64 decoding to locate specific characters.
// It returns a pointer to the first occurrence of the character, or end if
// the character is not present.
//
// Invariants checked:
// 1. All implementations agree on the returned position (differential testing).
// 2. The returned pointer is within [start, end].
// 3. If result != end, the character at result equals the searched character.
// 4. No character before result equals the searched character (first
// occurrence).

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

#include "helpers/common.h"
#include "simdutf.h"

static void test_find_char(std::span<const char> input, char needle) {
  const auto implementations = get_supported_implementations();
  if (implementations.empty()) {
    return;
  }

  const char* start = input.data();
  const char* end = input.data() + input.size();

  std::vector<const char*> results;
  results.reserve(implementations.size());
  for (auto impl : implementations) {
    results.push_back(impl->find(start, end, needle));
  }

  // All implementations must agree on the returned pointer.
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "find(char): implementations disagree on result\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "  " << implementations[i]->name()
                << ": offset=" << (results[i] - start) << "\n";
    }
    std::abort();
  }

  const char* result = results[0];

  // Result must be in [start, end].
  if (result < start || result > end) {
    std::cerr << "find(char): result pointer out of range\n";
    std::abort();
  }

  if (result != end) {
    // The character at result must equal the needle.
    if (*result != needle) {
      std::cerr << "find(char): *result != needle\n";
      std::abort();
    }
    // No character before result should equal the needle (first occurrence).
    for (const char* p = start; p < result; ++p) {
      if (*p == needle) {
        std::cerr << "find(char): needle found before reported position\n";
        std::abort();
      }
    }
  } else {
    // needle not found: no character in [start, end) should equal needle.
    for (const char* p = start; p < end; ++p) {
      if (*p == needle) {
        std::cerr << "find(char): result is end but needle exists in input\n";
        std::abort();
      }
    }
  }
}

static void test_find_char16(std::span<const char16_t> input, char16_t needle) {
  const auto implementations = get_supported_implementations();
  if (implementations.empty()) {
    return;
  }

  const char16_t* start = input.data();
  const char16_t* end = input.data() + input.size();

  std::vector<const char16_t*> results;
  results.reserve(implementations.size());
  for (auto impl : implementations) {
    results.push_back(impl->find(start, end, needle));
  }

  // All implementations must agree on the returned pointer.
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "find(char16_t): implementations disagree on result\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "  " << implementations[i]->name()
                << ": offset=" << (results[i] - start) << "\n";
    }
    std::abort();
  }

  const char16_t* result = results[0];

  // Result must be in [start, end].
  if (result < start || result > end) {
    std::cerr << "find(char16_t): result pointer out of range\n";
    std::abort();
  }

  if (result != end) {
    // The character at result must equal the needle.
    if (*result != needle) {
      std::cerr << "find(char16_t): *result != needle\n";
      std::abort();
    }
    // No character before result should equal the needle (first occurrence).
    for (const char16_t* p = start; p < result; ++p) {
      if (*p == needle) {
        std::cerr << "find(char16_t): needle found before reported position\n";
        std::abort();
      }
    }
  } else {
    // needle not found: no character in [start, end) should equal needle.
    for (const char16_t* p = start; p < end; ++p) {
      if (*p == needle) {
        std::cerr
            << "find(char16_t): result is end but needle exists in input\n";
        std::abort();
      }
    }
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Layout: [action(1)] [needle_lo(1)] [needle_hi(1)] [pad(1)] [payload...]
  // For char: needle = needle_lo (low byte)
  // For char16_t: needle = needle_lo | (needle_hi << 8)
  if (size < 4) {
    return 0;
  }

  constexpr auto Ncases = 2u;
  constexpr auto actionmask = std::bit_ceil(Ncases) - 1;
  const auto action = data[0] & actionmask;
  const auto needle_lo = data[1];
  const auto needle_hi = data[2];
  // data[3] = pad for alignment

  data += 4;
  size -= 4;

  switch (action) {
  case 0: {
    const char needle = static_cast<char>(needle_lo);
    const std::span<const char> chardata{reinterpret_cast<const char*>(data),
                                         size};
    test_find_char(chardata, needle);
  } break;
  case 1: {
    const char16_t needle = static_cast<char16_t>(
        needle_lo | (static_cast<uint16_t>(needle_hi) << 8));
    const std::span<const char16_t> chardata{
        reinterpret_cast<const char16_t*>(data), size / sizeof(char16_t)};
    test_find_char16(chardata, needle);
  } break;
  }

  return 0;
}
