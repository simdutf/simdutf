#include "simdutf.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <tuple>

#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <vector>

TEST(stop_before_partial_one_char) {
  std::vector<char> base64(5463, 0x20);
  base64.back() = 0x38; // this is the number 8 (a valid base64 character)
  std::vector<char> back(0);
  // with stop_before_partial, we should stop before the last character
  // and not decode it. There should be no error.
  // https://tc39.es/proposal-arraybuffer-base64/spec/#sec-frombase64
  simdutf::result r = implementation.base64_to_binary(
      base64.data(), base64.size(), back.data(), simdutf::base64_default,
      simdutf::last_chunk_handling_options::stop_before_partial);
  ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(r.count, 0);
  size_t buflen = back.size();
  ASSERT_EQUAL(buflen, 0);
  r = simdutf::base64_to_binary_safe(
      base64.data(), base64.size(), back.data(), buflen,
      simdutf::base64_default,
      simdutf::last_chunk_handling_options::stop_before_partial);
  ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(buflen, 0);
  ASSERT_EQUAL(r.count, 5462);
  back.resize(base64.size());
  buflen = back.size();
  r = simdutf::base64_to_binary_safe(
      base64.data(), base64.size(), back.data(), buflen,
      simdutf::base64_default,
      simdutf::last_chunk_handling_options::stop_before_partial);
  ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(buflen, 0);
  ASSERT_EQUAL(r.count, 5462);
}

TEST(hybrid_decoding) {
  std::vector<std::pair<std::string, std::vector<uint8_t>>> test_data = {
      {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA__--_--"
       "_--__AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
       {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xbe, 0xff, 0xef, 0xbf, 0xfb, 0xef, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
      {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA__-+_--"
       "_--/_AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
       {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xbe, 0xff, 0xef, 0xbf, 0xfb, 0xef, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
      {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA__-+_--"
       " / "
       "--/_AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
       {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xbe, 0xff, 0xef, 0xbf, 0xfb, 0xef, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
      {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA//-+/"
       "--/--/"
       "_AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
       {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xbe, 0xff, 0xef, 0xbf, 0xfb, 0xef, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},

  };
  for (const auto &test : test_data) {
    const std::string &base64 = test.first;
    const std::vector<uint8_t> &expected = test.second;
    std::vector<uint8_t> decoded(simdutf::maximal_binary_length_from_base64(
        base64.data(), base64.size()));
    auto r = implementation.base64_to_binary(
        base64.data(), base64.size(), reinterpret_cast<char *>(decoded.data()),
        simdutf::base64_default_or_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, expected.size());
    decoded.resize(r.count);
    ASSERT_EQUAL(decoded, expected);
  }
}

// We may disable base64url tests by commenting out this next line.
#define SIMDUTF_BASE64URL_TESTS 1

using random_generator = std::mt19937;
static random_generator::result_type seed = 42;

constexpr uint8_t to_base64_value[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 64,  64,  255, 64,  64,  255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 64,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255,
    255, 255, 63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255,
    255, 255, 255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25,  255, 255, 255, 255, 255, 255, 26,  27,  28,  29,  30,  31,  32,  33,
    34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255};

constexpr uint8_t to_base64url_value[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 64,  64,  255, 64,  64,  255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 64,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    62,  255, 255, 52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255,
    255, 255, 255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25,  255, 255, 255, 255, 63,  255, 26,  27,  28,  29,  30,  31,  32,  33,
    34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255};

template <typename char_type> bool is_space(char_type c) {
  const static std::array<char_type, 5> space = {' ', '\t', '\n', '\r', '\f'};
  return std::find(space.begin(), space.end(), c) != space.end();
}

template <typename char_type> bool is_non_base64_space(char_type c) {
  return uint8_t(c) >= 128 || to_base64_value[uint8_t(c)] == 255;
}

template <typename char_type> bool is_non_base64_url_space(char_type c) {
  return uint8_t(c) >= 128 || to_base64url_value[uint8_t(c)] == 255;
}

template <typename char_type>
size_t add_space(std::vector<char_type> &v, std::mt19937 &gen) {
  const static std::array<char_type, 5> space = {' ', '\t', '\n', '\r', '\f'};
  std::uniform_int_distribution<int> index_dist(0, v.size());
  size_t i = index_dist(gen);
  std::uniform_int_distribution<int> char_dist(0, space.size() - 1);
  v.insert(v.begin() + i, space[char_dist(gen)]);
  return i;
}

// consider using add_simple_spaces for better performance.
template <typename char_type>
size_t add_simple_space(std::vector<char_type> &v, std::mt19937 &gen) {
  std::uniform_int_distribution<int> index_dist(0, v.size());
  size_t i = index_dist(gen);
  v.insert(v.begin() + i, ' ');
  return i;
}

template <typename char_type>
std::vector<char_type> add_simple_spaces(std::vector<char_type> &v,
                                         std::mt19937 &gen,
                                         size_t number_of_spaces) {
  // If there are no spaces to add or the vector is empty, return
  if (number_of_spaces == 0) {
    return v;
  }

  // Generate unique random positions
  std::vector<bool> positions(v.size() + number_of_spaces, false);
  std::uniform_int_distribution<size_t> dist(0, positions.size() - 1);
  for (size_t i = 0; i < number_of_spaces; ++i) {
    size_t pos = dist(gen);
    while (positions[pos]) {
      pos = dist(gen);
    }
    positions[pos] = true;
  }
  std::vector<char_type> result;
  result.resize(v.size() + number_of_spaces);
  int pos = 0;
  for (size_t i = 0; i < v.size() + number_of_spaces; ++i) {
    if (positions[i]) {
      result[i] = ' ';
    } else {
      result[i] = v[pos++];
    }
  }
  return result;
}

template <typename char_type>
size_t add_garbage(std::vector<char_type> &v, std::mt19937 &gen,
                   const uint8_t *table) {
  auto equal_sign = std::find(v.begin(), v.end(), '=');
  size_t len = v.size();
  if (equal_sign != v.end()) {
    len = std::distance(v.begin(), equal_sign);
  }
  std::uniform_int_distribution<int> index_dist(0, len);
  size_t i = index_dist(gen);
  std::uniform_int_distribution<int> char_dist(
      0, (1 << (sizeof(char_type) * 8)) - 1);
  uint8_t c = char_dist(gen);
  while (uint8_t(c) == c && table[uint8_t(c)] != 255) {
    c = char_dist(gen);
  }
  v.insert(v.begin() + i, c);
  return i;
}

TEST(roundtrip_base64_with_spaces) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = 'a' + i % ('z' - 'a'); // byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size);
      for (size_t i = 0; i < 5; i++) {
        add_space(buffer, gen);
      }
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        simdutf::result r = implementation.base64_to_binary(
            buffer.data(), buffer.size(), back.data(), simdutf::base64_default,
            option);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(r.count, len);
        ASSERT_BYTES_EQUAL(source, back, len);
      }
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        size_t back_length = back.size();
        auto r = simdutf::base64_to_binary_safe(
            buffer.data(), buffer.size(), back.data(), back_length,
            simdutf::base64_default, option);

        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        if (option ==
            simdutf::last_chunk_handling_options::stop_before_partial) {
          for (size_t i = r.count; i < buffer.size(); i++) {
            ASSERT_TRUE(is_space(buffer[i]));
          }
        } else {
          ASSERT_EQUAL(r.count, buffer.size());
        }
        ASSERT_TRUE(std::equal(back.begin(), back.begin() + back_length,
                               source.begin()));
      }
    }
  }
}

TEST(roundtrip_base64_with_garbage) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size);
      for (size_t i = 0; i < 5; i++) {
        add_garbage(buffer, gen, to_base64_value);
      }
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        simdutf::result r = implementation.base64_to_binary(
            buffer.data(), buffer.size(), back.data(),
            simdutf::base64_default_accept_garbage, option);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(r.count, len);
        ASSERT_TRUE(
            std::equal(back.begin(), back.begin() + len, source.begin()));
      }
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        size_t back_length = back.size();
        auto r = simdutf::base64_to_binary_safe(
            buffer.data(), buffer.size(), back.data(), back_length,
            simdutf::base64_default_accept_garbage, option);

        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        if (option ==
            simdutf::last_chunk_handling_options::stop_before_partial) {
          for (size_t i = r.count; i < buffer.size(); i++) {
            ASSERT_TRUE(is_non_base64_space(buffer[i]));
          }
        } else {
          ASSERT_EQUAL(r.count, buffer.size());
        }
        ASSERT_TRUE(std::equal(back.begin(), back.begin() + back_length,
                               source.begin()));
      }
    }
  }
}

TEST(roundtrip_base64_url_with_garbage) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data(), simdutf::base64_url);
      buffer.resize(size);
      for (size_t i = 0; i < 5; i++) {
        add_garbage(buffer, gen, to_base64url_value);
      }
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        simdutf::result r = implementation.base64_to_binary(
            buffer.data(), buffer.size(), back.data(),
            simdutf::base64_url_accept_garbage, option);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(r.count, len);
        ASSERT_TRUE(
            std::equal(back.begin(), back.begin() + len, source.begin()));
      }
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        size_t back_length = back.size();
        auto r = simdutf::base64_to_binary_safe(
            buffer.data(), buffer.size(), back.data(), back_length,
            simdutf::base64_url_accept_garbage, option);

        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        if (option ==
            simdutf::last_chunk_handling_options::stop_before_partial) {
          for (size_t i = r.count; i < buffer.size(); i++) {
            ASSERT_TRUE(is_non_base64_url_space(buffer[i]));
          }
        } else {
          ASSERT_EQUAL(r.count, buffer.size());
        }
        ASSERT_TRUE(std::equal(back.begin(), back.begin() + back_length,
                               source.begin()));
      }
    }
  }
}

