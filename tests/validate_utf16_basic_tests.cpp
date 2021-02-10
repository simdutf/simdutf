#include "simdutf.h"

#include <random>
#include <algorithm>

namespace utf16::random {

  // Generates UTF16 string with one-word codepoints; values
  // from ranges [0x0000 .. 0xd7ff] and [0xe000 .. 0xffff]
  class SingleCodepointGenerator {
    std::mt19937 gen;

  public:
    SingleCodepointGenerator(std::random_device& rd)
      : gen{rd()} {}

    std::vector<uint16_t> generate(size_t size);
    std::vector<uint16_t> generate(size_t size, long seed);

  private:
    std::uniform_int_distribution<uint16_t> random_word{0x0000, 0xffff};
    uint16_t generate();
  };

  std::vector<uint16_t> SingleCodepointGenerator::generate(size_t count)
  {
    std::vector<uint16_t> result;
    result.reserve(count);
    for (size_t i=0; i < count; i++)
      result.push_back(generate());

    return result;
  }

  std::vector<uint16_t> SingleCodepointGenerator::generate(size_t size, long seed) {
    gen.seed(seed);
    return generate(size);
  }

  uint16_t SingleCodepointGenerator::generate() {
    while (true) {
      const uint16_t res = random_word(gen);
      if (res <= 0xd7ff or res >= 0xe000)
        return res;
    }
  }

} // namespace utf16::random

#define TEST(name)                                          \
void test_impl_##name(const simdutf::implementation& impl); \
void name(const simdutf::implementation& impl) {            \
  std::string title = #name;                                \
  std::replace(title.begin(), title.end(), '_', ' ');       \
  printf("%s...", title.c_str()); fflush(stdout);           \
  test_impl_##name(impl);                                   \
  puts(" OK");                                              \
}                                                           \
void test_impl_##name(const simdutf::implementation& implementation)

#define ASSERT_TRUE(cond) {                                 \
  const bool expr = (cond);                                 \
  if (!expr) {                                              \
    printf("expected %s to be true, it's false\n", #cond);  \
    exit(1);                                                \
  }                                                         \
}

#define ASSERT_FALSE(cond) {                                \
  const bool expr = !(cond);                                \
  if (!expr) {                                              \
    printf("expected %s to be false, it's true\n", #cond);  \
    exit(1);                                                \
  }                                                         \
}

std::vector<uint16_t> generate_valid_utf16(size_t size = 512) {
  std::random_device rd{};
  utf16::random::SingleCodepointGenerator generator{rd};
  return generator.generate(size);
}

TEST(validate_utf16__returns_true_for_valid_input_single_codepoint) {
  const auto utf16{generate_valid_utf16()};

  ASSERT_TRUE(implementation.validate_utf16(
                reinterpret_cast<const char*>(utf16.data()), utf16.size() * 2));
}

TEST(validate_utf16__returns_true_for_empty_string) {
  const char* buf = "";

  ASSERT_TRUE(implementation.validate_utf16(buf, 0));
}

TEST(validate_utf16__returns_false_when_input_has_odd_number_of_bytes) {
  const char* buf = "?";

  ASSERT_FALSE(implementation.validate_utf16(buf, 1));
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
  const char*  buf = reinterpret_cast<const char*>(utf16.data());
  const size_t len = 2 * utf16.size();

  for (uint16_t wrong_value = 0xdc00; wrong_value <= 0xdfff; wrong_value++) {
    for (size_t i=0; i < utf16.size(); i++) {
      const uint16_t old = utf16[i];
      utf16[i] = wrong_value;

      ASSERT_FALSE(implementation.validate_utf16(buf, 1));

      utf16[i] = old;
    }
  }
}

int main() {
  for (const auto& implementation: simdutf::available_implementations) {
    if (implementation == nullptr) {
      puts("SIMDUTF implementation is null");
      abort();
    }

    const simdutf::implementation& impl = *implementation;
    printf("Checking implementation %s\n", implementation->name().c_str());

    validate_utf16__returns_true_for_valid_input_single_codepoint(*implementation);
    validate_utf16__returns_true_for_empty_string(*implementation);
    validate_utf16__returns_false_when_input_has_odd_number_of_bytes(*implementation);
    validate_utf16__returns_false_when_input_has_wrong_first_word_value(*implementation);
  }
}
