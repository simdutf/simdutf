#include "simdutf.h"

#include <array>
#include <algorithm>

#include "helpers/random_utf16.h"
#include <tests/helpers/test.h>


namespace {
  std::vector<char16_t> generate_valid_utf16(size_t size = 512) {
    std::random_device rd{};
    simdutf::tests::helpers::random_utf16 generator{rd, 1, 0};
    return generator.generate(size);
  }
}

TEST(validate_utf16__returns_true_for_valid_input__single_words) {
  std::random_device rd{};
    simdutf::tests::helpers::random_utf16 generator{rd, 1, 0};
  const long seed = 0; // make test repeatable
  const auto utf16{generator.generate(512, seed)};

  ASSERT_TRUE(implementation.validate_utf16(
              reinterpret_cast<const char16_t*>(utf16.data()), utf16.size()));
}

TEST(validate_utf16__returns_true_for_valid_input__surrogate_pairs) {
  std::random_device rd{};
  simdutf::tests::helpers::random_utf16 generator{rd, 0, 1};
  const auto utf16{generator.generate(512)};

  ASSERT_TRUE(implementation.validate_utf16(
              reinterpret_cast<const char16_t*>(utf16.data()), utf16.size()));
}

// mixed = either 16-bit or 32-bit codewords
TEST(validate_utf16__returns_true_for_valid_input__mixed) {
  std::random_device rd{};
  simdutf::tests::helpers::random_utf16 generator{rd, 1, 1};
  const auto utf16{generator.generate(512)};

  ASSERT_TRUE(implementation.validate_utf16(
              reinterpret_cast<const char16_t*>(utf16.data()), utf16.size()));
}

TEST(validate_utf16__returns_true_for_empty_string) {
  const char16_t* buf = (char16_t*)"";

  ASSERT_TRUE(implementation.validate_utf16(buf, 0));
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
TEST(validate_utf16__returns_false_when_input_has_wrong_first_word_value) {
  auto utf16{generate_valid_utf16(128)};
  const char16_t*  buf = reinterpret_cast<const char16_t*>(utf16.data());
  const size_t len = utf16.size();

  for (char16_t wrong_value = 0xdc00; wrong_value <= 0xdfff; wrong_value++) {
    for (size_t i=0; i < utf16.size(); i++) {
      const char16_t old = utf16[i];
      utf16[i] = wrong_value;

      ASSERT_FALSE(implementation.validate_utf16(buf, len));

      utf16[i] = old;
    }
  }
}

/*
 RFC-2781:

 3) [..] if W2 is not between 0xDC00 and 0xDFFF, the sequence is in error.
    Terminate.
*/
TEST(validate_utf16__returns_false_when_input_has_wrong_second_word_value) {
  auto utf16{generate_valid_utf16(128)};
  const char16_t*  buf = reinterpret_cast<const char16_t*>(utf16.data());
  const size_t len = utf16.size();

  const std::array<char16_t, 5> sample_wrong_second_word{
    0x0000, 0x1000, 0xdbff, 0xe000, 0xffff
  };

  const char16_t valid_surrogate_W1 = 0xd800;
  for (char16_t W2: sample_wrong_second_word) {
    for (size_t i=0; i < utf16.size() - 1; i++) {
      const char16_t old_W1 = utf16[i + 0];
      const char16_t old_W2 = utf16[i + 1];

      utf16[i + 0] = valid_surrogate_W1;
      utf16[i + 1] = W2;

      ASSERT_FALSE(implementation.validate_utf16(buf, len));

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
TEST(validate_utf16__returns_false_when_input_is_truncated) {
  const char16_t valid_surrogate_W1 = 0xd800;
  for (size_t size = 1; size < 128; size++) {
    auto utf16{generate_valid_utf16(128)};
    const char16_t*  buf = reinterpret_cast<const char16_t*>(utf16.data());
    const size_t len = utf16.size();

    utf16[size - 1] = valid_surrogate_W1;

    ASSERT_FALSE(implementation.validate_utf16(buf, len));
  }
}

#include "validate_utf16_testcases.inl"

TEST(validate_utf16__extensive_tests) {
  constexpr size_t len = 32;
  char16_t buf[len];
  for (int i=0; i < validate_utf16_testcase_size; i++) {

    // prepare input
    const auto& testcase = validate_utf16_testcase[i];
    for (int j=0; j < 16; j++)
      buf[j] = testcase.values[j];

    for (int j=16; j < len; j++)
      buf[j] = V;

    // check
    ASSERT_TRUE(implementation.validate_utf16(buf, len) == testcase.valid);
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