TEST(roundtrip_base64_with_lots_of_spaces) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size);
      add_simple_spaces(buffer, gen, 5 + 2 * len);
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        simdutf::result r = implementation.base64_to_binary(
            buffer.data(), buffer.size(), back.data(), simdutf::base64_default,
            option);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(r.count, len);
        ASSERT_TRUE(
            std::equal(back.begin(), back.begin() + len, source.begin()));
      }
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        size_t back_length = back.size();
        auto r = simdutf::base64_to_binary_safe(
            buffer.data(), buffer.size(), back.data(), back_length,
            simdutf::base64_default, option);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        if (option ==
            simdutf::last_chunk_handling_options::stop_before_partial) {
          for (size_t i = r.count; i < buffer.size(); i++) {
            ASSERT_TRUE(is_space(buffer[i]));
          }
          if (r.count > 0) {
            ASSERT_TRUE(!is_space(buffer[r.count - 1]));
          }
        } else {
          ASSERT_EQUAL(r.count, buffer.size());
        }
        ASSERT_TRUE(std::equal(back.begin(), back.begin() + back_length,
                               source.begin()));
      }
    }
  }
}

// partial decoding with 'safe' will succeed and just decode the first 3 bytes,
// even if we have ample output memory. It should consume only 4 bytes.
TEST(base64_decode_just_one_padding_partial_safe) {
  std::vector<std::tuple<std::string, simdutf::result, size_t>> test_cases = {
      {"uuuu             =", {simdutf::error_code::SUCCESS, 4}, 3}};
  std::vector<char> buffer(128);
  for (auto &p : test_cases) {
    auto input = std::get<0>(p);
    auto expected_result = std::get<1>(p);
    size_t expected_output = std::get<2>(p);

    for (auto option : {simdutf::base64_options::base64_default,
                        simdutf::base64_options::base64_url}) {
      for (auto chunk_option :
           {simdutf::last_chunk_handling_options::stop_before_partial}) {
        for (size_t output = 3; output < buffer.size(); output++) {
          size_t written = output;
          auto result = simdutf::base64_to_binary_safe(
              input.data(), input.size(), buffer.data(), written, option,
              chunk_option);
          ASSERT_EQUAL(result.error, expected_result.error);
          ASSERT_EQUAL(result.count, expected_result.count);
          ASSERT_EQUAL(written, expected_output);
        }
      }
    }
  }
}

// partial decoding will succeed and just decode the first 3 bytes, even if we
// have ample output memory.
TEST(base64_decode_just_one_padding_partial_generous) {
  std::vector<std::pair<std::string, simdutf::result>> test_cases = {
      {"uuuu             =", {simdutf::error_code::SUCCESS, 3}}};
  std::vector<char> buffer(6);
  for (auto &p : test_cases) {
    auto input = p.first;
    auto expected_result = p.second;
    for (auto option : {simdutf::base64_options::base64_default,
                        simdutf::base64_options::base64_url}) {
      for (auto chunk_option :
           {simdutf::last_chunk_handling_options::stop_before_partial}) {
        auto result = implementation.base64_to_binary(
            input.data(), input.size(), buffer.data(), option, chunk_option);
        ASSERT_EQUAL(result.error, expected_result.error);
        ASSERT_EQUAL(result.count, expected_result.count);
      }
    }
  }
}

// loose decoding will fail when there is a single leftover padding character.
TEST(base64_decode_just_one_padding_loose) {
  std::vector<std::pair<std::string, simdutf::result>> test_cases = {
      {"uuuu             =",
       {simdutf::error_code::INVALID_BASE64_CHARACTER, 17}}};
  std::vector<char> buffer(3);
  for (auto &p : test_cases) {
    auto input = p.first;
    auto expected_result = p.second;
    for (auto option : {simdutf::base64_options::base64_default,
                        simdutf::base64_options::base64_url}) {
      for (auto chunk_option : {simdutf::last_chunk_handling_options::loose}) {
        auto result = implementation.base64_to_binary(
            input.data(), input.size(), buffer.data(), option, chunk_option);
        ASSERT_EQUAL(result.error, expected_result.error);
        ASSERT_EQUAL(result.count, expected_result.count);
      }
    }
  }
}

// strict decoding will fail with a pointer to the last valid character.
TEST(base64_decode_just_one_padding_strict) {
  std::vector<std::pair<std::string, simdutf::result>> test_cases = {
      {"uuuu             =", {simdutf::error_code::BASE64_INPUT_REMAINDER, 3}}};
  std::vector<char> buffer(3);
  for (auto &p : test_cases) {
    auto input = p.first;
    auto expected_result = p.second;
    for (auto option : {simdutf::base64_options::base64_default,
                        simdutf::base64_options::base64_url}) {
      for (auto chunk_option : {simdutf::last_chunk_handling_options::strict}) {
        auto result = implementation.base64_to_binary(
            input.data(), input.size(), buffer.data(), option, chunk_option);
        ASSERT_EQUAL(result.error, expected_result.error);
        ASSERT_EQUAL(result.count, expected_result.count);
      }
    }
  }
}

// partial decoding will succeed and just decode the first 3 bytes.
TEST(base64_decode_just_one_padding_partial) {
  std::vector<std::pair<std::string, simdutf::result>> test_cases = {
      {"uuuu             =", {simdutf::error_code::SUCCESS, 3}}};
  std::vector<char> buffer(3);
  for (auto &p : test_cases) {
    auto input = p.first;
    auto expected_result = p.second;
    for (auto option : {simdutf::base64_options::base64_default,
                        simdutf::base64_options::base64_url}) {
      for (auto chunk_option :
           {simdutf::last_chunk_handling_options::stop_before_partial}) {
        auto result = implementation.base64_to_binary(
            input.data(), input.size(), buffer.data(), option, chunk_option);
        ASSERT_EQUAL(result.error, expected_result.error);
        ASSERT_EQUAL(result.count, expected_result.count);
      }
    }
  }
}

TEST(base64_decode_partial_cases) {
  std::vector<std::pair<std::string, simdutf::result>> test_cases = {
      {"ZXhhZg", {simdutf::error_code::SUCCESS, 4}},
      {"ZXhhZg                                                                ",
       {simdutf::error_code::SUCCESS, 4}},
      {"                                                                ZXhhZg",
       {simdutf::error_code::SUCCESS, 68}},
  };
  std::vector<char> buffer(3);
  for (auto &p : test_cases) {
    auto input = p.first;
    auto expected_result = p.second;
    size_t written = buffer.size();
    auto result = simdutf::base64_to_binary_safe(
        input.data(), input.size(), buffer.data(), written,
        simdutf::base64_default,
        simdutf::last_chunk_handling_options::stop_before_partial);
    ASSERT_EQUAL(result.error, expected_result.error);
    ASSERT_EQUAL(result.count, expected_result.count);
  }
}

TEST(base64_decode_strict_cases) {
  std::vector<std::pair<std::string, uint64_t>> test_cases = {
      {"ZXhhZg==", simdutf::error_code::SUCCESS},
      {"YWE=", simdutf::error_code::SUCCESS},
      {"YWF=", simdutf::error_code::BASE64_EXTRA_BITS},
      {"ZXhhZh==", simdutf::error_code::BASE64_EXTRA_BITS},
      {"ZXhhZg", simdutf::error_code::BASE64_INPUT_REMAINDER},
      {"ZXhhZh", simdutf::error_code::BASE64_INPUT_REMAINDER},
      {"Z   X  h  h   Z h =   =", simdutf::error_code::BASE64_EXTRA_BITS},
      {"ZX  h  hZg", simdutf::error_code::BASE64_INPUT_REMAINDER},
      {"ZXh  hZ  h", simdutf::error_code::BASE64_INPUT_REMAINDER},
  };
  std::vector<char> buffer(1024);
  for (auto &p : test_cases) {
    auto input = p.first;
    auto expected_error = p.second;
    simdutf::result result = implementation.base64_to_binary(
        input.data(), input.size(), buffer.data(), simdutf::base64_default,
        simdutf::last_chunk_handling_options::strict);
    ASSERT_EQUAL(result.error, expected_error);
    size_t written = buffer.size();
    result = simdutf::base64_to_binary_safe(
        input.data(), input.size(), buffer.data(), written,
        simdutf::base64_default, simdutf::last_chunk_handling_options::strict);
    ASSERT_EQUAL(result.error, expected_error);
  }
}

TEST(base64_decode_strict_cases_length) {
  std::vector<std::pair<std::string, simdutf::result>> test_cases = {
      {"ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
       "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddzzz=",
       {simdutf::error_code::BASE64_EXTRA_BITS, 131}},
  };
  std::vector<char> buffer(1024);
  for (auto &p : test_cases) {
    auto input = p.first;
    auto expected_result = p.second;
    simdutf::result result = implementation.base64_to_binary(
        input.data(), input.size(), buffer.data(), simdutf::base64_default,
        simdutf::last_chunk_handling_options::strict);
    ASSERT_EQUAL(result.error, expected_result.error);
    ASSERT_EQUAL(result.count, expected_result.count);
    size_t written = buffer.size();
    result = simdutf::base64_to_binary_safe(
        input.data(), input.size(), buffer.data(), written,
        simdutf::base64_default, simdutf::last_chunk_handling_options::strict);
    ASSERT_EQUAL(result.error, expected_result.error);
    ASSERT_EQUAL(result.count, expected_result.count);
  }
}

// https://bugs.webkit.org/show_bug.cgi?id=290829
TEST(issue_webkit_290829) {
  std::string data = "MjYyZg===";
  std::vector<char> output(5); // 5 is part of the issue
  std::vector<uint8_t> expected = {0x32, 0x36, 0x32};

  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::loose,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    std::fill(output.begin(), output.end(), 0);
    const auto r1 =
        implementation.base64_to_binary(data.data(), data.size(), output.data(),
                                        simdutf::base64_default, option);
    ASSERT_EQUAL(r1.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
    ASSERT_EQUAL(r1.count, 6);
  }

  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::loose,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    std::fill(output.begin(), output.end(), 0);
    size_t back_length = output.size();
    constexpr bool decode_up_to_bad_char = true;
    auto r = simdutf::base64_to_binary_safe(
        data.data(), data.size(), output.data(), back_length,
        simdutf::base64_default, option, decode_up_to_bad_char);

    ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
    ASSERT_EQUAL(r.count, 6);
    ASSERT_EQUAL(back_length, 3);
    ASSERT_BYTES_EQUAL(output, expected, 3);
  }
}

