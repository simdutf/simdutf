#include "simdutf.h"

#include <tests/helpers/test.h>
#include <tests/normalization_test_data/normalization_test_data.h>

namespace {

using namespace std::literals;

std::string_view combining_grapheme_joiner = "\u034f"sv;

std::string splat(std::string_view sv, int n) {
  std::string result;
  result.reserve((sv.size() + combining_grapheme_joiner.size()) * n);
  for (int i = 0; i < n; ++i) {
    // Insert the combining grapheme joiner at the beginning of every string.
    // This is because normalizaiton is not closed under concatenation, so a
    // naive repetition when the input is made entirely of combining characters
    // will not be canonically ordered. This special character is designed to
    // break up long sequences of combining classes, as it has a combining class
    // of zero. Inserting this in between repetitions will preserve canonical
    // ordering.
    // NOTE: to be more precise, we could conditionally insert this code point
    // depending on if the first character of the input has combining class
    // zero. But CCC lookup is not part of the simdutf public API.
    result += combining_grapheme_joiner;
    result += sv;
  }
  return result;
}

std::pair<std::string, bool> to_nfc(const simdutf::implementation &impl,
                                    std::string_view input) {
  size_t output_length;
  bool qc = impl.normalize_utf8_to_nfc_check(input.data(), input.size(),
                                             &output_length);
  std::string output(output_length, 0);
  size_t written =
      impl.normalize_utf8_to_nfc(input.data(), input.size(), output.data());
  output.resize(written);
  return std::make_pair(output, qc);
}

std::pair<std::string, bool> to_nfd(const simdutf::implementation &impl,
                                    std::string_view input) {
  size_t output_length;
  bool qc = impl.normalize_utf8_to_nfd_check(input.data(), input.size(),
                                             &output_length);
  std::string output(output_length, 0);
  size_t written =
      impl.normalize_utf8_to_nfd(input.data(), input.size(), output.data());
  output.resize(written);
  return std::make_pair(output, qc);
}

std::pair<std::string, bool> to_nfkc(const simdutf::implementation &impl,
                                     std::string_view input) {
  size_t output_length;
  bool qc = impl.normalize_utf8_to_nfkc_check(input.data(), input.size(),
                                              &output_length);
  std::string output(output_length, 0);
  size_t written =
      impl.normalize_utf8_to_nfkc(input.data(), input.size(), output.data());
  output.resize(written);
  return std::make_pair(output, qc);
}

std::pair<std::string, bool> to_nfkd(const simdutf::implementation &impl,
                                     std::string_view input) {
  size_t output_length;
  bool qc = impl.normalize_utf8_to_nfkd_check(input.data(), input.size(),
                                              &output_length);
  std::string output(output_length, 0);
  size_t written =
      impl.normalize_utf8_to_nfkd(input.data(), input.size(), output.data());
  output.resize(written);
  return std::make_pair(output, qc);
}

void dump_hex_string(std::string_view s) {
  for (unsigned char b : s) {
    printf("%02x ", b);
  }
  printf("\n");
}

void report_mismatch(const char *what, std::string_view expected,
                     const std::string &actual, size_t line) {
  printf("normalization conformance failure on line %zu: check "
         "%s\n",
         line, what);
  printf("  expected (%zu bytes): ", expected.size());
  dump_hex_string(expected);
  printf("  actual   (%zu bytes): ", actual.size());
  dump_hex_string(actual);
  exit(1);
}

void report_bad_qc(const char *what, std::string_view input,
                   const std::string &output, size_t line) {
  printf("normalization quick check failure on line %zu: check %s\n", line,
         what);
  printf("  input:  (%zu bytes): ", input.size());
  dump_hex_string(input);
  printf("  output: (%zu bytes): ", output.size());
  dump_hex_string(output);
  exit(1);
}

} // namespace

#define CHECK_NORM(what, expected, input, actual, line)                        \
  {                                                                            \
    const std::string_view _cn_expected = (expected);                          \
    const std::pair<std::string, bool> _cn_actual = (actual);                  \
    if (_cn_expected != _cn_actual.first) {                                    \
      report_mismatch(what, _cn_expected, _cn_actual.first, line);             \
    }                                                                          \
    /* Make sure that qc is set only if the input is equal to the output */    \
    if (_cn_actual.second && (input != _cn_actual.first)) {                    \
      report_bad_qc(what, input, _cn_actual.first, line);                      \
    }                                                                          \
  }

