// Tests for the simdutf::inlineable namespace. These cover a few of the
// fully-inlineable scalar variants of the public API, and illustrate the
// recommended "branch on input size" pattern: dispatch small inputs to the
// inlineable scalar variant, and large inputs to the runtime-dispatched SIMD
// implementation in the main simdutf library.

#include "simdutf.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include <tests/helpers/test.h>

namespace {

// Threshold under which we route calls to the inlineable (scalar) namespace.
// In real code this would be tuned to the workload; for the tests we only
// need a threshold that is small enough that at least one of our inputs is
// below it and at least one is above it.
constexpr size_t small_input_threshold = 32;

// Wrapper that demonstrates the intended usage pattern.
size_t convert_utf8_to_utf16_auto(const char *input, size_t len,
                                  char16_t *output) {
  if (len < small_input_threshold) {
    return simdutf::inlineable::convert_utf8_to_utf16(input, len, output);
  }
  return simdutf::convert_utf8_to_utf16(input, len, output);
}

bool validate_utf8_auto(const char *input, size_t len) {
  if (len < small_input_threshold) {
    return simdutf::inlineable::validate_utf8(input, len);
  }
  return simdutf::validate_utf8(input, len);
}

size_t convert_utf16_to_utf8_auto(const char16_t *input, size_t len,
                                  char *output) {
  if (len < small_input_threshold) {
    return simdutf::inlineable::convert_utf16_to_utf8(input, len, output);
  }
  return simdutf::convert_utf16_to_utf8(input, len, output);
}

} // namespace

TEST(inlineable_validate_utf8_matches_simdutf) {
  const std::vector<std::string> inputs = {
      "",
      "a",
      "hello",
      "Привет, мир!",            // Cyrillic
      "héllo wörld",             // Latin-1 supplement via UTF-8
      std::string(500, 'x'),     // pure ASCII, above threshold
      "日本語テキスト",          // Japanese
      std::string("\xff\xfe"),   // invalid UTF-8
      std::string("abc\xc3\x28") // invalid continuation
  };

  for (const auto &s : inputs) {
    const bool lib = simdutf::validate_utf8(s.data(), s.size());
    const bool inl = simdutf::inlineable::validate_utf8(s.data(), s.size());
    ASSERT_EQUAL(lib, inl);
    // The dispatching helper must agree with the library result.
    ASSERT_EQUAL(validate_utf8_auto(s.data(), s.size()), lib);
  }
}

TEST(inlineable_convert_utf8_to_utf16_matches_simdutf) {
  const std::vector<std::string> inputs = {
      "",
      "a",
      "hello",
      "héllo",
      "Привет, мир!",
      "日本語",
      std::string(200, 'z'), // large pure-ASCII input
  };

  for (const auto &s : inputs) {
    std::vector<char16_t> out_lib(s.size() + 4, 0);
    std::vector<char16_t> out_inl(s.size() + 4, 0);
    std::vector<char16_t> out_auto(s.size() + 4, 0);

    const size_t n_lib =
        simdutf::convert_utf8_to_utf16(s.data(), s.size(), out_lib.data());
    const size_t n_inl = simdutf::inlineable::convert_utf8_to_utf16(
        s.data(), s.size(), out_inl.data());
    const size_t n_auto =
        convert_utf8_to_utf16_auto(s.data(), s.size(), out_auto.data());

    ASSERT_EQUAL(n_lib, n_inl);
    ASSERT_EQUAL(n_lib, n_auto);
    for (size_t i = 0; i < n_lib; i++) {
      ASSERT_EQUAL(out_lib[i], out_inl[i]);
      ASSERT_EQUAL(out_lib[i], out_auto[i]);
    }
  }
}

TEST(inlineable_convert_utf16_to_utf8_matches_simdutf) {
  // Build a variety of UTF-16 inputs from UTF-8 source via the library.
  const std::vector<std::string> sources = {
      "",      "a", "hello", "héllo", "Привет, мир!", "日本語",
      "emoji:😀 done",
  };

  for (const auto &src : sources) {
    std::vector<char16_t> utf16(src.size() + 4, 0);
    const size_t n16 =
        simdutf::convert_utf8_to_utf16(src.data(), src.size(), utf16.data());

    std::vector<char> out_lib(src.size() + 8, 0);
    std::vector<char> out_inl(src.size() + 8, 0);
    std::vector<char> out_auto(src.size() + 8, 0);

    const size_t n_lib =
        simdutf::convert_utf16_to_utf8(utf16.data(), n16, out_lib.data());
    const size_t n_inl = simdutf::inlineable::convert_utf16_to_utf8(
        utf16.data(), n16, out_inl.data());
    const size_t n_auto =
        convert_utf16_to_utf8_auto(utf16.data(), n16, out_auto.data());

    ASSERT_EQUAL(n_lib, n_inl);
    ASSERT_EQUAL(n_lib, n_auto);
    for (size_t i = 0; i < n_lib; i++) {
      ASSERT_EQUAL(out_lib[i], out_inl[i]);
      ASSERT_EQUAL(out_lib[i], out_auto[i]);
    }
  }
}

TEST(inlineable_count_and_length_helpers) {
  const std::string s = "Hello, 世界! 🌍";
  const size_t cp_lib = simdutf::count_utf8(s.data(), s.size());
  const size_t cp_inl = simdutf::inlineable::count_utf8(s.data(), s.size());
  ASSERT_EQUAL(cp_lib, cp_inl);

  const size_t u16_lib =
      simdutf::utf16_length_from_utf8(s.data(), s.size());
  const size_t u16_inl =
      simdutf::inlineable::utf16_length_from_utf8(s.data(), s.size());
  ASSERT_EQUAL(u16_lib, u16_inl);
}

TEST(inlineable_validate_ascii_matches_simdutf) {
  const std::string ok = "plain ascii string";
  ASSERT_TRUE(simdutf::inlineable::validate_ascii(ok.data(), ok.size()));
  ASSERT_EQUAL(simdutf::validate_ascii(ok.data(), ok.size()),
               simdutf::inlineable::validate_ascii(ok.data(), ok.size()));

  const std::string not_ok = "not ascii: é";
  ASSERT_EQUAL(simdutf::validate_ascii(not_ok.data(), not_ok.size()),
               simdutf::inlineable::validate_ascii(not_ok.data(), not_ok.size()));
}

TEST_MAIN