// https://bugs.webkit.org/show_bug.cgi?id=290829
TEST(issue_webkit_utf16_290829) {
  std::string data = "MjYyZg===";
  std::vector<char> output(5); // 5 is part of the issue
  std::vector<uint16_t> expected = {0x32, 0x36, 0x32};

  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::loose,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    std::fill(output.begin(), output.end(), 0);
    const auto r1 =
        implementation.base64_to_binary(data.data(), data.size(), output.data(),
                                        simdutf::base64_default, option);
    ASSERT_EQUAL(r1.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
    ASSERT_EQUAL(r1.count, 6);
  }

  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::loose,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    std::fill(output.begin(), output.end(), 0);
    size_t back_length = output.size();
    constexpr bool decode_up_to_bad_char = true;
    auto r = simdutf::base64_to_binary_safe(
        data.data(), data.size(), output.data(), back_length,
        simdutf::base64_default, option, decode_up_to_bad_char);

    ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
    ASSERT_EQUAL(r.count, 6);
    ASSERT_EQUAL(back_length, 3);
    ASSERT_BYTES_EQUAL(output, expected, 3);
  }
}

// https://bugs.webkit.org/show_bug.cgi?id=290829

TEST(issue_webkit_utf16_290829_bad_char) {
  std::string data(1024, 'A');
  std::vector<char> expected(1024 / 4 * 3, 0);
  std::vector<char> output(1024 / 4 * 3);
  for (size_t invalid = 0; invalid < data.size(); invalid++) {
    data[invalid] = '?'; // invalid
    for (auto option :
         {simdutf::last_chunk_handling_options::strict,
          simdutf::last_chunk_handling_options::loose,
          simdutf::last_chunk_handling_options::stop_before_partial}) {
      std::fill(output.begin(), output.end(), 255);
      size_t back_length = output.size();
      constexpr bool decode_up_to_bad_char = true;
      auto r = simdutf::base64_to_binary_safe(
          data.data(), data.size(), output.data(), back_length,
          simdutf::base64_default, option, decode_up_to_bad_char);

      ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
      ASSERT_EQUAL(r.count, invalid);
      size_t expected_length = invalid / 4 * 3;
      ASSERT_EQUAL(back_length, expected_length);
      ASSERT_BYTES_EQUAL(output, expected, expected_length);
    }
    data[invalid] = 'A'; // valid
  }
}

TEST(issue_webkit_utf16_290829_example) {
  std::string data(1024, 'A');
  std::vector<char> expected(1024 / 4 * 3, 0);
  std::vector<char> output(1024 / 4 * 3);
  for (size_t invalid = 0; invalid < data.size(); invalid++) {
    data[invalid] = '?'; // invalid
    for (auto option :
         {simdutf::last_chunk_handling_options::strict,
          simdutf::last_chunk_handling_options::loose,
          simdutf::last_chunk_handling_options::stop_before_partial}) {
      std::fill(output.begin(), output.end(), 255);
      size_t back_length = output.size();
      constexpr bool decode_up_to_bad_char = true;
      auto r = simdutf::base64_to_binary_safe(
          data.data(), data.size(), output.data(), back_length,
          simdutf::base64_default, option, decode_up_to_bad_char);

      ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
      ASSERT_EQUAL(r.count, invalid);
      size_t expected_length = invalid / 4 * 3;
      ASSERT_EQUAL(back_length, expected_length);
      ASSERT_BYTES_EQUAL(output, expected, expected_length);
    }
    data[invalid] = 'A'; // valid
  }
}

TEST(issue_single_bad16) {
  std::vector<char16_t> data = {0x3f};
  ASSERT_EQUAL(data.size(), 1);
  size_t outlen = implementation.maximal_binary_length_from_base64(data.data(),
                                                                   data.size());
  std::vector<char> out(outlen);
  const auto r = implementation.base64_to_binary(
      (const char *)data.data(), data.size(), out.data(),
      simdutf::base64_url_with_padding,
      simdutf::last_chunk_handling_options::strict);
  ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
  ASSERT_EQUAL(r.count, 0);
}

