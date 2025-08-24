#include "simdutf.h"

#include <array>
#include <fstream>
#include <memory>

#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>
#include <tests/helpers/utf16.h>

constexpr size_t trials = 1000;

TEST(issue92) {
  char16_t input[] = u"\u5d00\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041"
                     u"\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041"
                     u"\u0041\u0041\u0041\u0041\u0041\u0041";
  size_t strlen = sizeof(input) / sizeof(char16_t);
  to_utf16le_inplace(input, strlen);

  ASSERT_TRUE(implementation.validate_utf16le(input, strlen));
  ASSERT_EQUAL(implementation.utf8_length_from_utf16le(input, strlen),
               2 + strlen);
  size_t size =
      implementation.utf8_length_from_utf16le(input, strlen); // should be 26.
  std::unique_ptr<char[]> output_buffer{new char[size]};
  size_t measured_size = implementation.convert_valid_utf16le_to_utf8(
      input, strlen, output_buffer.get());
  ASSERT_EQUAL(measured_size, size);
}

TEST_LOOP(trials, validate_utf16le_ascii) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  auto utf16{generator.generate_le(512, seed)};
  generator.to_ascii_le(utf16);
  ASSERT_TRUE(
      implementation.validate_utf16le_as_ascii(utf16.data(), utf16.size()));
  utf16[utf16.size() / 2] = 0xC0C0;
  ASSERT_FALSE(
      implementation.validate_utf16le_as_ascii(utf16.data(), utf16.size()));
}

TEST_LOOP(trials, validate_utf16le_returns_true_for_valid_input_single_words) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  const auto utf16{generator.generate_le(512, seed)};

  ASSERT_TRUE(implementation.validate_utf16le(
      reinterpret_cast<const char16_t *>(utf16.data()), utf16.size()));
}

TEST_LOOP(trials,
          validate_utf16le_returns_true_for_valid_input_surrogate_pairs_short) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate_le(8)};

  ASSERT_TRUE(implementation.validate_utf16le(
      reinterpret_cast<const char16_t *>(utf16.data()), utf16.size()));
}

TEST_LOOP(trials,
          validate_utf16le_returns_true_for_valid_input_surrogate_pairs) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate_le(512)};

  ASSERT_TRUE(implementation.validate_utf16le(
      reinterpret_cast<const char16_t *>(utf16.data()), utf16.size()));
}

// mixed = either 16-bit or 32-bit codewords
TEST(validate_utf16le_returns_true_for_valid_input_mixed) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate_le(512)};

  ASSERT_TRUE(implementation.validate_utf16le(
      reinterpret_cast<const char16_t *>(utf16.data()), utf16.size()));
}

TEST(validate_utf16le_returns_true_for_empty_string) {
  const char16_t *buf = (char16_t *)"";

  ASSERT_TRUE(implementation.validate_utf16le(buf, 0));
}

// The first word must not be in range [0xDC00 .. 0xDFFF]
/*
2.2 Decoding UTF-16

   [...]

   1) If W1 < 0xD800 or W1 > 0xDFFF, the character value U is the value
      of W1. Terminate.

   2) Determine if W1 is between 0xD800 and 0xDBFF. If not, the sequence
      is in error [...]
*/
TEST_LOOP(
    10, validate_utf16le_returns_false_when_input_has_wrong_first_word_value) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  auto utf16{generator.generate_le(128)};
  const size_t len = utf16.size();

  for (char16_t wrong_value = 0xdc00; wrong_value <= 0xdfff; wrong_value++) {
    for (size_t i = 0; i < utf16.size(); i++) {
      const char16_t old = utf16[i];
      utf16[i] = to_utf16le(wrong_value);

      ASSERT_FALSE(implementation.validate_utf16le(utf16.data(), len));

      utf16[i] = old;
    }
  }
}

/*
 RFC-2781:

 3) [..] if W2 is not between 0xDC00 and 0xDFFF, the sequence is in error.
    Terminate.
*/
TEST(validate_utf16le_returns_false_when_input_has_wrong_second_word_value) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  auto utf16{generator.generate_le(128)};
  const size_t len = utf16.size();
  const std::array<char16_t, 5> sample_wrong_second_word{0x0000, 0x1000, 0xdbff,
                                                         0xe000, 0xffff};
  const char16_t valid_surrogate_W1 = to_utf16le(0xd800);
  for (char16_t W2 : sample_wrong_second_word) {
    for (size_t i = 0; i < utf16.size() - 1; i++) {
      const char16_t old_W1 = utf16[i + 0];
      const char16_t old_W2 = utf16[i + 1];

      utf16[i + 0] = valid_surrogate_W1;
      utf16[i + 1] = to_utf16le(W2);
      ASSERT_FALSE(implementation.validate_utf16le(utf16.data(), len));

      utf16[i + 0] = old_W1;
      utf16[i + 1] = old_W2;
    }
  }
}

/*
 RFC-2781:

 3) If there is no W2 (that is, the sequence ends with W1) [...]
    the sequence is in error. Terminate.
*/
TEST(validate_utf16le_returns_false_when_input_is_truncated) {
  const char16_t valid_surrogate_W1 = to_utf16le(0xd800);
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  for (size_t size = 1; size < 128; size++) {
    auto utf16{generator.generate_le(128)};
    const size_t len = utf16.size();

    utf16[size - 1] = valid_surrogate_W1;

    ASSERT_FALSE(implementation.validate_utf16le(utf16.data(), len));
  }
}

TEST(validate_utf16le_extensive_tests) {
#ifdef RUN_IN_SPIKE_SIMULATOR
  printf("skipping, cannot be run under Spike");
  return;
#endif
  const std::string path{"validate_utf16_testcases.txt"};
  std::ifstream file{path};
  if (not file) {
    printf("skipping, file '%s' cannot be open", path.c_str());
    return;
  }

  const uint16_t V = to_utf16le(0xfaea);
  const uint16_t L = to_utf16le(0xd852);
  const uint16_t H = to_utf16le(0xde12);

  constexpr size_t len = 32;
  char16_t buf[len];

  long lineno = 0;
  while (file) {
    std::string line;
    std::getline(file, line);
    lineno += 1;
    if (line.empty() or line[0] == '#')
      continue;

    // format: [TF][VLH]{16}
    bool valid = false;
    switch (line[0]) {
    case 'T':
      valid = true;
      break;
    case 'F':
      valid = false;
      break;
    default:
      throw std::invalid_argument(
          "Error at line #" + std::to_string(lineno) +
          ": the first character must be either 'T' or 'F'");
    }

    // prepare input
    for (size_t i = 0; i < len; i++) {
      buf[i] = V;
    }

    for (size_t i = 1; i < line.size(); i++) {
      switch (line[i]) {
      case 'L':
        buf[i - 1] = L;
        break;
      case 'H':
        buf[i - 1] = H;
        break;
      case 'V':
        buf[i - 1] = V;
        break;
      default:
        throw std::invalid_argument(
            "Error at line #" + std::to_string(lineno) +
            ": allowed characters are 'L', 'H' and 'V'");
      }
    }

    // check
    ASSERT_EQUAL(implementation.validate_utf16le(buf, len), valid);
  }
}

TEST_MAIN
