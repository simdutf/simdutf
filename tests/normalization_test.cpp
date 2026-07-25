#include "simdutf.h"

#include <tests/helpers/test.h>
#include <tests/normalization_test_data/normalization_test_data.h>

#include <random>

namespace {

template <simdutf::encoding_type encoding, typename T>
std::basic_string<T> splat(std::basic_string_view<T> sv, int n) {
  std::basic_string<T> result;
  std::basic_string_view<T> combining_grapheme_joiner;
  if constexpr (encoding == simdutf::encoding_type::UTF16_LE) {
    if constexpr (match_system(simdutf::endianness::LITTLE)) {
      combining_grapheme_joiner = u"\u034f";
    } else {
      combining_grapheme_joiner = u"\u4f03";
    }
  }
  if constexpr (encoding == simdutf::encoding_type::UTF16_BE) {
    if constexpr (match_system(simdutf::endianness::BIG)) {
      combining_grapheme_joiner = u"\u034f";
    } else {
      combining_grapheme_joiner = u"\u4f03";
    }
  }
  if constexpr (encoding == simdutf::encoding_type::UTF8) {
    combining_grapheme_joiner = "\u034f";
  }
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

std::u16string to_utf16le(const simdutf::implementation &impl,
                          std::string_view input) {
  size_t length = impl.utf16_length_from_utf8(input.data(), input.size());
  std::u16string output(length, 0);
  size_t written =
      impl.convert_utf8_to_utf16le(input.data(), input.size(), output.data());
  output.resize(written);
  return output;
}

std::u16string to_utf16be(const simdutf::implementation &impl,
                          std::string_view input) {
  size_t length = impl.utf16_length_from_utf8(input.data(), input.size());
  std::u16string output(length, 0);
  size_t written =
      impl.convert_utf8_to_utf16be(input.data(), input.size(), output.data());
  output.resize(written);
  return output;
}

#define NORMALIZATION_FUNCTION(form, encoding, string_type, string_view_type)  \
  std::pair<string_type, bool> to_##form##_##encoding(                         \
      const simdutf::implementation &impl, string_view_type input) {           \
    size_t output_length;                                                      \
    bool qc = impl.normalize_##encoding##_to_##form##_check(                   \
        input.data(), input.size(), &output_length);                           \
    string_type output(output_length, 0);                                      \
    size_t written = impl.normalize_##encoding##_to_##form(                    \
        input.data(), input.size(), output.data());                            \
    /* The buffer is sized to the bound and not one unit more, so that a       \
       sanitizer build catches an overrun. */                                  \
    if (written > output_length) {                                             \
      printf("normalize_" #encoding "_to_" #form " wrote %zu units, but the "  \
             "_check function promised at most %zu\n",                         \
             written, output_length);                                          \
      exit(1);                                                                 \
    }                                                                          \
    output.resize(written);                                                    \
    return std::make_pair(output, qc);                                         \
  }

NORMALIZATION_FUNCTION(nfc, utf8, std::string, std::string_view);
NORMALIZATION_FUNCTION(nfd, utf8, std::string, std::string_view);
NORMALIZATION_FUNCTION(nfkc, utf8, std::string, std::string_view);
NORMALIZATION_FUNCTION(nfkd, utf8, std::string, std::string_view);
NORMALIZATION_FUNCTION(nfc, utf16le, std::u16string, std::u16string_view);
NORMALIZATION_FUNCTION(nfd, utf16le, std::u16string, std::u16string_view);
NORMALIZATION_FUNCTION(nfkc, utf16le, std::u16string, std::u16string_view);
NORMALIZATION_FUNCTION(nfkd, utf16le, std::u16string, std::u16string_view);
NORMALIZATION_FUNCTION(nfc, utf16be, std::u16string, std::u16string_view);
NORMALIZATION_FUNCTION(nfd, utf16be, std::u16string, std::u16string_view);
NORMALIZATION_FUNCTION(nfkc, utf16be, std::u16string, std::u16string_view);
NORMALIZATION_FUNCTION(nfkd, utf16be, std::u16string, std::u16string_view);

template <typename T> void dump_hex_string(std::basic_string_view<T> s) {
  for (uint8_t b : s) {
    if constexpr (sizeof(T) == 1) {
      printf("%02x ", b);
    }
    if constexpr (sizeof(T) == 2) {
      printf("%04x ", b);
    }
  }
  printf("\n");
}

template <typename T>
void report_mismatch(const char *what, std::basic_string_view<T> expected,
                     std::basic_string_view<T> actual, size_t line) {
  printf("normalization conformance failure on line %zu: check "
         "%s\n",
         line, what);
  printf("  expected (%zu words): ", expected.size());
  dump_hex_string(expected);
  printf("  actual   (%zu words): ", actual.size());
  dump_hex_string(actual);
  exit(1);
}

template <typename T>
void report_bad_qc(const char *what, std::basic_string_view<T> input,
                   std::basic_string_view<T> output, size_t line) {
  printf("normalization quick check failure on line %zu: check %s\n", line,
         what);
  printf("  input:  (size %zu): ", input.size());
  dump_hex_string(input);
  printf("  output: (size %zu): ", output.size());
  dump_hex_string(output);
  exit(1);
}