TEST(issue_615) {
  const std::vector<char> data{' ', '=', '='};
  std::vector<char> output(100);
  const auto r1 =
      implementation.base64_to_binary(data.data(), data.size(), output.data(),
                                      simdutf::base64_default, simdutf::strict);
  ASSERT_EQUAL(r1.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
  ASSERT_EQUAL(r1.count, 0);
  const auto r2 = implementation.base64_to_binary(
      data.data() + 1, data.size() - 1, output.data(), simdutf::base64_default,
      simdutf::strict);
  ASSERT_EQUAL(r2.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
  ASSERT_EQUAL(r2.count, 0);
  const auto r3 = implementation.base64_to_binary(
      data.data(), data.size(), output.data(), simdutf::base64_default,
      simdutf::stop_before_partial);
  ASSERT_EQUAL(r3.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(r3.count, 0);
  const auto r4 = implementation.base64_to_binary(
      data.data() + 1, data.size() - 1, output.data(), simdutf::base64_default,
      simdutf::stop_before_partial);
  ASSERT_EQUAL(r4.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(r4.count, 0);
}

TEST(issue_kkk) {
  std::vector<char> data = {
      0x20, 0x20, 0x20, 0x20, 0x20, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b,
      0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x4b, 0x20, 0x20, 0x20,
      0x63};
  ASSERT_EQUAL(data.size(), 193);
  size_t outlen = implementation.maximal_binary_length_from_base64(data.data(),
                                                                   data.size());
  std::vector<char> out(outlen);
  const auto r = implementation.base64_to_binary(
      (const char *)data.data(), data.size(), out.data(),
      simdutf::base64_url_with_padding,
      simdutf::last_chunk_handling_options::strict);
  ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
  ASSERT_EQUAL(r.count, 132);
}

TEST(issue_520) {
  std::vector<unsigned char> data{
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 12, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 82,
  };
  std::vector<char> out(48);

  const auto r =
      implementation.base64_to_binary((const char *)data.data(), data.size(),
                                      out.data(), simdutf::base64_default);
  ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
  ASSERT_EQUAL(r.count, 0);
}

TEST(base64_decode_complete_input) {
  std::vector<unsigned char> input_data = {0x54, 0x57, 0x46,
                                           0x75}; // "TWFu" Base64 for "Man"
  std::vector<char> expected_output = {'M', 'a', 'n'};
  std::vector<char> output_buffer(expected_output.size());

  // Test with all last_chunk_handling_options
  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::loose,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    auto result = implementation.base64_to_binary(
        reinterpret_cast<const char *>(input_data.data()), input_data.size(),
        output_buffer.data(), simdutf::base64_default, option);

    ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(result.count, expected_output.size());
    ASSERT_TRUE(
        (std::equal(output_buffer.begin(), output_buffer.begin() + result.count,
                    expected_output.begin())));
  }
}

// https://github.com/WebKit/WebKit/blob/18fad22e2078542316a576989676d31cfd08d777/JSTests/stress/uint8array-fromBase64.js#L135
TEST(base64_decode_webkit_cases) {
  std::vector<std::pair<std::string, std::vector<uint8_t>>> test_cases = {
      {"", {}},
      {"AA==", {0}},
      {"AQ==", {1}},
      {"gA==", {128}},
      {"/g==", {254}},
      {"/w==", {255}},
      {"AAE=", {0, 1}},
      {"/v8=", {254, 255}},
      {"AAGA/v8=", {0, 1, 128, 254, 255}},
      {"  ", {}},
      {"  A  A  =  =  ", {0}},
      {"  A  Q  =  =  ", {1}},
      {"  g  A  =  =  ", {128}},
      {"  /  g  =  =  ", {254}},
      {"  /  w  =  =  ", {255}},
      {"  A  A  E  =  ", {0, 1}},
      {"  /  v  8  =  ", {254, 255}},
      {"  A  A  G  A  /  v  8  =  ", {0, 1, 128, 254, 255}}};

  // Test with all last_chunk_handling_options
  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::loose,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    for (const std::pair<std::string, std::vector<uint8_t>> &t : test_cases) {
      auto input_data = t.first;
      auto expected_output = t.second;
      std::vector<uint8_t> output_buffer(
          implementation.maximal_binary_length_from_base64(input_data.data(),
                                                           input_data.size()));
      auto result = implementation.base64_to_binary(
          input_data.data(), input_data.size(),
          reinterpret_cast<char *>(output_buffer.data()),
          simdutf::base64_default, option);
      ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(result.count, expected_output.size());
      output_buffer.resize(result.count);
      ASSERT_TRUE(output_buffer == expected_output);
    }
  }

  // Test with all last_chunk_handling_options
  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::loose,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    for (const std::pair<std::string, std::vector<uint8_t>> &t : test_cases) {
      auto input_data = t.first;
      auto expected_output = t.second;
      std::vector<uint8_t> output_buffer(
          implementation.maximal_binary_length_from_base64(input_data.data(),
                                                           input_data.size()));
      size_t written = output_buffer.size();
      auto result = simdutf::base64_to_binary_safe(
          input_data.data(), input_data.size(),
          reinterpret_cast<char *>(output_buffer.data()), written,
          simdutf::base64_default, option);
      ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(written, expected_output.size());
      output_buffer.resize(written);
      ASSERT_TRUE(output_buffer == expected_output);
    }
  }
}

// https://github.com/WebKit/WebKit/blob/18fad22e2078542316a576989676d31cfd08d777/JSTests/stress/uint8array-fromBase64.js#L206
TEST(base64_decode_webkit_more_cases) {
  std::vector<std::string> test_cases = {
      "AA",       "  A  A  ",    "AQ",       "  A  Q  ",   "gA",
      "  g  A  ", "/g",          "  /  g  ", "/w",         "  /  w  ",
      "AAE",      "  A  A  E  ", "/v8",      "  /  v  8  "};
  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    for (const std::string &input_data : test_cases) {
      std::vector<uint8_t> output_buffer(
          implementation.maximal_binary_length_from_base64(input_data.data(),
                                                           input_data.size()));
      auto result = implementation.base64_to_binary(
          input_data.data(), input_data.size(),
          reinterpret_cast<char *>(output_buffer.data()),
          simdutf::base64_default, option);
      if (option == simdutf::last_chunk_handling_options::strict) {
        ASSERT_EQUAL(result.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
      } else {
        ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(result.count, 0);
      }
    }
  }
  for (auto option :
       {simdutf::last_chunk_handling_options::strict,
        simdutf::last_chunk_handling_options::stop_before_partial}) {
    for (const std::string &input_data : test_cases) {
      std::vector<uint8_t> output_buffer(
          implementation.maximal_binary_length_from_base64(input_data.data(),
                                                           input_data.size()));
      size_t written = output_buffer.size();
      auto result = simdutf::base64_to_binary_safe(
          input_data.data(), input_data.size(),
          reinterpret_cast<char *>(output_buffer.data()), written,
          simdutf::base64_default, option);
      if (option == simdutf::last_chunk_handling_options::strict) {
        ASSERT_EQUAL(result.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
      } else {
        ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(written, 0);
      }
    }
  }
}

TEST(base64_decode_webkit_like_but_random_more_cases) {
  for (size_t len = 1; len <= 2048; len++) {
    for (size_t trial = 0; trial < 10; trial++) {
      std::vector<char> source(len, 0);
      std::vector<char> buffer;
      buffer.resize(implementation.base64_length_from_binary(len));
      std::mt19937 gen((std::mt19937::result_type)(seed));
      std::uniform_int_distribution<int> byte_generator{0, 255};
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      simdutf_maybe_unused const size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      for (size_t removed = 1; !buffer.empty() && removed <= 2; removed++) {
        buffer.pop_back();
        for (auto option :
             {simdutf::last_chunk_handling_options::strict,
              simdutf::last_chunk_handling_options::stop_before_partial}) {
          std::vector<uint8_t> output_buffer(
              implementation.maximal_binary_length_from_base64(buffer.data(),
                                                               buffer.size()));
          auto result = implementation.base64_to_binary(
              buffer.data(), buffer.size(),
              reinterpret_cast<char *>(output_buffer.data()),
              simdutf::base64_default, option);
          if (option == simdutf::last_chunk_handling_options::strict) {
            ASSERT_EQUAL(result.error,
                         simdutf::error_code::BASE64_INPUT_REMAINDER);
          } else {
            ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
            ASSERT_EQUAL(result.count, (len - 1) / 3 * 3);
          }
        }
        for (auto option :
             {simdutf::last_chunk_handling_options::strict,
              simdutf::last_chunk_handling_options::stop_before_partial}) {
          std::vector<uint8_t> output_buffer(
              implementation.maximal_binary_length_from_base64(buffer.data(),
                                                               buffer.size()));
          size_t written = output_buffer.size();
          auto result = simdutf::base64_to_binary_safe(
              buffer.data(), buffer.size(),
              reinterpret_cast<char *>(output_buffer.data()), written,
              simdutf::base64_default, option);
          if (option == simdutf::last_chunk_handling_options::strict) {
            ASSERT_EQUAL(result.error,
                         simdutf::error_code::BASE64_INPUT_REMAINDER);
          } else {
            ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
            ASSERT_EQUAL(written, (len - 1) / 3 * 3);
          }
        }
      }
    }
  }
}

TEST(base64_decode_webkit_like_but_random_with_spaces_more_cases) {
  for (size_t len = 1; len <= 2048; len++) {
    for (size_t trial = 0; trial < 20; trial++) {
      std::vector<char> source(len, 0);
      std::vector<char> buffer;
      buffer.resize(implementation.base64_length_from_binary(len));
      std::mt19937 gen((std::mt19937::result_type)(seed));
      std::uniform_int_distribution<int> byte_generator{0, 255};
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      simdutf_maybe_unused const size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer = add_simple_spaces(buffer, gen, 5 + len / 4);
      auto is_space = [](char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
      };
      for (size_t removed = 1; !buffer.empty() && removed <= 2; removed++) {
        while (is_space(buffer.back())) {
          buffer.pop_back();
        }
        buffer.pop_back();
        for (auto option :
             {simdutf::last_chunk_handling_options::strict,
              simdutf::last_chunk_handling_options::stop_before_partial}) {
          std::vector<uint8_t> output_buffer(
              implementation.maximal_binary_length_from_base64(buffer.data(),
                                                               buffer.size()));
          auto result = implementation.base64_to_binary(
              buffer.data(), buffer.size(),
              reinterpret_cast<char *>(output_buffer.data()),
              simdutf::base64_default, option);
          if (option == simdutf::last_chunk_handling_options::strict) {
            ASSERT_EQUAL(result.error,
                         simdutf::error_code::BASE64_INPUT_REMAINDER);
          } else {
            ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
            ASSERT_EQUAL(result.count, (len - 1) / 3 * 3);
          }
        }
        for (auto option :
             {simdutf::last_chunk_handling_options::strict,
              simdutf::last_chunk_handling_options::stop_before_partial}) {
          std::vector<uint8_t> output_buffer(
              simdutf::maximal_binary_length_from_base64(buffer.data(),
                                                         buffer.size()));
          size_t written = output_buffer.size();
          auto result = simdutf::base64_to_binary_safe(
              buffer.data(), buffer.size(),
              reinterpret_cast<char *>(output_buffer.data()), written,
              simdutf::base64_default, option);
          if (option == simdutf::last_chunk_handling_options::strict) {
            ASSERT_EQUAL(result.error,
                         simdutf::error_code::BASE64_INPUT_REMAINDER);
          } else {
            ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
            ASSERT_EQUAL(written, (len - 1) / 3 * 3);
          }
        }
      }
    }
  }
}

TEST(base64_decode_strict_mode) {
  std::vector<std::pair<std::string, std::string>> test_cases = {
      {"TQ", "M"},            // Length 2 (not multiple of 4)
      {"TWE", "Ma"},          // Length 3 (not multiple of 4)
      {"TWFu", "Man"},        // Length 4 (multiple of 4)
      {"TWF1", "Mau"},        // Length 4 (multiple of 4)
      {"TWFubWFu", "Manman"}, // Length 8 (multiple of 4)
  };
  for (const std::pair<std::string, std::string> &t : test_cases) {
    auto input_data = t.first;
    auto expected_output = t.second;
    std::vector<uint8_t> output_buffer(expected_output.size() +
                                       3); // Add extra space for safety

    auto result = implementation.base64_to_binary(
        input_data.data(), input_data.size(),
        reinterpret_cast<char *>(output_buffer.data()), simdutf::base64_default,
        simdutf::last_chunk_handling_options::strict);

    if (input_data.size() % 4 == 0) {
      // Input length is a multiple of 4, expect success
      ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(result.count, expected_output.size());
      ASSERT_TRUE((std::equal(output_buffer.begin(),
                              output_buffer.begin() + result.count,
                              expected_output.begin())));
    } else {
      // Input length is not a multiple of 4, expect failure in strict mode
      ASSERT_EQUAL(result.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
    }
  }
}

TEST(base64_decode_stop_before_partial) {
  std::vector<std::pair<std::string, std::string>> test_cases = {
      {"TQ", ""},             // Length 2 (no complete blocks)
      {"TWE", ""},            // Length 3 (no complete blocks)
      {"TWFu", "Man"},        // Length 4 (1 complete block)
      {"TWFuTQ", "Man"},      // Length 6 (1 complete block)
      {"TWFuTW", "Man"},      // Length 7 (1 complete block)
      {"TWFuTWFu", "ManMan"}, // Length 8 (2 complete blocks)
  };

  for (const std::pair<std::string, std::string> &t : test_cases) {
    auto input_data = t.first;
    auto expected_output = t.second;
    std::vector<char> output_buffer(expected_output.size() + 3); // Extra space

    auto result = implementation.base64_to_binary(
        input_data.data(), input_data.size(), output_buffer.data(),
        simdutf::base64_default,
        simdutf::last_chunk_handling_options::stop_before_partial);

    ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(result.count, expected_output.size());
    ASSERT_TRUE(
        (std::equal(output_buffer.begin(), output_buffer.begin() + result.count,
                    expected_output.begin())));
  }
}

TEST(issue_520_url) {
  // output differs between implementations for decode
  // impl arm64 got maxbinarylength=48 convertresult=[count=64,
  // error=INVALID_BASE64_CHARACTER] impl fallback got maxbinarylength=48
  // convertresult=[count=0, error=BASE64_INPUT_REMAINDER]

  std::vector<unsigned char> data{
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 12, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 82,
  };
  std::vector<char> out(48);

  const auto r = implementation.base64_to_binary(
      (const char *)data.data(), data.size(), out.data(), simdutf::base64_url);
  ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
  ASSERT_EQUAL(r.count, 0);
}

TEST(issue_511) {
  // 0x7f is not a valid base64 character.
  std::vector<unsigned char> data{
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x7f, 0x57, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5a};
  std::vector<char> out(48);
  const auto r = implementation.base64_to_binary(
      (const char *)data.data(), data.size(), out.data(), simdutf::base64_url);
  ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
  ASSERT_EQUAL(r.count, 12);
}

TEST(issue_509) {
  std::vector<char> data{' ', '='};
  std::vector<char> out(1);
  const auto r = implementation.base64_to_binary(
      data.data(), data.size(), out.data(), simdutf::base64_default);
  ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
  ASSERT_EQUAL(r.count, 1);
}

TEST(issue_502_alt) {
  for (std::size_t nof_equals = 1; nof_equals < 100; ++nof_equals) {
    std::vector<char> data(nof_equals, '=');
    std::vector<char> out(1);
    const auto r = implementation.base64_to_binary(
        data.data(), data.size(), out.data(), simdutf::base64_default);
    ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
    ASSERT_EQUAL(r.count, 0);
  }
}

TEST(issue_504) {
  std::array<char16_t, 1> data{61}; // 61 is the ASCII code for '='
  std::vector<char> out(1);
  const auto r = implementation.base64_to_binary(
      data.data(), data.size(), out.data(), simdutf::base64_default);
  ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
  ASSERT_EQUAL(r.count, 0);
}

TEST(issue_504_8bit) {
  std::array<char, 1> data{61}; // 61 is the ASCII code for '='
  std::vector<char> out(1);
  const auto r = implementation.base64_to_binary(
      data.data(), data.size(), out.data(), simdutf::base64_default);
  ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
  ASSERT_EQUAL(r.count, 0);
}

TEST(issue_502) {
  std::array<char, 1> data{'='};
  std::vector<char> out(1);
  const auto r = implementation.base64_to_binary(
      data.data(), data.size(), out.data(), simdutf::base64_default);
  ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
  ASSERT_EQUAL(r.count, 0);
}

TEST(issue_503) {
  std::array<char16_t, 1> data{15626}; // 0x3D0A
  std::vector<char> out(1);
  const auto r = implementation.base64_to_binary(
      data.data(), data.size(), out.data(), simdutf::base64_default);
  ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
  ASSERT_EQUAL(r.count, 0);
}

TEST(decode_non_ascii_utf16) {
  std::vector<std::u16string> cases = {u"Zg\u2009=="};
  std::vector<simdutf::error_code> codes = {
      simdutf::error_code::INVALID_BASE64_CHARACTER};
  std::vector<size_t> counts = {2};

  for (size_t i = 0; i < cases.size(); i++) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        cases[i].data(), cases[i].size()));
    simdutf::result r = implementation.base64_to_binary(
        cases[i].data(), cases[i].size(), buffer.data());
    ASSERT_EQUAL(r.error, codes[i]);
    ASSERT_EQUAL(r.count, counts[i]);
    size_t len = buffer.size();
    r = simdutf::base64_to_binary_safe(cases[i].data(), cases[i].size(),
                                       buffer.data(), len);
    ASSERT_EQUAL(r.error, codes[i]);
    ASSERT_EQUAL(r.count, counts[i]);
  }
}

