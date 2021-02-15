#include "simdutf.h"

#include <array>
#include <random>
#include <algorithm>
#include <stdexcept>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>

#include "test_macros.h"

namespace {
  std::array<size_t, 4> input_size{16, 64, 128, 256};

  using simdutf::tests::helpers::TranscodeTestBase;
  using simdutf::tests::helpers::Conversion;

  class TranscodeTest: public TranscodeTestBase {
  public:
    TranscodeTest(GenerateCodepoint generate, size_t input_size)
      : TranscodeTestBase(generate, input_size, Conversion::utf8_to_utf16) {}
  };
}

TEST(convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t {
    return counter++ & 0x7f;
  };

  auto procedure = [&implementation](const char* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16(utf8, size, utf16);
  };

  for (size_t size: input_size) {
    TranscodeTest test(generator, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST(convert_1_or_2_UTF8_bytes) {
  simdutf::tests::helpers::RandomInt random(0x0000, 0x07ff); // range for 1 or 2 UTF-8 bytes

  auto procedure = [&implementation](const char* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16(utf8, size, utf16);
  };

  for (size_t size: input_size) {
    TranscodeTest test(random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST(convert_1_or_2_or_3_UTF8_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0xd7ff},
                                                   {0xe000, 0xffff}});

  auto procedure = [&implementation](const char* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16(utf8, size, utf16);
  };

  for (size_t size: input_size) {
    TranscodeTest test(random, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST(convert_3_or_4_UTF8_bytes) {
  simdutf::tests::helpers::RandomInt random(0x0800, 0x10'ffff); // range for 3 or 4 UTF-8 bytes

  auto procedure = [&implementation](const char* utf8, size_t size, char* utf16) -> size_t {
    return implementation.convert_valid_utf8_to_utf16(utf8, size, utf16);
  };

  for (size_t size: input_size) {
    TranscodeTest test(random, size);
    ASSERT_TRUE(test(procedure));
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

    for (auto test: test_procedures())
      test(*implementation);
  }
}