// Builds a pseudo-random UTF-8 input out of the conformance corpus, with ASCII
// runs of random length in between so that every fragment lands at a different
// offset from one trial to the next.
std::string random_corpus(uint32_t seed, size_t target_bytes) {
  std::mt19937 gen(seed);
  std::uniform_int_distribution<size_t> pick(
      0, simdutf::test::normalization_test_cases.size() - 1);
  std::uniform_int_distribution<int> column(0, 4);
  std::uniform_int_distribution<int> ascii_run(0, 48);
  std::uniform_int_distribution<int> ascii_char(0x20, 0x7e);

  std::string out;
  out.reserve(target_bytes + 128);
  while (out.size() < target_bytes) {
    const auto &tc = simdutf::test::normalization_test_cases[pick(gen)];
    const std::string_view fragments[] = {tc.c1, tc.c2, tc.c3, tc.c4, tc.c5};
    out += fragments[column(gen)];
    const int run = ascii_run(gen);
    for (int i = 0; i < run; i++) {
      out += char(ascii_char(gen));
    }
  }
  return out;
}

// The inputs here reach tens of kilobytes, so we print a window around the
// first differing code unit rather than the whole string.
template <typename T>
void report_disagreement(const char *what, uint32_t seed, size_t input_units,
                         std::basic_string_view<T> expected,
                         std::basic_string_view<T> actual) {
  size_t at = 0;
  while (at < expected.size() && at < actual.size() &&
         expected[at] == actual[at]) {
    at++;
  }
  const size_t from = (at > 8) ? at - 8 : 0;
  printf("UTF-16 normalization disagrees with UTF-8: %s\n", what);
  printf("  seed %u, %zu input units, first difference at index %zu\n", seed,
         input_units, at);
  printf("  expected (%zu units, from index %zu): ", expected.size(), from);
  dump_hex_string(expected.substr(from, 24));
  printf("  actual   (%zu units, from index %zu): ", actual.size(), from);
  dump_hex_string(actual.substr(from, 24));
  exit(1);
}

template <typename T>
void check_agreement(const char *what, uint32_t seed,
                     std::basic_string_view<T> expected,
                     std::basic_string_view<T> input,
                     const std::pair<std::basic_string<T>, bool> &actual) {
  if (expected != actual.first) {
    report_disagreement(what, seed, input.size(), expected,
                        std::basic_string_view<T>(actual.first));
  }
  if (actual.second && (input != actual.first)) {
    report_disagreement(what, seed, input.size(), input,
                        std::basic_string_view<T>(actual.first));
  }
}

std::string combining_run_utf8(size_t n) {
  std::string result;
  result.reserve(n * 2);
  for (size_t i = 0; i < n; i++) {
    result += "\u0301";
  }
  return result;
}

using find_first_utf8_fn = const char *(*)(const char *, size_t);
using find_last_utf8_fn = const char *(*)(const char *, size_t);
void check_find_stable_utf8(find_first_utf8_fn find_first,
                            find_last_utf8_fn find_last) {
  const std::string prefix = combining_run_utf8(5);
  const std::string s = prefix + "A" + combining_run_utf8(5);
  ASSERT_EQUAL(find_first(s.data(), s.size()), s.data() + prefix.size());

  const std::string all_combining = combining_run_utf8(50);
  ASSERT_EQUAL(find_first(all_combining.data(), all_combining.size()),
               all_combining.data() + all_combining.size());
  ASSERT_EQUAL(find_last(all_combining.data(), all_combining.size()),
               all_combining.data() + all_combining.size());

  const std::string starts_stable = "A" + combining_run_utf8(5);
  ASSERT_EQUAL(find_last(starts_stable.data(), starts_stable.size()),
               starts_stable.data());
}

using to_utf16_fn = std::u16string (*)(const simdutf::implementation &,
                                       std::string_view);
using find_first_utf16_fn = const char16_t *(*)(const char16_t *, size_t);
using find_last_utf16_fn = const char16_t *(*)(const char16_t *, size_t);
void check_find_stable_utf16(const simdutf::implementation &impl,
                             to_utf16_fn to_utf16,
                             find_first_utf16_fn find_first,
                             find_last_utf16_fn find_last) {
  const std::string prefix_u8 = combining_run_utf8(5);
  const std::u16string prefix = to_utf16(impl, prefix_u8);
  const std::u16string s =
      to_utf16(impl, prefix_u8 + "A" + combining_run_utf8(5));
  ASSERT_EQUAL(find_first(s.data(), s.size()), s.data() + prefix.size());

  const std::u16string all_combining = to_utf16(impl, combining_run_utf8(50));
  ASSERT_EQUAL(find_first(all_combining.data(), all_combining.size()),
               all_combining.data() + all_combining.size());
  ASSERT_EQUAL(find_last(all_combining.data(), all_combining.size()),
               all_combining.data() + all_combining.size());

  const std::u16string starts_stable =
      to_utf16(impl, "A" + combining_run_utf8(5));
  ASSERT_EQUAL(find_last(starts_stable.data(), starts_stable.size()),
               starts_stable.data());
}

} // namespace