TEST(decode_non_ascii) {
  std::vector<std::string> cases = {"Zg\u2009=="};
  std::vector<simdutf::error_code> codes = {
      simdutf::error_code::INVALID_BASE64_CHARACTER};
  std::vector<size_t> counts = {2};

  for (size_t i = 0; i < cases.size(); i++) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        cases[i].data(), cases[i].size()));
    simdutf::result r = implementation.base64_to_binary(
        cases[i].data(), cases[i].size(), buffer.data());
    ASSERT_EQUAL(r.error, codes[i]);
    ASSERT_EQUAL(r.count, counts[i]);
    size_t len = buffer.size();
    r = simdutf::base64_to_binary_safe(cases[i].data(), cases[i].size(),
                                       buffer.data(), len);
    ASSERT_EQUAL(r.error, codes[i]);
    ASSERT_EQUAL(r.count, counts[i]);
  }
}

TEST(decode_base64_cases) {
  std::vector<std::vector<char>> cases = {{0x53, 0x53}};
  std::vector<simdutf::error_code> codes = {simdutf::error_code::SUCCESS};
  std::vector<size_t> counts = {1};

  for (size_t i = 0; i < cases.size(); i++) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        cases[i].data(), cases[i].size()));
    simdutf::result r = implementation.base64_to_binary(
        cases[i].data(), cases[i].size(), buffer.data());
    ASSERT_EQUAL(r.error, codes[i]);
    ASSERT_EQUAL(r.count, counts[i]);
  }
}