// Runs the NormalizationTest.txt conformance suite from the Unicode
// Character Database. For every test case (c1, c2, c3, c4, c5), the
// following invariants must hold:
//
//   NFC:  c2 == toNFC(c1) == toNFC(c2) == toNFC(c3)
//         c4 == toNFC(c4) == toNFC(c5)
//   NFD:  c3 == toNFD(c1) == toNFD(c2) == toNFD(c3)
//         c5 == toNFD(c4) == toNFD(c5)
//   NFKC: c4 == toNFKC(c1) == toNFKC(c2) == toNFKC(c3) == toNFKC(c4) ==
//         toNFKC(c5)
//   NFKD: c5 == toNFKD(c1) == toNFKD(c2) == toNFKD(c3) == toNFKD(c4) ==
//         toNFKD(c5)
TEST(conformance) {
  for (const auto &tc : simdutf::test::normalization_test_cases) {
    CHECK_NORM("NFC(c1) == c2", tc.c2, tc.c1, to_nfc(implementation, tc.c1),
               tc.line);
    CHECK_NORM("NFC(c2) == c2", tc.c2, tc.c2, to_nfc(implementation, tc.c2),
               tc.line);
    CHECK_NORM("NFC(c3) == c2", tc.c2, tc.c3, to_nfc(implementation, tc.c3),
               tc.line);
    CHECK_NORM("NFC(c4) == c4", tc.c4, tc.c4, to_nfc(implementation, tc.c4),
               tc.line);
    CHECK_NORM("NFC(c5) == c4", tc.c4, tc.c5, to_nfc(implementation, tc.c5),
               tc.line);

    CHECK_NORM("NFD(c1) == c3", tc.c3, tc.c1, to_nfd(implementation, tc.c1),
               tc.line);
    CHECK_NORM("NFD(c2) == c3", tc.c3, tc.c2, to_nfd(implementation, tc.c2),
               tc.line);
    CHECK_NORM("NFD(c3) == c3", tc.c3, tc.c3, to_nfd(implementation, tc.c3),
               tc.line);
    CHECK_NORM("NFD(c4) == c5", tc.c5, tc.c4, to_nfd(implementation, tc.c4),
               tc.line);
    CHECK_NORM("NFD(c5) == c5", tc.c5, tc.c5, to_nfd(implementation, tc.c5),
               tc.line);

    CHECK_NORM("NFKC(c1) == c4", tc.c4, tc.c1, to_nfkc(implementation, tc.c1),
               tc.line);
    CHECK_NORM("NFKC(c2) == c4", tc.c4, tc.c2, to_nfkc(implementation, tc.c2),
               tc.line);
    CHECK_NORM("NFKC(c3) == c4", tc.c4, tc.c3, to_nfkc(implementation, tc.c3),
               tc.line);
    CHECK_NORM("NFKC(c4) == c4", tc.c4, tc.c4, to_nfkc(implementation, tc.c4),
               tc.line);
    CHECK_NORM("NFKC(c5) == c4", tc.c4, tc.c5, to_nfkc(implementation, tc.c5),
               tc.line);

    CHECK_NORM("NFKD(c1) == c5", tc.c5, tc.c1, to_nfkd(implementation, tc.c1),
               tc.line);
    CHECK_NORM("NFKD(c2) == c5", tc.c5, tc.c2, to_nfkd(implementation, tc.c2),
               tc.line);
    CHECK_NORM("NFKD(c3) == c5", tc.c5, tc.c3, to_nfkd(implementation, tc.c3),
               tc.line);
    CHECK_NORM("NFKD(c4) == c5", tc.c5, tc.c4, to_nfkd(implementation, tc.c4),
               tc.line);
    CHECK_NORM("NFKD(c5) == c5", tc.c5, tc.c5, to_nfkd(implementation, tc.c5),
               tc.line);
  }
}

// Runs the same conformance suite as above, but with every field splated many
// times. This ensures that the vectorized code path is taken for the same
// normalization tests that scalar has to go through.
TEST(conformance_vectorized) {
  for (const auto &tc : simdutf::test::normalization_test_cases) {
    const std::string c1 = splat(tc.c1, 128);
    const std::string c2 = splat(tc.c2, 128);
    const std::string c3 = splat(tc.c3, 128);
    const std::string c4 = splat(tc.c4, 128);
    const std::string c5 = splat(tc.c5, 128);

    CHECK_NORM("splat NFC(c1) == c2", c2, c1, to_nfc(implementation, c1),
               tc.line);
    CHECK_NORM("splat NFC(c2) == c2", c2, c2, to_nfc(implementation, c2),
               tc.line);
    CHECK_NORM("splat NFC(c3) == c2", c2, c3, to_nfc(implementation, c3),
               tc.line);
    CHECK_NORM("splat NFC(c4) == c4", c4, c4, to_nfc(implementation, c4),
               tc.line);
    CHECK_NORM("splat NFC(c5) == c4", c4, c5, to_nfc(implementation, c5),
               tc.line);

    CHECK_NORM("splat NFD(c1) == c3", c3, c1, to_nfd(implementation, c1),
               tc.line);
    CHECK_NORM("splat NFD(c2) == c3", c3, c2, to_nfd(implementation, c2),
               tc.line);
    CHECK_NORM("splat NFD(c3) == c3", c3, c3, to_nfd(implementation, c3),
               tc.line);
    CHECK_NORM("splat NFD(c4) == c5", c5, c4, to_nfd(implementation, c4),
               tc.line);
    CHECK_NORM("splat NFD(c5) == c5", c5, c5, to_nfd(implementation, c5),
               tc.line);

    CHECK_NORM("splat NFKC(c1) == c4", c4, c1, to_nfkc(implementation, c1),
               tc.line);
    CHECK_NORM("splat NFKC(c2) == c4", c4, c2, to_nfkc(implementation, c2),
               tc.line);
    CHECK_NORM("splat NFKC(c3) == c4", c4, c3, to_nfkc(implementation, c3),
               tc.line);
    CHECK_NORM("splat NFKC(c4) == c4", c4, c4, to_nfkc(implementation, c4),
               tc.line);
    CHECK_NORM("splat NFKC(c5) == c4", c4, c5, to_nfkc(implementation, c5),
               tc.line);

    CHECK_NORM("splat NFKD(c1) == c5", c5, c1, to_nfkd(implementation, c1),
               tc.line);
    CHECK_NORM("splat NFKD(c2) == c5", c5, c2, to_nfkd(implementation, c2),
               tc.line);
    CHECK_NORM("splat NFKD(c3) == c5", c5, c3, to_nfkd(implementation, c3),
               tc.line);
    CHECK_NORM("splat NFKD(c4) == c5", c5, c4, to_nfkd(implementation, c4),
               tc.line);
    CHECK_NORM("splat NFKD(c5) == c5", c5, c5, to_nfkd(implementation, c5),
               tc.line);
  }
}

TEST_MAIN
