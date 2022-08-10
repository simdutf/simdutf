#include "simdutf.h"

#include <array>
#include <algorithm>

#include "helpers/random_utf16.h"
#include <tests/helpers/test.h>
#include <fstream>
#include <iostream>
#include <memory>

TEST(validate_utf16be__returns_true_for_valid_input__single_words) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf16{generator.generate(512, seed)};
    std::vector<char16_t> flipped(utf16.size());
    implementation.change_endianness_utf16(utf16.data(), utf16.size(), flipped.data());

    simdutf::result res = implementation.validate_utf16be_with_errors(reinterpret_cast<const char16_t*>(flipped.data()), flipped.size());
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(res.count, utf16.size());
  }
}

TEST(validate_utf16be__returns_true_for_valid_input__surrogate_pairs_short) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf16{generator.generate(8)};
    std::vector<char16_t> flipped(utf16.size());
    implementation.change_endianness_utf16(utf16.data(), utf16.size(), flipped.data());

    simdutf::result res = implementation.validate_utf16be_with_errors(reinterpret_cast<const char16_t*>(flipped.data()), flipped.size());
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(res.count, utf16.size());
  }
}


TEST(validate_utf16be__returns_true_for_valid_input__surrogate_pairs) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf16{generator.generate(512)};
    std::vector<char16_t> flipped(utf16.size());
    implementation.change_endianness_utf16(utf16.data(), utf16.size(), flipped.data());

    simdutf::result res = implementation.validate_utf16be_with_errors(reinterpret_cast<const char16_t*>(flipped.data()), flipped.size());
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(res.count, utf16.size());
  }
}

// mixed = either 16-bit or 32-bit codewords
TEST(validate_utf16be__returns_true_for_valid_input__mixed) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate(512)};
  std::vector<char16_t> flipped(utf16.size());
  implementation.change_endianness_utf16(utf16.data(), utf16.size(), flipped.data());

  simdutf::result res = implementation.validate_utf16be_with_errors(reinterpret_cast<const char16_t*>(flipped.data()), flipped.size());
  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(res.count, utf16.size());
}

TEST(validate_utf16be__returns_true_for_empty_string) {
  const char16_t* buf = (char16_t*)"";

  simdutf::result res = implementation.validate_utf16be_with_errors(reinterpret_cast<const char16_t*>(buf), 0);
  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(res.count, 0);
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
TEST(validate_utf16be__returns_false_when_input_has_wrong_first_word_value) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  for(size_t trial = 0; trial < 10; trial++) {
    auto utf16{generator.generate(128)};
    const size_t len = utf16.size();

    std::vector<char16_t> flipped(len);
    implementation.change_endianness_utf16(utf16.data(), utf16.size(), flipped.data());

    for (char16_t wrong_value = 0xdc00; wrong_value <= 0xdfff; wrong_value++) {
      for (size_t i=0; i < utf16.size(); i++) {
        const char16_t old = flipped[i];
        flipped[i] = char16_t((wrong_value >> 8) | (wrong_value << 8));

        simdutf::result res = implementation.validate_utf16be_with_errors(reinterpret_cast<const char16_t*>(flipped.data()), flipped.size());
        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);

        flipped[i] = old;
      }
    }
  }
}

/*
 RFC-2781:

 3) [..] if W2 is not between 0xDC00 and 0xDFFF, the sequence is in error.
    Terminate.
*/
TEST(validate_utf16be__returns_false_when_input_has_wrong_second_word_value) {
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  auto utf16{generator.generate(128)};
  const size_t len = utf16.size();

  std::vector<char16_t> flipped(len);
  implementation.change_endianness_utf16(utf16.data(), utf16.size(), flipped.data());

  const std::array<char16_t, 5> sample_wrong_second_word{
    0x0000, 0x0010, 0xffdb, 0x00e0, 0xffff
  };

  const char16_t valid_surrogate_W1 = 0x00d8;
  for (char16_t W2: sample_wrong_second_word) {
    for (size_t i=0; i < utf16.size() - 1; i++) {
      const char16_t old_W1 = flipped[i + 0];
      const char16_t old_W2 = flipped[i + 1];

      flipped[i + 0] = valid_surrogate_W1;
      flipped[i + 1] = W2;

      simdutf::result res = implementation.validate_utf16be_with_errors(reinterpret_cast<const char16_t*>(flipped.data()), flipped.size());
      ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
      ASSERT_EQUAL(res.count, i);

      flipped[i + 0] = old_W1;
      flipped[i + 1] = old_W2;
    }
  }
}

/*
 RFC-2781:

 3) If there is no W2 (that is, the sequence ends with W1) [...]
    the sequence is in error. Terminate.
*/
TEST(validate_utf16be__returns_false_when_input_is_truncated) {
  const char16_t valid_surrogate_W1 = 0x00d8;
  uint32_t seed{1234};
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 0};
  for (size_t size = 1; size < 128; size++) {
    auto utf16{generator.generate(128)};
    const size_t len = utf16.size();

    std::vector<char16_t> flipped(len);
    implementation.change_endianness_utf16(utf16.data(), utf16.size(), flipped.data());

    flipped[size - 1] = valid_surrogate_W1;

    simdutf::result res = implementation.validate_utf16be_with_errors(reinterpret_cast<const char16_t*>(flipped.data()), flipped.size());
    ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
    ASSERT_EQUAL(res.count, size - 1);
  }
}


int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