namespace cases {
const std::vector<std::pair<std::string, std::string>> whitespaces = {
    {"abcd", " Y\fW\tJ\njZ A=\r= "},
};

const std::vector<std::pair<std::string, std::string>> simple = {
    {"Hello, World!", "SGVsbG8sIFdvcmxkIQ=="},
    {"GeeksforGeeks", "R2Vla3Nmb3JHZWVrcw=="},
    {"123456", "MTIzNDU2"},
    {"Base64 Encoding", "QmFzZTY0IEVuY29kaW5n"},
    {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ "
     "PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k",
     "IVJ+SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c+"
     "fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNTk"
     "tWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};

const std::vector<std::pair<std::string, std::string>> no_padding = {
    {"Hello, World!", "SGVsbG8sIFdvcmxkIQ"},
    {"GeeksforGeeks", "R2Vla3Nmb3JHZWVrcw"},
    {"123456", "MTIzNDU2"},
    {"Base64 Encoding", "QmFzZTY0IEVuY29kaW5n"},
    {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ "
     "PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k",
     "IVJ+SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c+"
     "fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNTk"
     "tWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};

const std::vector<std::pair<std::string, std::string>> simple_url = {
    {"Hello, World!", "SGVsbG8sIFdvcmxkIQ"},
    {"GeeksforGeeks", "R2Vla3Nmb3JHZWVrcw"},
    {"123456", "MTIzNDU2"},
    {"Base64 Encoding", "QmFzZTY0IEVuY29kaW5n"},
    {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ "
     "PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k",
     "IVJ-SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c-"
     "fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNTk"
     "tWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};

const std::vector<std::pair<std::string, std::string>> simple_url_with_padding =
    {{"Hello, World!", "SGVsbG8sIFdvcmxkIQ=="},
     {"GeeksforGeeks", "R2Vla3Nmb3JHZWVrcw=="},
     {"123456", "MTIzNDU2"},
     {"Base64 Encoding", "QmFzZTY0IEVuY29kaW5n"},
     {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ "
      "PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k",
      "IVJ-SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c-"
      "fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNTk"
      "tWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};
} // namespace cases

namespace cases16 {
const std::vector<std::pair<std::string, std::u16string>> simple_with_padding =
    {{"Hello, World!", u"SGVsbG8sIFdvcmxkIQ=="},
     {"GeeksforGeeks", u"R2Vla3Nmb3JHZWVrcw=="},
     {"123456", u"MTIzNDU2"},
     {"Base64 Encoding", u"QmFzZTY0IEVuY29kaW5n"},
     {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ "
      "PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k",
      u"IVJ+SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c+"
      u"fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNT"
      u"ktWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};

const std::vector<std::pair<std::string, std::u16string>>
    simple_url_without_padding = {
        {"Hello, World!", u"SGVsbG8sIFdvcmxkIQ"},
        {"GeeksforGeeks", u"R2Vla3Nmb3JHZWVrcw"},
        {"123456", u"MTIzNDU2"},
        {"Base64 Encoding", u"QmFzZTY0IEVuY29kaW5n"},
        {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ "
         "PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k",
         u"IVJ-SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c-"
         u"fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVy"
         u"NT"
         u"ktWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};
} // namespace cases16

TEST(complete_decode_base64_cases) {
  for (const auto &p : cases::whitespaces) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    simdutf::result r = implementation.base64_to_binary(
        p.second.data(), p.second.size(), buffer.data());
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.first.size());
    for (size_t i = 0; i < r.count; i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

TEST(complete_safe_decode_base64_cases) {
  for (const auto &p : cases::whitespaces) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    size_t bufsize = buffer.size();
    simdutf::result r = simdutf::base64_to_binary_safe(
        p.second.data(), p.second.size(), buffer.data(), bufsize);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(bufsize, p.first.size());
    for (size_t i = 0; i < bufsize; i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
#if SIMDUTF_ATOMIC_REF
    bufsize = buffer.size();
    r = simdutf::atomic_base64_to_binary_safe(p.second.data(), p.second.size(),
                                              buffer.data(), bufsize);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(bufsize, p.first.size());
    for (size_t i = 0; i < bufsize; i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
#endif
  }
}

TEST(complete_safe_decode_base64url_cases) {
  for (const auto &p : cases::whitespaces) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    size_t bufsize = buffer.size();
    simdutf::result r = simdutf::base64_to_binary_safe(
        p.second.data(), p.second.size(), buffer.data(), bufsize,
        simdutf::base64_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(bufsize, p.first.size());
    for (size_t i = 0; i < bufsize; i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
#if SIMDUTF_ATOMIC_REF
    bufsize = buffer.size();
    r = simdutf::base64_to_binary_safe(p.second.data(), p.second.size(),
                                       buffer.data(), bufsize,
                                       simdutf::base64_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(bufsize, p.first.size());
    for (size_t i = 0; i < bufsize; i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
#endif
  }
}

TEST(encode_base64_cases) {
  for (const auto &p : cases::simple) {
    std::vector<char> buffer(
        implementation.base64_length_from_binary(p.first.size()));
    ASSERT_EQUAL(buffer.size(), p.second.size());
    size_t s = implementation.binary_to_base64(p.first.data(), p.first.size(),
                                               buffer.data());
    ASSERT_EQUAL(s, p.second.size());
    ASSERT_EQUAL(std::string(buffer.data(), buffer.size()), p.second);
  }
}

TEST(decode_base64_simple) {
  for (const auto &p : cases::simple) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    simdutf::result r = implementation.base64_to_binary(
        p.second.data(), p.second.size(), buffer.data());
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

TEST(safe_decode_base64_simple) {
  for (const auto &p : cases::simple) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    size_t length = buffer.size();
    simdutf::result r = simdutf::base64_to_binary_safe(
        p.second.data(), p.second.size(), buffer.data(), length);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.second.size());
    ASSERT_EQUAL(length, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

TEST(encode_base64_cases_no_padding) {
  for (const auto &p : cases::no_padding) {
    std::vector<char> buffer(implementation.base64_length_from_binary(
        p.first.size(), simdutf::base64_default_no_padding));
    ASSERT_EQUAL(buffer.size(), p.second.size());
    size_t s = implementation.binary_to_base64(
        p.first.data(), p.first.size(), buffer.data(),
        simdutf::base64_default_no_padding);
    ASSERT_EQUAL(s, p.second.size());
    ASSERT_EQUAL(std::string(buffer.data(), buffer.size()), p.second);
  }
}

#if SIMDUTF_BASE64URL_TESTS

TEST(encode_base64url_simple) {
  for (const auto &p : cases::simple_url) {
    std::vector<char> buffer(implementation.base64_length_from_binary(
        p.first.size(), simdutf::base64_url));
    ASSERT_EQUAL(buffer.size(), p.second.size());
    size_t s = implementation.binary_to_base64(
        p.first.data(), p.first.size(), buffer.data(), simdutf::base64_url);
    ASSERT_EQUAL(s, p.second.size());
    if (std::string(buffer.data(), buffer.size()) != p.second) {
      printf("difference:\n");
      printf(" %.*s\n", (int)s, buffer.data());
      printf(" %.*s\n", (int)s, p.second.data());
    }
    ASSERT_EQUAL(std::string(buffer.data(), buffer.size()), p.second);
  }
}

TEST(decode_base64url) {
  for (const auto &p : cases::simple_url) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    simdutf::result r = implementation.base64_to_binary(
        p.second.data(), p.second.size(), buffer.data(), simdutf::base64_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

TEST(safe_decode_base64url) {
  for (const auto &p : cases::simple_url) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    size_t length = buffer.size();
    simdutf::result r = simdutf::base64_to_binary_safe(
        p.second.data(), p.second.size(), buffer.data(), length,
        simdutf::base64_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.second.size());
    ASSERT_EQUAL(length, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

TEST(encode_base64url_with_padding_cases) {
  for (const auto &p : cases::simple_url_with_padding) {
    std::vector<char> buffer(implementation.base64_length_from_binary(
        p.first.size(), simdutf::base64_url_with_padding));
    ASSERT_EQUAL(buffer.size(), p.second.size());
    size_t s = implementation.binary_to_base64(
        p.first.data(), p.first.size(), buffer.data(),
        simdutf::base64_url_with_padding);
    ASSERT_EQUAL(s, p.second.size());
    if (std::string(buffer.data(), buffer.size()) != p.second) {
      printf("difference:\n");
      printf(" %.*s\n", (int)s, buffer.data());
      printf(" %.*s\n", (int)s, p.second.data());
    }
    ASSERT_EQUAL(std::string(buffer.data(), buffer.size()), p.second);
  }
}

#endif

TEST(decode_base64_cases_16) {
  for (const auto &p : cases16::simple_with_padding) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    simdutf::result r = implementation.base64_to_binary(
        p.second.data(), p.second.size(), buffer.data());
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.first.size());
    ASSERT_BYTES_EQUAL(buffer, p.first, r.count);
  }
}

TEST(safe_decode_base64_cases_16) {
  for (const auto &p : cases16::simple_with_padding) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    size_t length = buffer.size();
    simdutf::result r = simdutf::base64_to_binary_safe(
        p.second.data(), p.second.size(), buffer.data(), length);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.second.size());
    ASSERT_EQUAL(length, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

#if SIMDUTF_BASE64URL_TESTS

TEST(decode_base64url_cases_16) {
  for (const auto &p : cases16::simple_url_without_padding) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    simdutf::result r = implementation.base64_to_binary(
        p.second.data(), p.second.size(), buffer.data(), simdutf::base64_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

TEST(safe_decode_base64url_cases_16) {
  for (const auto &p : cases16::simple_url_without_padding) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    size_t length = buffer.size();
    simdutf::result r = simdutf::base64_to_binary_safe(
        p.second.data(), p.second.size(), buffer.data(), length,
        simdutf::base64_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.second.size());
    ASSERT_EQUAL(length, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

#endif

TEST(roundtrip_base64) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(len));
      simdutf::result r =
          implementation.base64_to_binary(buffer.data(), size, back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);

      // Test with all last_chunk_handling_options
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        r = implementation.base64_to_binary(buffer.data(), size, back.data(),
                                            simdutf::base64_default, option);
        ASSERT_TRUE((size % 4) == 0);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(r.count, len);
        ASSERT_TRUE(back == source);
      }
    }
  }
}

TEST(roundtrip_base64_16) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    std::vector<char16_t> buffer16;

    buffer.resize(implementation.base64_length_from_binary(len));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size);
      buffer16.resize(buffer.size());
      for (size_t i = 0; i < buffer.size(); i++) {
        buffer16[i] = buffer[i];
      }
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(len));
      simdutf::result r =
          implementation.base64_to_binary(buffer16.data(), size, back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);

      // Test with all last_chunk_handling_options
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        r = implementation.base64_to_binary(buffer.data(), size, back.data(),
                                            simdutf::base64_default, option);
        ASSERT_TRUE((size % 4) == 0);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(r.count, len);
        ASSERT_TRUE(back == source);
      }
    }
  }
}

#if SIMDUTF_BASE64URL_TESTS

TEST(roundtrip_base64url) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(
        implementation.base64_length_from_binary(len, simdutf::base64_url));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data(), simdutf::base64_url);
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(
                             len, simdutf::base64_url));
      simdutf::result r = implementation.base64_to_binary(
          buffer.data(), size, back.data(), simdutf::base64_url);
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);

      // Test with all last_chunk_handling_options
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        r = implementation.base64_to_binary(buffer.data(), size, back.data(),
                                            simdutf::base64_url, option);
        if ((size % 4) == 0) {
          ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
          ASSERT_EQUAL(r.count, len);
          ASSERT_TRUE(back == source);
        } else {
          if (option == simdutf::last_chunk_handling_options::strict) {
            ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
          } else if (option == simdutf::last_chunk_handling_options::loose) {
            ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
            ASSERT_EQUAL(r.count, len);
            ASSERT_TRUE(back == source);
          } else if (option == simdutf::last_chunk_handling_options::
                                   stop_before_partial) {
            ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
            ASSERT_EQUAL(r.count, len / 3 * 3);
            ASSERT_TRUE(std::equal(back.begin(), back.begin() + len / 3 * 3,
                                   source.begin()));
          }
        }
      }
    }
  }
}

TEST(roundtrip_base64url_16) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    std::vector<char16_t> buffer16;

    buffer.resize(
        implementation.base64_length_from_binary(len, simdutf::base64_url));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data(), simdutf::base64_url);
      buffer.resize(size);
      buffer16.resize(buffer.size());
      for (size_t i = 0; i < buffer.size(); i++) {
        buffer16[i] = buffer[i];
      }
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(
                             len, simdutf::base64_url));
      simdutf::result r = implementation.base64_to_binary(
          buffer16.data(), size, back.data(), simdutf::base64_url);
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        r = implementation.base64_to_binary(buffer.data(), size, back.data(),
                                            simdutf::base64_url, option);
        if ((size % 4) == 0) {
          ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
          ASSERT_EQUAL(r.count, len);
          ASSERT_TRUE(back == source);
        } else {
          if (option == simdutf::last_chunk_handling_options::strict) {
            ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
          } else if (option == simdutf::last_chunk_handling_options::loose) {
            ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
            ASSERT_EQUAL(r.count, len);
            ASSERT_TRUE(back == source);
          } else if (option == simdutf::last_chunk_handling_options::
                                   stop_before_partial) {
            ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
            ASSERT_EQUAL(r.count, len / 3 * 3);
            ASSERT_TRUE(std::equal(back.begin(), back.begin() + len / 3 * 3,
                                   source.begin()));
          }
        }
      }
    }
  }
}
#endif

TEST(bad_padding_base64) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len) + 1);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      size_t padding = 0;
      if (size > 0 && buffer[size - 1] == '=') {
        padding++;
        if (size > 1 && buffer[size - 2] == '=') {
          padding++;
        }
      }
      buffer.resize(size);
      ; // in case we need
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      if (padding == 1) {
        // adding padding should break
        buffer.push_back('=');
        for (size_t i = 0; i < 5; i++) {
          add_space(buffer, gen);
        }
        simdutf::result r = simdutf::base64_to_binary(
            buffer.data(), buffer.size(), back.data());
        ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
      } else if (padding == 2) {
        // adding padding should break
        {
          auto copy = buffer;
          copy.push_back('=');
          for (size_t i = 0; i < 5; i++) {
            add_space(copy, gen);
          }
          simdutf::result r =
              simdutf::base64_to_binary(copy.data(), copy.size(), back.data());
          ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
        }
        // removing padding should break
        {
          auto copy = buffer;
          copy.resize(copy.size() - 1);
          for (size_t i = 0; i < 5; i++) {
            add_space(copy, gen);
          }
          simdutf::result r =
              simdutf::base64_to_binary(copy.data(), copy.size(), back.data());
          ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
        }

      } else {
        {
          auto copy = buffer;
          copy.push_back('=');
          for (size_t i = 0; i < 5; i++) {
            add_space(copy, gen);
          }
          simdutf::result r =
              simdutf::base64_to_binary(copy.data(), copy.size(), back.data());
          ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
        }
      }
    }
  }
}
TEST(doomed_base64_roundtrip) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size);
      size_t location = add_garbage(buffer, gen, to_base64_value);
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      simdutf::result r =
          simdutf::base64_to_binary(buffer.data(), buffer.size(), back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
      ASSERT_EQUAL(r.count, location);
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        size_t back_length = back.size();
        r = simdutf::base64_to_binary_safe(buffer.data(), buffer.size(),
                                           back.data(), back_length,
                                           simdutf::base64_default, option);
        ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
        ASSERT_EQUAL(r.count, location);
      }
    }
  }
}

