#include "simdutf.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include <tests/helpers/random_int.h>
#include <tests/helpers/random_utf8.h>
#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>
#include <tests/reference/encode_utf16.h>

namespace {
std::array<size_t, 7> input_size{8, 16, 12, 64, 68, 128, 256};
} // namespace

TEST(pure_utf8_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 1, 0, 0, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(pure_utf16_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::RandomInt random(0,127, seed);

    for (size_t size : input_size) {
      std::vector<uint16_t> generated;
      for (int i = 0; i < size/2; i++) {
        generated.push_back(uint16_t(random()));
      }
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(pure_utf32_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::RandomInt random(0,0x7f, seed);

    for (size_t size : input_size) {
      std::vector<uint32_t> generated;
      for (int i = 0; i < size/4; i++) {
        generated.push_back(random());
      }
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE | simdutf::encoding_type::UTF32_LE;
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(no_utf8_bytes_no_surrogates) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::RandomIntRanges random({{0x007f, 0xd800-1},
                                                     {0xe000, 0xffff}}, seed);

    for (size_t size : input_size) {
      std::vector<uint32_t> generated;
      for (int i = 0; i < size/4; i++) {
        generated.push_back(random());
      }
      auto expected = simdutf::encoding_type::UTF16_LE | simdutf::encoding_type::UTF32_LE;
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(two_utf8_bytes) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 0, 1, 0, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(utf_16_surrogates) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf16 random(seed, 0, 1);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size/2);
      auto expected = simdutf::encoding_type::UTF16_LE;
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(utf32_surrogates) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::RandomInt random_prefix(0x10000, 0x10ffff, seed);
    simdutf::tests::helpers::RandomInt random_suffix(0xd800, 0xdfff, seed);

    for (size_t size : input_size) {
      std::vector<uint32_t> generated;
      for (int i = 0; i < size/4; i++) {
        generated.push_back((random_prefix() & 0xffff0000) + random_suffix());
      }
      auto expected = simdutf::encoding_type::UTF32_LE;
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

TEST(edge_surrogate) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::RandomInt random(0x10000, 0x10ffff, seed);

    const size_t size = 512;
    std::vector<uint16_t> generated(size/2,0);
    int i = 31;
    while (i + 32 < (size/2) - 1) {
      char16_t W1;
      char16_t W2;
      ASSERT_EQUAL(simdutf::tests::reference::utf16::encode(random(), W1, W2), 2);
      generated[i] = W1;
      generated[i+1] = W2;
      i += 32;
    }
    auto expected = simdutf::encoding_type::UTF16_LE;
    auto actual = implementation.detect_encodings(
                    reinterpret_cast<const char *>(generated.data()),
                    size);
    ASSERT_TRUE(actual == expected);
  }
}

TEST(tail_utf8) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 0, 0, 1, 0);
    std::array<size_t, 5> multiples_three{12, 54, 66, 126, 252};
    for (size_t size : multiples_three) {
      auto generated = random.generate_counted(size);
      auto expected = simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;
      auto actual = implementation.detect_encodings(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size);
      ASSERT_TRUE(actual == expected);
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