template <typename T>
void check_norm(const char *what, std::basic_string_view<T> expected,
                std::basic_string_view<T> input,
                const std::pair<std::basic_string<T>, bool> &actual,
                size_t line) {
  if (expected != actual.first) {
    report_mismatch(what, expected, std::basic_string_view<T>(actual.first),
                    line);
  }
  if (actual.second && (input != actual.first)) {
    report_bad_qc(what, input, std::basic_string_view<T>(actual.first), line);
  }
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
TEST(conformance_utf8) {
  for (const auto &tc : simdutf::test::normalization_test_cases) {
    check_norm("NFC(c1) == c2", tc.c2, tc.c1,
               to_nfc_utf8(implementation, tc.c1), tc.line);
    check_norm("NFC(c2) == c2", tc.c2, tc.c2,
               to_nfc_utf8(implementation, tc.c2), tc.line);
    check_norm("NFC(c3) == c2", tc.c2, tc.c3,
               to_nfc_utf8(implementation, tc.c3), tc.line);
    check_norm("NFC(c4) == c4", tc.c4, tc.c4,
               to_nfc_utf8(implementation, tc.c4), tc.line);
    check_norm("NFC(c5) == c4", tc.c4, tc.c5,
               to_nfc_utf8(implementation, tc.c5), tc.line);

    check_norm("NFD(c1) == c3", tc.c3, tc.c1,
               to_nfd_utf8(implementation, tc.c1), tc.line);
    check_norm("NFD(c2) == c3", tc.c3, tc.c2,
               to_nfd_utf8(implementation, tc.c2), tc.line);
    check_norm("NFD(c3) == c3", tc.c3, tc.c3,
               to_nfd_utf8(implementation, tc.c3), tc.line);
    check_norm("NFD(c4) == c5", tc.c5, tc.c4,
               to_nfd_utf8(implementation, tc.c4), tc.line);
    check_norm("NFD(c5) == c5", tc.c5, tc.c5,
               to_nfd_utf8(implementation, tc.c5), tc.line);

    check_norm("NFKC(c1) == c4", tc.c4, tc.c1,
               to_nfkc_utf8(implementation, tc.c1), tc.line);
    check_norm("NFKC(c2) == c4", tc.c4, tc.c2,
               to_nfkc_utf8(implementation, tc.c2), tc.line);
    check_norm("NFKC(c3) == c4", tc.c4, tc.c3,
               to_nfkc_utf8(implementation, tc.c3), tc.line);
    check_norm("NFKC(c4) == c4", tc.c4, tc.c4,
               to_nfkc_utf8(implementation, tc.c4), tc.line);
    check_norm("NFKC(c5) == c4", tc.c4, tc.c5,
               to_nfkc_utf8(implementation, tc.c5), tc.line);

    check_norm("NFKD(c1) == c5", tc.c5, tc.c1,
               to_nfkd_utf8(implementation, tc.c1), tc.line);
    check_norm("NFKD(c2) == c5", tc.c5, tc.c2,
               to_nfkd_utf8(implementation, tc.c2), tc.line);
    check_norm("NFKD(c3) == c5", tc.c5, tc.c3,
               to_nfkd_utf8(implementation, tc.c3), tc.line);
    check_norm("NFKD(c4) == c5", tc.c5, tc.c4,
               to_nfkd_utf8(implementation, tc.c4), tc.line);
    check_norm("NFKD(c5) == c5", tc.c5, tc.c5,
               to_nfkd_utf8(implementation, tc.c5), tc.line);
  }
}

// Runs the same conformance suite as above, but with every field splated many
// times. This ensures that the vectorized code path is taken for the same
// normalization tests that scalar has to go through.
TEST(conformance_vectorized_utf8) {
  for (const auto &tc : simdutf::test::normalization_test_cases) {
    const std::string c1 = splat<simdutf::encoding_type::UTF8>(tc.c1, 128);
    const std::string c2 = splat<simdutf::encoding_type::UTF8>(tc.c2, 128);
    const std::string c3 = splat<simdutf::encoding_type::UTF8>(tc.c3, 128);
    const std::string c4 = splat<simdutf::encoding_type::UTF8>(tc.c4, 128);
    const std::string c5 = splat<simdutf::encoding_type::UTF8>(tc.c5, 128);

    check_norm("NFC(splat(c1)) == splat(c2)", std::string_view(c2),
               std::string_view(c1), to_nfc_utf8(implementation, c1), tc.line);
    check_norm("NFC(splat(c2)) == splat(c2)", std::string_view(c2),
               std::string_view(c2), to_nfc_utf8(implementation, c2), tc.line);
    check_norm("NFC(splat(c3)) == splat(c2)", std::string_view(c2),
               std::string_view(c3), to_nfc_utf8(implementation, c3), tc.line);
    check_norm("NFC(splat(c4)) == splat(c4)", std::string_view(c4),
               std::string_view(c4), to_nfc_utf8(implementation, c4), tc.line);
    check_norm("NFC(splat(c5)) == splat(c4)", std::string_view(c4),
               std::string_view(c5), to_nfc_utf8(implementation, c5), tc.line);

    check_norm("NFD(splat(c1)) == splat(c3)", std::string_view(c3),
               std::string_view(c1), to_nfd_utf8(implementation, c1), tc.line);
    check_norm("NFD(splat(c2)) == splat(c3)", std::string_view(c3),
               std::string_view(c2), to_nfd_utf8(implementation, c2), tc.line);
    check_norm("NFD(splat(c3)) == splat(c3)", std::string_view(c3),
               std::string_view(c3), to_nfd_utf8(implementation, c3), tc.line);
    check_norm("NFD(splat(c4)) == splat(c5)", std::string_view(c5),
               std::string_view(c4), to_nfd_utf8(implementation, c4), tc.line);
    check_norm("NFD(splat(c5)) == splat(c5)", std::string_view(c5),
               std::string_view(c5), to_nfd_utf8(implementation, c5), tc.line);

    check_norm("NFKC(splat(c1)) == splat(c4)", std::string_view(c4),
               std::string_view(c1), to_nfkc_utf8(implementation, c1), tc.line);
    check_norm("NFKC(splat(c2)) == splat(c4)", std::string_view(c4),
               std::string_view(c2), to_nfkc_utf8(implementation, c2), tc.line);
    check_norm("NFKC(splat(c3)) == splat(c4)", std::string_view(c4),
               std::string_view(c3), to_nfkc_utf8(implementation, c3), tc.line);
    check_norm("NFKC(splat(c4)) == splat(c4)", std::string_view(c4),
               std::string_view(c4), to_nfkc_utf8(implementation, c4), tc.line);
    check_norm("NFKC(splat(c5)) == splat(c4)", std::string_view(c4),
               std::string_view(c5), to_nfkc_utf8(implementation, c5), tc.line);

    check_norm("NFKD(splat(c1)) == splat(c5)", std::string_view(c5),
               std::string_view(c1), to_nfkd_utf8(implementation, c1), tc.line);
    check_norm("NFKD(splat(c2)) == splat(c5)", std::string_view(c5),
               std::string_view(c2), to_nfkd_utf8(implementation, c2), tc.line);
    check_norm("NFKD(splat(c3)) == splat(c5)", std::string_view(c5),
               std::string_view(c3), to_nfkd_utf8(implementation, c3), tc.line);
    check_norm("NFKD(splat(c4)) == splat(c5)", std::string_view(c5),
               std::string_view(c4), to_nfkd_utf8(implementation, c4), tc.line);
    check_norm("NFKD(splat(c5)) == splat(c5)", std::string_view(c5),
               std::string_view(c5), to_nfkd_utf8(implementation, c5), tc.line);
  }
}

TEST(conformance_utf16le) {
  for (const auto &tc : simdutf::test::normalization_test_cases) {
    const std::u16string c1 = to_utf16le(implementation, tc.c1);
    const std::u16string c2 = to_utf16le(implementation, tc.c2);
    const std::u16string c3 = to_utf16le(implementation, tc.c3);
    const std::u16string c4 = to_utf16le(implementation, tc.c4);
    const std::u16string c5 = to_utf16le(implementation, tc.c5);

    check_norm("NFC(utf16le(c1)) == utf16le(c2)", std::u16string_view(c2),
               std::u16string_view(c1), to_nfc_utf16le(implementation, c1),
               tc.line);
    check_norm("NFC(utf16le(c2)) == utf16le(c2)", std::u16string_view(c2),
               std::u16string_view(c2), to_nfc_utf16le(implementation, c2),
               tc.line);
    check_norm("NFC(utf16le(c3)) == utf16le(c2)", std::u16string_view(c2),
               std::u16string_view(c3), to_nfc_utf16le(implementation, c3),
               tc.line);
    check_norm("NFC(utf16le(c4)) == utf16le(c4)", std::u16string_view(c4),
               std::u16string_view(c4), to_nfc_utf16le(implementation, c4),
               tc.line);
    check_norm("NFC(utf16le(c5)) == utf16le(c4)", std::u16string_view(c4),
               std::u16string_view(c5), to_nfc_utf16le(implementation, c5),
               tc.line);

    check_norm("NFD(utf16le(c1)) == utf16le(c3)", std::u16string_view(c3),
               std::u16string_view(c1), to_nfd_utf16le(implementation, c1),
               tc.line);
    check_norm("NFD(utf16le(c2)) == utf16le(c3)", std::u16string_view(c3),
               std::u16string_view(c2), to_nfd_utf16le(implementation, c2),
               tc.line);
    check_norm("NFD(utf16le(c3)) == utf16le(c3)", std::u16string_view(c3),
               std::u16string_view(c3), to_nfd_utf16le(implementation, c3),
               tc.line);
    check_norm("NFD(utf16le(c4)) == utf16le(c5)", std::u16string_view(c5),
               std::u16string_view(c4), to_nfd_utf16le(implementation, c4),
               tc.line);
    check_norm("NFD(utf16le(c5)) == utf16le(c5)", std::u16string_view(c5),
               std::u16string_view(c5), to_nfd_utf16le(implementation, c5),
               tc.line);

    check_norm("NFKC(utf16le(c1)) == utf16le(c4)", std::u16string_view(c4),
               std::u16string_view(c1), to_nfkc_utf16le(implementation, c1),
               tc.line);
    check_norm("NFKC(utf16le(c2)) == utf16le(c4)", std::u16string_view(c4),
               std::u16string_view(c2), to_nfkc_utf16le(implementation, c2),
               tc.line);
    check_norm("NFKC(utf16le(c3)) == utf16le(c4)", std::u16string_view(c4),
               std::u16string_view(c3), to_nfkc_utf16le(implementation, c3),
               tc.line);
    check_norm("NFKC(utf16le(c4)) == utf16le(c4)", std::u16string_view(c4),
               std::u16string_view(c4), to_nfkc_utf16le(implementation, c4),
               tc.line);
    check_norm("NFKC(utf16le(c5)) == utf16le(c4)", std::u16string_view(c4),
               std::u16string_view(c5), to_nfkc_utf16le(implementation, c5),
               tc.line);

    check_norm("NFKD(utf16le(c1)) == utf16le(c5)", std::u16string_view(c5),
               std::u16string_view(c1), to_nfkd_utf16le(implementation, c1),
               tc.line);
    check_norm("NFKD(utf16le(c2)) == utf16le(c5)", std::u16string_view(c5),
               std::u16string_view(c2), to_nfkd_utf16le(implementation, c2),
               tc.line);
    check_norm("NFKD(utf16le(c3)) == utf16le(c5)", std::u16string_view(c5),
               std::u16string_view(c3), to_nfkd_utf16le(implementation, c3),
               tc.line);
    check_norm("NFKD(utf16le(c4)) == utf16le(c5)", std::u16string_view(c5),
               std::u16string_view(c4), to_nfkd_utf16le(implementation, c4),
               tc.line);
    check_norm("NFKD(utf16le(c5)) == utf16le(c5)", std::u16string_view(c5),
               std::u16string_view(c5), to_nfkd_utf16le(implementation, c5),
               tc.line);
  }
}

TEST(conformance_utf16le_vectorized) {
  for (const auto &tc : simdutf::test::normalization_test_cases) {
    const std::u16string c1 = splat<simdutf::encoding_type::UTF16_LE>(
        std::u16string_view(to_utf16le(implementation, tc.c1)), 128);
    const std::u16string c2 = splat<simdutf::encoding_type::UTF16_LE>(
        std::u16string_view(to_utf16le(implementation, tc.c2)), 128);
    const std::u16string c3 = splat<simdutf::encoding_type::UTF16_LE>(
        std::u16string_view(to_utf16le(implementation, tc.c3)), 128);
    const std::u16string c4 = splat<simdutf::encoding_type::UTF16_LE>(
        std::u16string_view(to_utf16le(implementation, tc.c4)), 128);
    const std::u16string c5 = splat<simdutf::encoding_type::UTF16_LE>(
        std::u16string_view(to_utf16le(implementation, tc.c5)), 128);

    check_norm("NFC(splat(utf16le(c1))) == splat(utf16le(c2))",
               std::u16string_view(c2), std::u16string_view(c1),
               to_nfc_utf16le(implementation, c1), tc.line);
    check_norm("NFC(splat(utf16le(c2))) == splat(utf16le(c2))",
               std::u16string_view(c2), std::u16string_view(c2),
               to_nfc_utf16le(implementation, c2), tc.line);
    check_norm("NFC(splat(utf16le(c3))) == splat(utf16le(c2))",
               std::u16string_view(c2), std::u16string_view(c3),
               to_nfc_utf16le(implementation, c3), tc.line);
    check_norm("NFC(splat(utf16le(c4))) == splat(utf16le(c4))",
               std::u16string_view(c4), std::u16string_view(c4),
               to_nfc_utf16le(implementation, c4), tc.line);
    check_norm("NFC(splat(utf16le(c5))) == splat(utf16le(c4))",
               std::u16string_view(c4), std::u16string_view(c5),
               to_nfc_utf16le(implementation, c5), tc.line);

    check_norm("NFD(splat(utf16le(c1))) == splat(utf16le(c3))",
               std::u16string_view(c3), std::u16string_view(c1),
               to_nfd_utf16le(implementation, c1), tc.line);
    check_norm("NFD(splat(utf16le(c2))) == splat(utf16le(c3))",
               std::u16string_view(c3), std::u16string_view(c2),
               to_nfd_utf16le(implementation, c2), tc.line);
    check_norm("NFD(splat(utf16le(c3))) == splat(utf16le(c3))",
               std::u16string_view(c3), std::u16string_view(c3),
               to_nfd_utf16le(implementation, c3), tc.line);
    check_norm("NFD(splat(utf16le(c4))) == splat(utf16le(c5))",
               std::u16string_view(c5), std::u16string_view(c4),
               to_nfd_utf16le(implementation, c4), tc.line);
    check_norm("NFD(splat(utf16le(c5))) == splat(utf16le(c5))",
               std::u16string_view(c5), std::u16string_view(c5),
               to_nfd_utf16le(implementation, c5), tc.line);

    check_norm("NFKC(splat(utf16le(c1))) == splat(utf16le(c4))",
               std::u16string_view(c4), std::u16string_view(c1),
               to_nfkc_utf16le(implementation, c1), tc.line);
    check_norm("NFKC(splat(utf16le(c2))) == splat(utf16le(c4))",
               std::u16string_view(c4), std::u16string_view(c2),
               to_nfkc_utf16le(implementation, c2), tc.line);
    check_norm("NFKC(splat(utf16le(c3))) == splat(utf16le(c4))",
               std::u16string_view(c4), std::u16string_view(c3),
               to_nfkc_utf16le(implementation, c3), tc.line);
    check_norm("NFKC(splat(utf16le(c4))) == splat(utf16le(c4))",
               std::u16string_view(c4), std::u16string_view(c4),
               to_nfkc_utf16le(implementation, c4), tc.line);
    check_norm("NFKC(splat(utf16le(c5))) == splat(utf16le(c4))",
               std::u16string_view(c4), std::u16string_view(c5),
               to_nfkc_utf16le(implementation, c5), tc.line);

    check_norm("NFKD(splat(utf16le(c1))) == splat(utf16le(c5))",
               std::u16string_view(c5), std::u16string_view(c1),
               to_nfkd_utf16le(implementation, c1), tc.line);
    check_norm("NFKD(splat(utf16le(c2))) == splat(utf16le(c5))",
               std::u16string_view(c5), std::u16string_view(c2),
               to_nfkd_utf16le(implementation, c2), tc.line);
    check_norm("NFKD(splat(utf16le(c3))) == splat(utf16le(c5))",
               std::u16string_view(c5), std::u16string_view(c3),
               to_nfkd_utf16le(implementation, c3), tc.line);
    check_norm("NFKD(splat(utf16le(c4))) == splat(utf16le(c5))",
               std::u16string_view(c5), std::u16string_view(c4),
               to_nfkd_utf16le(implementation, c4), tc.line);
    check_norm("NFKD(splat(utf16le(c5))) == splat(utf16le(c5))",
               std::u16string_view(c5), std::u16string_view(c5),
               to_nfkd_utf16le(implementation, c5), tc.line);
  }
}

TEST(conformance_utf16be) {
  for (const auto &tc : simdutf::test::normalization_test_cases) {
    const std::u16string c1 = to_utf16be(implementation, tc.c1);
    const std::u16string c2 = to_utf16be(implementation, tc.c2);
    const std::u16string c3 = to_utf16be(implementation, tc.c3);
    const std::u16string c4 = to_utf16be(implementation, tc.c4);
    const std::u16string c5 = to_utf16be(implementation, tc.c5);

    check_norm("NFC(utf16be(c1)) == utf16be(c2)", std::u16string_view(c2),
               std::u16string_view(c1), to_nfc_utf16be(implementation, c1),
               tc.line);
    check_norm("NFC(utf16be(c2)) == utf16be(c2)", std::u16string_view(c2),
               std::u16string_view(c2), to_nfc_utf16be(implementation, c2),
               tc.line);
    check_norm("NFC(utf16be(c3)) == utf16be(c2)", std::u16string_view(c2),
               std::u16string_view(c3), to_nfc_utf16be(implementation, c3),
               tc.line);
    check_norm("NFC(utf16be(c4)) == utf16be(c4)", std::u16string_view(c4),
               std::u16string_view(c4), to_nfc_utf16be(implementation, c4),
               tc.line);
    check_norm("NFC(utf16be(c5)) == utf16be(c4)", std::u16string_view(c4),
               std::u16string_view(c5), to_nfc_utf16be(implementation, c5),
               tc.line);

    check_norm("NFD(utf16be(c1)) == utf16be(c3)", std::u16string_view(c3),
               std::u16string_view(c1), to_nfd_utf16be(implementation, c1),
               tc.line);
    check_norm("NFD(utf16be(c2)) == utf16be(c3)", std::u16string_view(c3),
               std::u16string_view(c2), to_nfd_utf16be(implementation, c2),
               tc.line);
    check_norm("NFD(utf16be(c3)) == utf16be(c3)", std::u16string_view(c3),
               std::u16string_view(c3), to_nfd_utf16be(implementation, c3),
               tc.line);
    check_norm("NFD(utf16be(c4)) == utf16be(c5)", std::u16string_view(c5),
               std::u16string_view(c4), to_nfd_utf16be(implementation, c4),
               tc.line);
    check_norm("NFD(utf16be(c5)) == utf16be(c5)", std::u16string_view(c5),
               std::u16string_view(c5), to_nfd_utf16be(implementation, c5),
               tc.line);

    check_norm("NFKC(utf16be(c1)) == utf16be(c4)", std::u16string_view(c4),
               std::u16string_view(c1), to_nfkc_utf16be(implementation, c1),
               tc.line);
    check_norm("NFKC(utf16be(c2)) == utf16be(c4)", std::u16string_view(c4),
               std::u16string_view(c2), to_nfkc_utf16be(implementation, c2),
               tc.line);
    check_norm("NFKC(utf16be(c3)) == utf16be(c4)", std::u16string_view(c4),
               std::u16string_view(c3), to_nfkc_utf16be(implementation, c3),
               tc.line);
    check_norm("NFKC(utf16be(c4)) == utf16be(c4)", std::u16string_view(c4),
               std::u16string_view(c4), to_nfkc_utf16be(implementation, c4),
               tc.line);
    check_norm("NFKC(utf16be(c5)) == utf16be(c4)", std::u16string_view(c4),
               std::u16string_view(c5), to_nfkc_utf16be(implementation, c5),
               tc.line);

    check_norm("NFKD(utf16be(c1)) == utf16be(c5)", std::u16string_view(c5),
               std::u16string_view(c1), to_nfkd_utf16be(implementation, c1),
               tc.line);
    check_norm("NFKD(utf16be(c2)) == utf16be(c5)", std::u16string_view(c5),
               std::u16string_view(c2), to_nfkd_utf16be(implementation, c2),
               tc.line);
    check_norm("NFKD(utf16be(c3)) == utf16be(c5)", std::u16string_view(c5),
               std::u16string_view(c3), to_nfkd_utf16be(implementation, c3),
               tc.line);
    check_norm("NFKD(utf16be(c4)) == utf16be(c5)", std::u16string_view(c5),
               std::u16string_view(c4), to_nfkd_utf16be(implementation, c4),
               tc.line);
    check_norm("NFKD(utf16be(c5)) == utf16be(c5)", std::u16string_view(c5),
               std::u16string_view(c5), to_nfkd_utf16be(implementation, c5),
               tc.line);
  }
}

TEST(conformance_utf16be_vectorized) {
  for (const auto &tc : simdutf::test::normalization_test_cases) {
    const std::u16string c1 = splat<simdutf::encoding_type::UTF16_BE>(
        std::u16string_view(to_utf16be(implementation, tc.c1)), 128);
    const std::u16string c2 = splat<simdutf::encoding_type::UTF16_BE>(
        std::u16string_view(to_utf16be(implementation, tc.c2)), 128);
    const std::u16string c3 = splat<simdutf::encoding_type::UTF16_BE>(
        std::u16string_view(to_utf16be(implementation, tc.c3)), 128);
    const std::u16string c4 = splat<simdutf::encoding_type::UTF16_BE>(
        std::u16string_view(to_utf16be(implementation, tc.c4)), 128);
    const std::u16string c5 = splat<simdutf::encoding_type::UTF16_BE>(
        std::u16string_view(to_utf16be(implementation, tc.c5)), 128);

    check_norm("NFC(splat(utf16be(c1))) == splat(utf16be(c2))",
               std::u16string_view(c2), std::u16string_view(c1),
               to_nfc_utf16be(implementation, c1), tc.line);
    check_norm("NFC(splat(utf16be(c2))) == splat(utf16be(c2))",
               std::u16string_view(c2), std::u16string_view(c2),
               to_nfc_utf16be(implementation, c2), tc.line);
    check_norm("NFC(splat(utf16be(c3))) == splat(utf16be(c2))",
               std::u16string_view(c2), std::u16string_view(c3),
               to_nfc_utf16be(implementation, c3), tc.line);
    check_norm("NFC(splat(utf16be(c4))) == splat(utf16be(c4))",
               std::u16string_view(c4), std::u16string_view(c4),
               to_nfc_utf16be(implementation, c4), tc.line);
    check_norm("NFC(splat(utf16be(c5))) == splat(utf16be(c4))",
               std::u16string_view(c4), std::u16string_view(c5),
               to_nfc_utf16be(implementation, c5), tc.line);

    check_norm("NFD(splat(utf16be(c1))) == splat(utf16be(c3))",
               std::u16string_view(c3), std::u16string_view(c1),
               to_nfd_utf16be(implementation, c1), tc.line);
    check_norm("NFD(splat(utf16be(c2))) == splat(utf16be(c3))",
               std::u16string_view(c3), std::u16string_view(c2),
               to_nfd_utf16be(implementation, c2), tc.line);
    check_norm("NFD(splat(utf16be(c3))) == splat(utf16be(c3))",
               std::u16string_view(c3), std::u16string_view(c3),
               to_nfd_utf16be(implementation, c3), tc.line);
    check_norm("NFD(splat(utf16be(c4))) == splat(utf16be(c5))",
               std::u16string_view(c5), std::u16string_view(c4),
               to_nfd_utf16be(implementation, c4), tc.line);
    check_norm("NFD(splat(utf16be(c5))) == splat(utf16be(c5))",
               std::u16string_view(c5), std::u16string_view(c5),
               to_nfd_utf16be(implementation, c5), tc.line);

    check_norm("NFKC(splat(utf16be(c1))) == splat(utf16be(c4))",
               std::u16string_view(c4), std::u16string_view(c1),
               to_nfkc_utf16be(implementation, c1), tc.line);
    check_norm("NFKC(splat(utf16be(c2))) == splat(utf16be(c4))",
               std::u16string_view(c4), std::u16string_view(c2),
               to_nfkc_utf16be(implementation, c2), tc.line);
    check_norm("NFKC(splat(utf16be(c3))) == splat(utf16be(c4))",
               std::u16string_view(c4), std::u16string_view(c3),
               to_nfkc_utf16be(implementation, c3), tc.line);
    check_norm("NFKC(splat(utf16be(c4))) == splat(utf16be(c4))",
               std::u16string_view(c4), std::u16string_view(c4),
               to_nfkc_utf16be(implementation, c4), tc.line);
    check_norm("NFKC(splat(utf16be(c5))) == splat(utf16be(c4))",
               std::u16string_view(c4), std::u16string_view(c5),
               to_nfkc_utf16be(implementation, c5), tc.line);

    check_norm("NFKD(splat(utf16be(c1))) == splat(utf16be(c5))",
               std::u16string_view(c5), std::u16string_view(c1),
               to_nfkd_utf16be(implementation, c1), tc.line);
    check_norm("NFKD(splat(utf16be(c2))) == splat(utf16be(c5))",
               std::u16string_view(c5), std::u16string_view(c2),
               to_nfkd_utf16be(implementation, c2), tc.line);
    check_norm("NFKD(splat(utf16be(c3))) == splat(utf16be(c5))",
               std::u16string_view(c5), std::u16string_view(c3),
               to_nfkd_utf16be(implementation, c3), tc.line);
    check_norm("NFKD(splat(utf16be(c4))) == splat(utf16be(c5))",
               std::u16string_view(c5), std::u16string_view(c4),
               to_nfkd_utf16be(implementation, c4), tc.line);
    check_norm("NFKD(splat(utf16be(c5))) == splat(utf16be(c5))",
               std::u16string_view(c5), std::u16string_view(c5),
               to_nfkd_utf16be(implementation, c5), tc.line);
  }
}

#define CHECK_AGREEMENT(form)                                                  \
  {                                                                            \
    const std::string expected_utf8 =                                          \
        to_##form##_utf8(implementation, utf8).first;                          \
    check_agreement(                                                           \
        #form "(utf16le(x)) == utf16le(" #form "(x))", seed,                   \
        std::u16string_view(to_utf16le(implementation, expected_utf8)),        \
        std::u16string_view(le), to_##form##_utf16le(implementation, le));     \
    check_agreement(                                                           \
        #form "(utf16be(x)) == utf16be(" #form "(x))", seed,                   \
        std::u16string_view(to_utf16be(implementation, expected_utf8)),        \
        std::u16string_view(be), to_##form##_utf16be(implementation, be));     \
  }

// The conformance tests above pin the UTF-8 code path to NormalizationTest.txt.
// This one makes the UTF-16 paths ride on that guarantee: for the same text,
// normalizing in UTF-16 must give exactly the transcoding of the UTF-8 answer.
// Unlike the conformance tests, the inputs are long and their interesting
// characters sit at arbitrary offsets, so the vectorized kernels' block
// boundaries and scalar tails are exercised at every alignment.
TEST(utf16_agrees_with_utf8) {
  const size_t sizes[] = {0,   1,   2,   3,   7,    16,   31,  32,
                          33,  63,  64,  65,  127,  128,  129, 255,
                          256, 511, 512, 997, 4096, 20011};
  uint32_t seed = 0;
  for (const size_t target : sizes) {
    for (int trial = 0; trial < 8; trial++, seed++) {
      const std::string utf8 = random_corpus(seed, target);
      const std::u16string le = to_utf16le(implementation, utf8);
      const std::u16string be = to_utf16be(implementation, utf8);

      CHECK_AGREEMENT(nfc);
      CHECK_AGREEMENT(nfd);
      CHECK_AGREEMENT(nfkc);
      CHECK_AGREEMENT(nfkd);
    }
  }
}

#undef CHECK_AGREEMENT

// When the vectorized NF(K)C path meets a 12-byte chunk holding three
// 1..4-byte code points, it hands the chunk to
// scalar::utf8_to_composed::normalize_with_context, which normalizes the whole
// surrounding combining sequence and returns how many input bytes it swallowed.
// That count is bounded only by the distance to the next stable starter, so it
// can reach past the end of the current 64-byte block, and the caller's
// `mask >>= consumed` then shifts a uint64_t by >= 64: undefined behaviour.
// (On x86-64 and AArch64 a shift by exactly 64 is a no-op rather than zero.)
//
// Each input below is three U+1D15E (4 bytes each, and none of them stable for
// NF(K)C) followed by `marks` combining acute accents (2 bytes each) and ASCII
// filler, so the fallback consumes 12 + 2 * marks bytes: marks == 26 lands on
// consumed == 64 exactly, and the loop above sweeps consumed from 52 to 92.
//
// The normalized output is correct either way -- the shifted mask is dead, as
// the inner loop always exits after such a chunk -- so this test is here for
// the sanitizer builds: it fails under -fsanitize=undefined until the shift is
// guarded.
TEST(composed_scalar_fallback_consumes_past_block) {
  for (int marks = 20; marks <= 40; marks++) {
    std::string input;
    std::string expected;
    for (int i = 0; i < 3; i++) {
      input += "\U0001D15E";              // MUSICAL SYMBOL HALF NOTE
      expected += "\U0001D157\U0001D165"; // stays decomposed: excluded from
                                          // composition
    }
    for (int i = 0; i < marks; i++) {
      input += "\u0300"; // COMBINING GRAVE ACCENT, ccc 230
      expected += "\u0300";
    }
    // The vectorized loop only runs while 64 + 64 bytes remain, so keep the
    // interesting region well inside it.
    input.append(256, 'a');
    expected.append(256, 'a');

    char what[96];
    snprintf(what, sizeof(what), "NFC, %d marks (fallback consumes %d bytes)",
             marks, 12 + 2 * marks);
    check_norm(what, std::string_view(expected), std::string_view(input),
               to_nfc_utf8(implementation, input), size_t(__LINE__));
    snprintf(what, sizeof(what), "NFKC, %d marks (fallback consumes %d bytes)",
             marks, 12 + 2 * marks);
    check_norm(what, std::string_view(expected), std::string_view(input),
               to_nfkc_utf8(implementation, input), size_t(__LINE__));
  }
}

TEST(find_stable_utf8_nfd) {
  check_find_stable_utf8(simdutf::find_first_stable_utf8_nfd,
                         simdutf::find_last_stable_utf8_nfd);
}

TEST(find_stable_utf8_nfkd) {
  check_find_stable_utf8(simdutf::find_first_stable_utf8_nfkd,
                         simdutf::find_last_stable_utf8_nfkd);
}

TEST(find_stable_utf16le_nfd) {
  check_find_stable_utf16(implementation, to_utf16le,
                          simdutf::find_first_stable_utf16le_nfd,
                          simdutf::find_last_stable_utf16le_nfd);
}

TEST(find_stable_utf16be_nfd) {
  check_find_stable_utf16(implementation, to_utf16be,
                          simdutf::find_first_stable_utf16be_nfd,
                          simdutf::find_last_stable_utf16be_nfd);
}

TEST(find_stable_utf16le_nfkd) {
  check_find_stable_utf16(implementation, to_utf16le,
                          simdutf::find_first_stable_utf16le_nfkd,
                          simdutf::find_last_stable_utf16le_nfkd);
}

TEST(find_stable_utf16be_nfkd) {
  check_find_stable_utf16(implementation, to_utf16be,
                          simdutf::find_first_stable_utf16be_nfkd,
                          simdutf::find_last_stable_utf16be_nfkd);
}

TEST(find_stable_utf8_nfc) {
  check_find_stable_utf8(simdutf::find_first_stable_utf8_nfc,
                         simdutf::find_last_stable_utf8_nfc);
}

TEST(find_stable_utf8_nfkc) {
  check_find_stable_utf8(simdutf::find_first_stable_utf8_nfkc,
                         simdutf::find_last_stable_utf8_nfkc);
}

TEST(find_stable_utf16le_nfc) {
  check_find_stable_utf16(implementation, to_utf16le,
                          simdutf::find_first_stable_utf16le_nfc,
                          simdutf::find_last_stable_utf16le_nfc);
}

TEST(find_stable_utf16be_nfc) {
  check_find_stable_utf16(implementation, to_utf16be,
                          simdutf::find_first_stable_utf16be_nfc,
                          simdutf::find_last_stable_utf16be_nfc);
}

TEST(find_stable_utf16le_nfkc) {
  check_find_stable_utf16(implementation, to_utf16le,
                          simdutf::find_first_stable_utf16le_nfkc,
                          simdutf::find_last_stable_utf16le_nfkc);
}

TEST(find_stable_utf16be_nfkc) {
  check_find_stable_utf16(implementation, to_utf16be,
                          simdutf::find_first_stable_utf16be_nfkc,
                          simdutf::find_last_stable_utf16be_nfkc);
}

TEST(find_stable_utf16_le_be_agree) {
  const std::string s_u8 = combining_run_utf8(5) + "A" + combining_run_utf8(5);
  const std::u16string le = to_utf16le(implementation, s_u8);
  const std::u16string be = to_utf16be(implementation, s_u8);
  const char16_t *first_le_nfc =
      simdutf::find_first_stable_utf16le_nfc(le.data(), le.size());
  const char16_t *first_be_nfc =
      simdutf::find_first_stable_utf16be_nfc(be.data(), be.size());
  ASSERT_EQUAL((first_le_nfc - le.data()), (first_be_nfc - be.data()));
  const char16_t *first_le_nfd =
      simdutf::find_first_stable_utf16le_nfd(le.data(), le.size());
  const char16_t *first_be_nfd =
      simdutf::find_first_stable_utf16be_nfd(be.data(), be.size());
  ASSERT_EQUAL((first_le_nfd - le.data()), (first_be_nfd - be.data()));
}

TEST_MAIN