TEST(doomed_truncated_base64_roundtrip) {
  for (size_t len = 1; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      buffer.resize(implementation.base64_length_from_binary(len));
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size - 3);
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      for (auto option : {simdutf::last_chunk_handling_options::loose,
                          simdutf::last_chunk_handling_options::strict}) {
        simdutf::result r = implementation.base64_to_binary(
            buffer.data(), buffer.size(), back.data(), simdutf::base64_default,
            option);
        ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
        ASSERT_EQUAL(r.count, (size - 4) / 4 * 3);
        size_t back_length = back.size();
        r = simdutf::base64_to_binary_safe(buffer.data(), buffer.size(),
                                           back.data(), back_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
        ASSERT_EQUAL(r.count, buffer.size());
      }
    }
  }
}

TEST(doomed_truncated_base64_roundtrip_16) {
  for (size_t len = 1; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    std::vector<char16_t> buffer16;
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      buffer.resize(implementation.base64_length_from_binary(len));
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size - 3);
      buffer16.resize(buffer.size());
      for (size_t i = 0; i < buffer.size(); i++) {
        buffer16[i] = buffer[i];
      }
      std::vector<char> back(implementation.maximal_binary_length_from_base64(
          buffer16.data(), buffer16.size()));
      simdutf::result r = implementation.base64_to_binary(
          buffer16.data(), buffer16.size(), back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
      ASSERT_EQUAL(r.count, (size - 4) / 4 * 3);
      size_t back_length = back.size();
      r = simdutf::base64_to_binary_safe(buffer16.data(), buffer16.size(),
                                         back.data(), back_length);
      ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
      ASSERT_EQUAL(r.count, buffer16.size());
    }
  }
}

TEST(roundtrip_base64_16_with_spaces) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    std::vector<char16_t> buffer16;

    buffer.resize(implementation.base64_length_from_binary(len));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size);
      for (size_t i = 0; i < 5; i++) {
        add_space(buffer, gen);
      }
      buffer16.resize(buffer.size());
      for (size_t i = 0; i < buffer.size(); i++) {
        buffer16[i] = buffer[i];
      }
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(len));
      simdutf::result r = implementation.base64_to_binary(
          buffer16.data(), buffer16.size(), back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);
    }
  }
}

TEST(roundtrip_base64_16_with_garbage) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    std::vector<char16_t> buffer16;

    buffer.resize(implementation.base64_length_from_binary(len));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size);
      for (size_t i = 0; i < 5; i++) {
        add_garbage(buffer, gen, to_base64_value);
      }
      buffer16.resize(buffer.size());
      for (size_t i = 0; i < buffer.size(); i++) {
        buffer16[i] = buffer[i];
      }
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(len));
      simdutf::result r = implementation.base64_to_binary(
          buffer16.data(), buffer16.size(), back.data(),
          simdutf::base64_default_accept_garbage);
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);
    }
  }
}

TEST(roundtrip_base64_url_16_with_garbage) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    std::vector<char16_t> buffer16;

    buffer.resize(implementation.base64_length_from_binary(len));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data(), simdutf::base64_url);
      buffer.resize(size);
      for (size_t i = 0; i < 5; i++) {
        add_garbage(buffer, gen, to_base64url_value);
      }
      buffer16.resize(buffer.size());
      for (size_t i = 0; i < buffer.size(); i++) {
        buffer16[i] = buffer[i];
      }
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(
                             len, simdutf::base64_url));
      simdutf::result r = implementation.base64_to_binary(
          buffer16.data(), buffer16.size(), back.data(),
          simdutf::base64_url_accept_garbage);
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);
    }
  }
}

