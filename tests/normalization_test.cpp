#include "simdutf.h"

#include <tests/helpers/test.h>
#include <tests/normalization_test_data/normalization_test_data.h>

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
  for (auto b : s) {
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

TEST_MAIN
