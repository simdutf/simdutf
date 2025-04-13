#include "simdutf.h"

#include <random>
#include <thread>
#include <vector>

#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>

constexpr size_t trials = 1000;

#if SIMDUTF_IS_BIG_ENDIAN
constexpr char16_t replacement_le = 0xFDFF;
constexpr char16_t replacement_be = 0xFFFD;
#else
constexpr char16_t replacement_le = 0xFFFD;
constexpr char16_t replacement_be = 0xFDFF;
#endif

TEST_LOOP(trials, to_well_formed_utf16le_single_surrogate) {
  const size_t length = 128;
  std::vector<uint16_t> utf16(length);
#if SIMDUTF_IS_BIG_ENDIAN
  std::vector<char16_t> surrogates = {0x00D8, 0x00DC, 0xFFDF, 0x00D8, 0x00DC};
#else
  std::vector<char16_t> surrogates = {0xD800, 0xDC00, 0xDFFF, 0xD800, 0xDC00};
#endif
  std::vector<char16_t> output(length);
  for (size_t j = 0; j < length; j++) {
    for (char16_t surrogate : surrogates) {
      utf16[j] = surrogate;
      std::fill(output.begin(), output.end(), 0);
      implementation.to_well_formed_utf16le((const char16_t *)utf16.data(),
                                            utf16.size(), output.data());
      ASSERT_EQUAL(output[j], replacement_le);
      utf16[j] = 0x0000; // Reset to a valid character
    }
  }
}

TEST_LOOP(trials, to_well_formed_utf16be_single_surrogate) {
  const size_t length = 128;
  std::vector<uint16_t> utf16(length);
#if SIMDUTF_IS_BIG_ENDIAN
  std::vector<char16_t> surrogates = {0xD800, 0xDC00, 0xDFFF, 0xD800, 0xDC00};
#else
  std::vector<char16_t> surrogates = {0x00D8, 0x00DC, 0xFFDF, 0x00D8, 0x00DC};
#endif
  std::vector<char16_t> output(length);
  for (size_t j = 0; j < length; j++) {
    for (char16_t surrogate : surrogates) {
      utf16[j] = surrogate;
      std::fill(output.begin(), output.end(), 0);
      implementation.to_well_formed_utf16be((const char16_t *)utf16.data(),
                                            utf16.size(), output.data());
      ASSERT_EQUAL(output[j], replacement_be);
      utf16[j] = 0x0000; // Reset to a valid character
    }
  }
}

// Should be the identity on valid input
TEST_LOOP(trials,
          to_well_formed_utf16le_for_valid_input_surrogate_pairs_short) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate_le(8)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16le(utf16.data(), len, output.data());
  ASSERT_EQUAL(output, utf16);
}

TEST_LOOP(trials,
          to_well_formed_utf16be_for_valid_input_surrogate_pairs_short) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate_be(8)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16be(utf16.data(), len, output.data());
  ASSERT_EQUAL(output, utf16);
}

TEST_LOOP(trials, to_well_formed_utf16le_for_valid_input_surrogate_pairs_long) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate_le(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16le(utf16.data(), len, output.data());
  ASSERT_EQUAL(output, utf16);
}

TEST_LOOP(trials, to_well_formed_utf16be_for_valid_input_surrogate_pairs_long) {
  simdutf::tests::helpers::random_utf16 generator{seed, 0, 1};
  const auto utf16{generator.generate_be(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16be(utf16.data(), len, output.data());
  ASSERT_EQUAL(output, utf16);
}

TEST_LOOP(trials, to_well_formed_utf16le_for_valid_input_mixed_long) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate_le(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16le(utf16.data(), len, output.data());
  ASSERT_EQUAL(output, utf16);
}

TEST_LOOP(trials, to_well_formed_utf16be_for_valid_input_mixed_long) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate_be(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output(len);
  implementation.to_well_formed_utf16be(utf16.data(), len, output.data());
  ASSERT_EQUAL(output, utf16);
}

TEST_LOOP(trials, to_well_formed_utf16le_for_valid_input_mixed_long_self) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate_le(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output = utf16;
  implementation.to_well_formed_utf16le(output.data(), len, output.data());
  ASSERT_EQUAL(output, utf16);
}

TEST_LOOP(trials, to_well_formed_utf16be_for_valid_input_mixed_long_self) {
  simdutf::tests::helpers::random_utf16 generator{seed, 1, 1};
  const auto utf16{generator.generate_be(512)};
  const auto len = utf16.size();
  std::vector<char16_t> output = utf16;
  implementation.to_well_formed_utf16be(output.data(), len, output.data());
  ASSERT_EQUAL(output, utf16);
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
        ASSERT_EQUAL(output[j], replacement_le);
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
    std::vector<char16_t> output(len);
    implementation.to_well_formed_utf16be(utf16.data(), len, output.data());
    for (size_t j = 0; j < len; j++) {
      if (utf16[j] != output[j]) {
        ASSERT_EQUAL(output[j], replacement_be);
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
        ASSERT_EQUAL(output[j], replacement_le);
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
    std::vector<char16_t> output = utf16;
    implementation.to_well_formed_utf16be(output.data(), len, output.data());
    for (size_t j = 0; j < len; j++) {
      if (utf16[j] != output[j]) {
        ASSERT_EQUAL(output[j], replacement_be);
      }
    }
    ASSERT_TRUE(implementation.validate_utf16be(output.data(), len));
  }
}

TEST_MAIN
