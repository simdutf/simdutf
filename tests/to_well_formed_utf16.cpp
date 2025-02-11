#include "simdutf.h"

#include <random>
#include <thread>
#include <vector>

#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>

constexpr size_t trials = 1000;

// Should be the identity on valid input
TEST_LOOP(trials,
          to_well_formed_utf16le_for_valid_input_surrogate_pairs_short) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate(8)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16le(utf16.data(), len, output.data());
  ASSERT_TRUE(output == utf16);
}

TEST_LOOP(trials,
          to_well_formed_utf16be_for_valid_input_surrogate_pairs_short) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate(8)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  std::vector<char16_t> flipped(utf16.size());
  implementation.change_endianness_utf16(utf16.data(), utf16.size(),
                                         flipped.data());
  implementation.to_well_formed_utf16be(flipped.data(), len, output.data());
  ASSERT_TRUE(output == flipped);
}

TEST_LOOP(trials, to_well_formed_utf16le_for_valid_input_surrogate_pairs_long) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16le(utf16.data(), len, output.data());
  ASSERT_TRUE(output == utf16);
}

TEST_LOOP(trials, to_well_formed_utf16be_for_valid_input_surrogate_pairs_long) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  std::vector<char16_t> flipped(utf16.size());
  implementation.change_endianness_utf16(utf16.data(), utf16.size(),
                                         flipped.data());
  implementation.to_well_formed_utf16be(flipped.data(), len, output.data());
  ASSERT_TRUE(output == flipped);
}

TEST_LOOP(trials, to_well_formed_utf16le_for_valid_input_mixed_long) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16le(utf16.data(), len, output.data());
  ASSERT_TRUE(output == utf16);
}

TEST_LOOP(trials, to_well_formed_utf16be_for_valid_input_mixed_long) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  std::vector<char16_t> flipped(utf16.size());
  implementation.change_endianness_utf16(utf16.data(), utf16.size(),
                                         flipped.data());
  implementation.to_well_formed_utf16be(flipped.data(), len, output.data());
  ASSERT_TRUE(output == flipped);
}

TEST_LOOP(trials, to_well_formed_utf16le_for_valid_input_mixed_long_self) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output = utf16;
  implementation.to_well_formed_utf16le(output.data(), len, output.data());
  ASSERT_TRUE(output == utf16);
}

TEST_LOOP(trials, to_well_formed_utf16be_for_valid_input_mixed_long_self) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate(512)};
  const auto len = utf16.size();
  std::vector<char16_t> flipped(utf16.size());
  implementation.change_endianness_utf16(utf16.data(), utf16.size(),
                                         flipped.data());
  std::vector<char16_t> output = flipped;
  implementation.to_well_formed_utf16be(output.data(), len, output.data());
  ASSERT_TRUE(output == flipped);
}

std::vector<char16_t> random_testcase(size_t n, std::mt19937 &rng) {
  std::uniform_int_distribution<int> dist(0, 0xFFFF);
  std::uniform_int_distribution<int> disthl(0, 3);
  std::vector<char16_t> buf(n);
  for (size_t i = 0; i < n; ++i) {
    uint16_t random_value(dist(rng));
    uint8_t random_hl(disthl(rng));
    if (random_hl == 0) { // 25% for low surrogate
      buf[i] = 0xD800 | (random_value & 0x07FF);
    } else if (random_hl == 1) { // Another 25% for high surrogate
      buf[i] = 0xDC00 | (random_value & 0x07FF);
    } else {
      // Generate any other character in the char16_t range
      // could be a surrogate pair...
      buf[i] = random_value;
    }
  }
  return buf;
}

TEST(to_well_formed_utf16le_bad_input) {
  std::mt19937 gen((std::mt19937::result_type)(42));
  for (size_t i = 0; i < 1000; i++) {
    auto utf16 = random_testcase(512, gen);
    auto len = utf16.size();
    std::vector<char16_t> output(len);
    implementation.to_well_formed_utf16le(utf16.data(), len, output.data());
    for (size_t j = 0; j < len; j++) {
      if (utf16[j] != output[j]) {
#if SIMDUTF_IS_BIG_ENDIAN
        ASSERT_TRUE(output[j] == 0xFDFF);
#else
        ASSERT_TRUE(output[j] == 0xFFFD);
#endif
      }
    }
    ASSERT_TRUE(implementation.validate_utf16le(output.data(), len));
  }
}

TEST(to_well_formed_utf16be_bad_input) {
  std::mt19937 gen((std::mt19937::result_type)(42));
  for (size_t i = 0; i < 1000; i++) {
    auto utf16 = random_testcase(512, gen);
    auto len = utf16.size();
    std::vector<char16_t> flipped(utf16.size());
    implementation.change_endianness_utf16(utf16.data(), utf16.size(),
                                           flipped.data());
    std::vector<char16_t> output(len);
    implementation.to_well_formed_utf16be(flipped.data(), len, output.data());
    for (size_t j = 0; j < len; j++) {
      if (flipped[j] != output[j]) {
#if SIMDUTF_IS_BIG_ENDIAN
        ASSERT_TRUE(output[j] == 0xFFFD);
#else
        ASSERT_TRUE(output[j] == 0xFDFF);
#endif
      }
    }
    ASSERT_TRUE(implementation.validate_utf16be(output.data(), len));
  }
}

TEST(to_well_formed_utf16le_bad_input_self) {
  std::mt19937 gen((std::mt19937::result_type)(42));
  for (size_t i = 0; i < 1000; i++) {
    auto utf16 = random_testcase(512, gen);
    auto len = utf16.size();
    std::vector<char16_t> output = utf16;
    implementation.to_well_formed_utf16le(output.data(), len, output.data());
    for (size_t j = 0; j < len; j++) {
      if (utf16[j] != output[j]) {
#if SIMDUTF_IS_BIG_ENDIAN
        ASSERT_TRUE(output[j] == 0xFDFF);
#else
        ASSERT_TRUE(output[j] == 0xFFFD);
#endif
      }
    }
    ASSERT_TRUE(implementation.validate_utf16le(output.data(), len));
  }
}

TEST(to_well_formed_utf16be_bad_input_self) {
  std::mt19937 gen((std::mt19937::result_type)(42));
  for (size_t i = 0; i < 1000; i++) {
    auto utf16 = random_testcase(512, gen);
    auto len = utf16.size();
    std::vector<char16_t> flipped(utf16.size());
    implementation.change_endianness_utf16(utf16.data(), utf16.size(),
                                           flipped.data());
    std::vector<char16_t> output = flipped;
    implementation.to_well_formed_utf16be(output.data(), len, output.data());
    for (size_t j = 0; j < len; j++) {
      if (flipped[j] != output[j]) {
#if SIMDUTF_IS_BIG_ENDIAN
        ASSERT_TRUE(output[j] == 0xFFFD);
#else
        ASSERT_TRUE(output[j] == 0xFDFF);
#endif
      }
    }
    ASSERT_TRUE(implementation.validate_utf16be(output.data(), len));
  }
}

TEST_MAIN