TEST(aborted_safe_roundtrip_base64) {
  for (size_t offset = 1; offset <= 16; offset += 3) {
    for (size_t len = offset; len < 1024; len++) {
      std::vector<char> source(len, 0);
      std::vector<char> buffer;
      buffer.resize(implementation.base64_length_from_binary(len));
      std::mt19937 gen((std::mt19937::result_type)(seed));
      std::uniform_int_distribution<int> byte_generator{0, 255};
      for (size_t trial = 0; trial < 10; trial++) {
        for (size_t i = 0; i < len; i++) {
          source[i] = byte_generator(gen);
        }
        size_t size = implementation.binary_to_base64(
            source.data(), source.size(), buffer.data());
        buffer.resize(size);
        std::vector<char> back(simdutf::maximal_binary_length_from_base64(
            buffer.data(), buffer.size()));
        size_t limited_length = len - offset; // intentionally too little
        back.resize(limited_length);
        back.shrink_to_fit();
        simdutf::result r = simdutf::base64_to_binary_safe(
            buffer.data(), buffer.size(), back.data(), limited_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL);
        for (size_t i = 0; i < limited_length; i++) {
          ASSERT_EQUAL(source[i], back[i]);
        }
        // Now let us decode the rest !!!
        size_t input_index = r.count;
        back.resize(simdutf::maximal_binary_length_from_base64(
            buffer.data() + input_index, buffer.size() - input_index));
        size_t second_length = back.size();
        r = simdutf::base64_to_binary_safe(buffer.data() + input_index,
                                           buffer.size() - input_index,
                                           back.data(), second_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        back.resize(second_length);
        ASSERT_EQUAL(second_length + limited_length, len);

        for (size_t i = 0; i < second_length; i++) {
          ASSERT_EQUAL(source[i + limited_length], back[i]);
        }
      }
    }
  }
}

TEST(aborted_safe_roundtrip_base64_16) {
  for (size_t offset = 1; offset <= 16; offset += 3) {
    for (size_t len = offset; len < 1024; len++) {
      std::vector<char> source(len, 0);
      std::vector<char> buffer;
      std::vector<char16_t> buffer16;

      buffer.resize(implementation.base64_length_from_binary(len));
      std::vector<char> back(len);
      std::mt19937 gen((std::mt19937::result_type)(seed));
      std::uniform_int_distribution<int> byte_generator{0, 255};
      for (size_t trial = 0; trial < 10; trial++) {
        for (size_t i = 0; i < len; i++) {
          source[i] = byte_generator(gen);
        }
        size_t size = implementation.binary_to_base64(
            source.data(), source.size(), buffer.data());
        buffer.resize(size);
        buffer16.resize(buffer.size());
        for (size_t i = 0; i < buffer.size(); i++) {
          buffer16[i] = buffer[i];
        }
        ASSERT_EQUAL(size, implementation.base64_length_from_binary(len));
        size_t limited_length = len - offset; // intentionally too little
        back.resize(limited_length);
        back.shrink_to_fit();
        simdutf::result r = simdutf::base64_to_binary_safe(
            buffer.data(), buffer.size(), back.data(), limited_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL);
        for (size_t i = 0; i < limited_length; i++) {
          ASSERT_EQUAL(source[i], back[i]);
        }
        // Now let us decode the rest !!!
        size_t input_index = r.count;
        back.resize(simdutf::maximal_binary_length_from_base64(
            buffer.data() + input_index, buffer.size() - input_index));
        size_t second_length = back.size();
        r = simdutf::base64_to_binary_safe(buffer.data() + input_index,
                                           buffer.size() - input_index,
                                           back.data(), second_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        back.resize(second_length);
        ASSERT_EQUAL(second_length + limited_length, len);
        for (size_t i = 0; i < second_length; i++) {
          ASSERT_EQUAL(source[i + limited_length], back[i]);
        }
      }
    }
  }
}

TEST(aborted_safe_roundtrip_base64_with_spaces) {
  for (size_t offset = 1; offset <= 16; offset += 3) {
    for (size_t len = offset; len < 1024; len++) {
      std::vector<char> source(len, 0);
      std::vector<char> buffer;
      buffer.resize(implementation.base64_length_from_binary(len));
      std::mt19937 gen((std::mt19937::result_type)(seed));
      std::uniform_int_distribution<int> byte_generator{0, 255};
      for (size_t trial = 0; trial < 10; trial++) {
        for (size_t i = 0; i < len; i++) {
          source[i] = byte_generator(gen);
        }
        size_t size = implementation.binary_to_base64(
            source.data(), source.size(), buffer.data());
        buffer.resize(size);
        for (size_t i = 0; i < 5; i++) {
          add_space(buffer, gen);
        }
        std::vector<char> back(simdutf::maximal_binary_length_from_base64(
            buffer.data(), buffer.size()));
        size_t limited_length = len - offset; // intentionally too little
        back.resize(limited_length);
        back.shrink_to_fit();
        simdutf::result r = simdutf::base64_to_binary_safe(
            buffer.data(), buffer.size(), back.data(), limited_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL);
        for (size_t i = 0; i < limited_length; i++) {
          ASSERT_EQUAL(source[i], back[i]);
        }
        // Now let us decode the rest !!!
        size_t input_index = r.count;
        back.resize(simdutf::maximal_binary_length_from_base64(
            buffer.data() + input_index, buffer.size() - input_index));
        size_t second_length = back.size();
        r = simdutf::base64_to_binary_safe(buffer.data() + input_index,
                                           buffer.size() - input_index,
                                           back.data(), second_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        back.resize(second_length);
        ASSERT_EQUAL(second_length + limited_length, len);
        for (size_t i = 0; i < second_length; i++) {
          ASSERT_EQUAL(source[i + limited_length], back[i]);
        }
      }
    }
  }
}

TEST(aborted_safe_roundtrip_base64_16_with_spaces) {
  for (size_t offset = 1; offset <= 16; offset += 3) {
    for (size_t len = offset; len < 1024; len++) {
      std::vector<char> source(len, 0);
      std::vector<char> buffer;
      std::vector<char16_t> buffer16;

      buffer.resize(implementation.base64_length_from_binary(len));
      std::vector<char> back(len);
      std::mt19937 gen((std::mt19937::result_type)(seed));
      std::uniform_int_distribution<int> byte_generator{0, 255};
      for (size_t trial = 0; trial < 10; trial++) {
        for (size_t i = 0; i < len; i++) {
          source[i] = byte_generator(gen);
        }
        size_t size = implementation.binary_to_base64(
            source.data(), source.size(), buffer.data());
        buffer.resize(size);
        for (size_t i = 0; i < 5; i++) {
          add_space(buffer, gen);
        }
        buffer16.resize(buffer.size());
        for (size_t i = 0; i < buffer.size(); i++) {
          buffer16[i] = buffer[i];
        }
        ASSERT_EQUAL(size, implementation.base64_length_from_binary(len));
        size_t limited_length = len - offset; // intentionally too little
        back.resize(limited_length);
        back.shrink_to_fit();
        simdutf::result r = simdutf::base64_to_binary_safe(
            buffer.data(), buffer.size(), back.data(), limited_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL);
        for (size_t i = 0; i < limited_length; i++) {
          ASSERT_EQUAL(source[i], back[i]);
        }
        // Now let us decode the rest !!!
        size_t input_index = r.count;
        back.resize(simdutf::maximal_binary_length_from_base64(
            buffer.data() + input_index, buffer.size() - input_index));
        size_t second_length = back.size();
        r = simdutf::base64_to_binary_safe(buffer.data() + input_index,
                                           buffer.size() - input_index,
                                           back.data(), second_length);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        back.resize(second_length);
        ASSERT_EQUAL(second_length + limited_length, len);
        for (size_t i = 0; i < second_length; i++) {
          ASSERT_EQUAL(source[i + limited_length], back[i]);
        }
      }
    }
  }
}

TEST(streaming_base64_roundtrip) {
  size_t len = 2048;
  std::vector<char> source(len, 0);
  std::vector<char> buffer;
  buffer.resize(implementation.base64_length_from_binary(len));
  std::mt19937 gen((std::mt19937::result_type)(seed));
  std::uniform_int_distribution<int> byte_generator{0, 255};
  for (size_t i = 0; i < len; i++) {
    source[i] = byte_generator(gen);
  }
  size_t size = implementation.binary_to_base64(source.data(), source.size(),
                                                buffer.data());
  buffer.resize(size);
  for (size_t window = 16; window <= 2048; window += 7) {
    // build a buffer with enough space to receive the decoded base64
    std::vector<char> back(len);
    size_t outpos = 0;
    for (size_t pos = 0; pos < buffer.size(); pos += window) {
      size_t count = std::min(window, buffer.size() - pos);
      simdutf::result r = implementation.base64_to_binary(
          buffer.data() + pos, count, back.data() + outpos);
      ASSERT_TRUE(r.error != simdutf::error_code::INVALID_BASE64_CHARACTER);
      if (count + pos == buffer.size()) {
        // We must check that the last call to base64_to_binary did not
        // end with an BASE64_INPUT_REMAINDER error.
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      } else {
        size_t tail_bytes_to_reprocess = 0;
        if (r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
          tail_bytes_to_reprocess = 1;
        } else {
          tail_bytes_to_reprocess = (r.count % 3) == 0 ? 0 : (r.count % 3) + 1;
        }
        pos -= tail_bytes_to_reprocess;
        r.count -= (r.count % 3);
      }
      outpos += r.count;
    }
    back.resize(outpos);
    ASSERT_TRUE(back == source);
  }
}

TEST(readme_test) {
  size_t len = 2048;
  std::vector<char> base64(len, 'a');
  std::vector<char> back((len + 3) / 4 * 3);
  size_t outpos = 0;
  size_t window = 512;
  for (size_t pos = 0; pos < base64.size(); pos += window) {
    // how many base64 characters we can process in this iteration
    size_t count = std::min(window, base64.size() - pos);
    simdutf::result r = simdutf::base64_to_binary(base64.data() + pos, count,
                                                  back.data() + outpos);
    if (r.error == simdutf::error_code::INVALID_BASE64_CHARACTER) {
      printf("Invalid base64 character at position %zu\n", pos + r.count);
      return;
    }
    // If we arrived at the end of the base64 input, we must check that the
    // number of characters processed is a multiple of 4, or that we have a
    // remainder of 0, 2 or 3.
    if (count + pos == base64.size() &&
        r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
      puts("The base64 input contained an invalid number of characters");
    }
    // If we are not at then end, we may have to reprocess either 1, 2 or 3
    // bytes, and to drop the last 0, 2 or 3 bytes decoded.
    size_t tail_bytes_to_reprocess = 0;
    if (r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
      tail_bytes_to_reprocess = 1;
    } else {
      tail_bytes_to_reprocess = (r.count % 3) == 0 ? 0 : (r.count % 3) + 1;
    }
    pos -= tail_bytes_to_reprocess;
    r.count -= (r.count % 3);
    outpos += r.count;
  }
  // At then end, we resize the buffer to the actual number of bytes decoded.
  back.resize(outpos);
}

TEST(readme_safe) {
  size_t len = 72;
  std::vector<char> base64(len, 'a');
  std::vector<char> back((len + 3) / 4 * 3);
  size_t limited_length = back.size() / 2; // Intentionally too small
  simdutf::result r = simdutf::base64_to_binary_safe(
      base64.data(), base64.size(), back.data(), limited_length);
  ASSERT_EQUAL(r.error, simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL);

  // We decoded 'limited_length' bytes to back.
  // Now let us decode the rest !!!
  size_t input_index = r.count;
  size_t limited_length2 = back.size();
  r = simdutf::base64_to_binary_safe(base64.data() + input_index,
                                     base64.size() - input_index, back.data(),
                                     limited_length2);
  ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
  back.resize(limited_length2);
  ASSERT_EQUAL(limited_length2 + limited_length, (len + 3) / 4 * 3);
}

#if SIMDUTF_SPAN
TEST(base64_to_binary_safe_span_api_char) {
  const std::string input{"QWJyYWNhZGFicmEh"};
  const std::string expected_output{"Abracadabra!"};
  std::string output(expected_output.size() + 4, '\0');
  const auto [ret, outlen] = simdutf::base64_to_binary_safe(input, output);
  ASSERT_EQUAL(ret.error, simdutf::SUCCESS);
  ASSERT_EQUAL(ret.count, 16); // amount of consumed input
  ASSERT_EQUAL(outlen, 12);    // how much was written to output
}

TEST(base64_to_binary_safe_span_api_char16) {
  const std::u16string input{u"QWJyYWNhZGFicmEh"};
  const std::string expected_output{"Abracadabra!"};
  std::string output(expected_output.size() + 4, '\0');
  const auto [ret, outlen] = simdutf::base64_to_binary_safe(input, output);
  ASSERT_EQUAL(ret.error, simdutf::SUCCESS);
  ASSERT_EQUAL(ret.count, 16); // amount of consumed input
  ASSERT_EQUAL(outlen, 12);    // how much was written to output
}
#endif

#if !defined(SIMDUTF_NO_THREADS) && SIMDUTF_ATOMIC_REF
TEST(atomic_roundtrip_base64) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = simdutf::atomic_binary_to_base64(
          source.data(), source.size(), buffer.data());
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(len));
      simdutf::result r =
          implementation.base64_to_binary(buffer.data(), size, back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);
      back.assign(len, 0);
      size_t written_out = back.size();
      r = simdutf::atomic_base64_to_binary_safe(buffer.data(), size,
                                                back.data(), written_out);
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, size);
      ASSERT_EQUAL(written_out, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);

      // Test with all last_chunk_handling_options
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        r = implementation.base64_to_binary(buffer.data(), size, back.data(),
                                            simdutf::base64_default, option);
        ASSERT_TRUE((size % 4) == 0);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(r.count, len);
        ASSERT_TRUE(back == source);
      }
    }
  }
}

TEST(atomic_binary_to_base64_large_roundtrip) {
  const std::vector<char> binary(10000, '\0');
  std::string b64output(binary.size() * 4 / 3 + 4, '\0');
  const auto ret = simdutf::atomic_binary_to_base64(binary, b64output);
  ASSERT_TRUE(ret > 0);
  b64output.resize(ret);
  std::vector<char> recovered(binary.size(), '?');
  std::size_t outlen = recovered.size();
  const auto ret2 = simdutf::atomic_base64_to_binary_safe(
      b64output.data(), b64output.size(), recovered.data(), outlen);
  ASSERT_EQUAL(ret2.error, simdutf::SUCCESS);
  ASSERT_EQUAL(recovered, binary);
}

TEST(atomic_span_roundtrip_base64) {
  for (size_t len = 0; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::vector<char> back(len);
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = simdutf::atomic_binary_to_base64(source, buffer);
      ASSERT_EQUAL(size, implementation.base64_length_from_binary(len));
      simdutf::result r =
          implementation.base64_to_binary(buffer.data(), size, back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            printf("Mismatch at position %zu trial %zu\n", i, trial);
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for (size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);

      // Test with all last_chunk_handling_options
      for (auto option :
           {simdutf::last_chunk_handling_options::strict,
            simdutf::last_chunk_handling_options::loose,
            simdutf::last_chunk_handling_options::stop_before_partial}) {
        r = implementation.base64_to_binary(buffer.data(), size, back.data(),
                                            simdutf::base64_default, option);
        ASSERT_TRUE((size % 4) == 0);
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(r.count, len);
        ASSERT_TRUE(back == source);
      }
    }
  }
}
#endif // !defined(SIMDUTF_NO_THREADS) && SIMDUTF_ATOMIC_REF

int main(int argc, char *argv[]) {
  const auto cmdline = simdutf::test::CommandLine::parse(argc, argv);
  seed = cmdline.seed;

  simdutf::test::run(cmdline);

  return 0;
}
