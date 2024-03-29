#include "simdutf.h"

#include <array>
#include <iostream>

#include <memory>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

using random_generator = std::mt19937;
static random_generator::result_type seed = 42;

const uint8_t to_base64_value[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 64,  64,  255, 255, 64,  255,
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


const uint8_t to_base64url_value[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 64,  64,  255, 255, 64,  255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 64,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  255,
    62, 255, 255,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255,
    255, 255, 255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25,  255, 255, 255, 255, 63, 255, 26,  27,  28,  29,  30,  31,  32,  33,
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
template <typename char_type>
size_t add_space(std::vector<char_type> &v, std::mt19937 &gen) {
  const static std::array<char_type, 4> space = {' ', '\t', '\n', '\r'};
  int padding = 0;
  if (v.size() > 0 && v[v.size() - 1] == '=') {
    padding++;
    if (v.size() > 0 && v[v.size() - 1] == '=') {
      padding++;
    }
  }
  std::uniform_int_distribution<int> index_dist(0, v.size() - padding);
  size_t i = index_dist(gen);
  std::uniform_int_distribution<int> char_dist(0, 3);
  v.insert(v.begin() + i, space[char_dist(gen)]);
  return i;
}

template <typename char_type>
size_t add_garbage(std::vector<char_type> &v, std::mt19937 &gen) {
  int padding = 0;
  if (v.size() > 0 && v[v.size() - 1] == '=') {
    padding++;
    if (v.size() > 0 && v[v.size() - 1] == '=') {
      padding++;
    }
  }
  std::uniform_int_distribution<int> index_dist(0, v.size() - padding);
  size_t i = index_dist(gen);
  std::uniform_int_distribution<int> char_dist(
      0, (1 << (sizeof(char_type) * 8)) - 1);
  uint8_t c = char_dist(gen);
  while (uint8_t(c) == c && to_base64_value[uint8_t(c)] != 255) {
    c = char_dist(gen);
  }
  v.insert(v.begin() + i, c);
  return i;
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

TEST(encode_base64_cases) {
  std::vector<std::pair<std::string, std::string>> cases = {
      {"Hello, World!", "SGVsbG8sIFdvcmxkIQ=="},
      {"GeeksforGeeks", "R2Vla3Nmb3JHZWVrcw=="},
      {"123456", "MTIzNDU2"},
      {"Base64 Encoding", "QmFzZTY0IEVuY29kaW5n"},
      {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k", "IVJ+SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c+fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNTktWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};
  std::vector<simdutf::error_code> codes = {simdutf::error_code::SUCCESS};
  std::vector<size_t> counts = {1};
  printf(" -- ");
  for (std::pair<std::string, std::string> p : cases) {
    std::vector<char> buffer(
        implementation.base64_length_from_binary(p.first.size()));
    ASSERT_EQUAL(buffer.size(), p.second.size());
    size_t s = implementation.binary_to_base64(p.first.data(), p.first.size(),
                                               buffer.data());
    ASSERT_EQUAL(s, p.second.size());
    ASSERT_TRUE(std::string(buffer.data(), buffer.size()) == p.second);
  }
  printf(" -- ");
  for (std::pair<std::string, std::string> p : cases) {
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
  printf(" --  ");
  for (std::pair<std::string, std::string> p : cases) {
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


TEST(encode_base64url_cases) {
  std::vector<std::pair<std::string, std::string>> cases = {
      {"Hello, World!", "SGVsbG8sIFdvcmxkIQ=="},
      {"GeeksforGeeks", "R2Vla3Nmb3JHZWVrcw=="},
      {"123456", "MTIzNDU2"},
      {"Base64 Encoding", "QmFzZTY0IEVuY29kaW5n"},
      {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k", "IVJ-SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c-fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNTktWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};
  std::vector<simdutf::error_code> codes = {simdutf::error_code::SUCCESS};
  std::vector<size_t> counts = {1};
  printf(" -- ");
  for (std::pair<std::string, std::string> p : cases) {
    std::vector<char> buffer(
        implementation.base64_length_from_binary(p.first.size()));
    ASSERT_EQUAL(buffer.size(), p.second.size());
    size_t s = implementation.binary_to_base64(p.first.data(), p.first.size(),
                                               buffer.data(), simdutf::base64_url);
    ASSERT_EQUAL(s, p.second.size());
    ASSERT_TRUE(std::string(buffer.data(), buffer.size()) == p.second);
  }
  printf(" -- ");
  for (std::pair<std::string, std::string> p : cases) {
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
  printf(" --  ");
  for (std::pair<std::string, std::string> p : cases) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    size_t length = buffer.size();
    simdutf::result r = simdutf::base64_to_binary_safe(
        p.second.data(), p.second.size(), buffer.data(), length, simdutf::base64_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.second.size());
    ASSERT_EQUAL(length, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

TEST(encode_base64_cases_16) {
  std::vector<std::pair<std::string, std::u16string>> cases = {
      {"Hello, World!", u"SGVsbG8sIFdvcmxkIQ=="},
      {"GeeksforGeeks", u"R2Vla3Nmb3JHZWVrcw=="},
      {"123456", u"MTIzNDU2"},
      {"Base64 Encoding", u"QmFzZTY0IEVuY29kaW5n"},
      {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k", u"IVJ+SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c+fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNTktWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};
  std::vector<simdutf::error_code> codes = {simdutf::error_code::SUCCESS};
  std::vector<size_t> counts = {1};
  printf(" -- ");

  for (std::pair<std::string, std::u16string> p : cases) {
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
  printf(" -- ");
  for (std::pair<std::string, std::u16string> p : cases) {
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


TEST(encode_base64url_cases_16) {
  std::vector<std::pair<std::string, std::u16string>> cases = {
      {"Hello, World!", u"SGVsbG8sIFdvcmxkIQ=="},
      {"GeeksforGeeks", u"R2Vla3Nmb3JHZWVrcw=="},
      {"123456", u"MTIzNDU2"},
      {"Base64 Encoding", u"QmFzZTY0IEVuY29kaW5n"},
      {"!R~J2jL&mI]O)3=c:G3Mo)oqmJdxoprTZDyxEvU0MI.'Ww5H{G>}y;;+B8E_Ah,Ed[ PdBqY'^N>O$4:7LK1<:|7)btV@|{YWR$$Er59-XjVrFl4L}~yzTEd4'E[@k", u"IVJ-SjJqTCZtSV1PKTM9YzpHM01vKW9xbUpkeG9wclRaRHl4RXZVME1JLidXdzVIe0c-fXk7OytCOEVfQWgsRWRbIFBkQnFZJ15OPk8kNDo3TEsxPDp8NylidFZAfHtZV1IkJEVyNTktWGpWckZsNEx9fnl6VEVkNCdFW0Br"}};
  std::vector<simdutf::error_code> codes = {simdutf::error_code::SUCCESS};
  std::vector<size_t> counts = {1};
  printf(" -- ");

  for (std::pair<std::string, std::u16string> p : cases) {
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
  printf(" -- ");
  for (std::pair<std::string, std::u16string> p : cases) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(
        p.second.data(), p.second.size()));
    ASSERT_EQUAL(buffer.size(), p.first.size());
    size_t length = buffer.size();
    simdutf::result r = simdutf::base64_to_binary_safe(
        p.second.data(), p.second.size(), buffer.data(), length, simdutf::base64_url);
    ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
    ASSERT_EQUAL(r.count, p.second.size());
    ASSERT_EQUAL(length, p.first.size());
    for (size_t i = 0; i < buffer.size(); i++) {
      ASSERT_EQUAL(buffer[i], p.first[i]);
    }
  }
}

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
      ASSERT_TRUE(size == implementation.base64_length_from_binary(len));
      simdutf::result r =
          implementation.base64_to_binary(buffer.data(), size, back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            std::cerr << "Mismatch at position " << i << " trial " << trial
                      << std::endl;
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
      ASSERT_TRUE(size == implementation.base64_length_from_binary(len));
      simdutf::result r =
          implementation.base64_to_binary(buffer16.data(), size, back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            std::cerr << "Mismatch at position " << i << " trial " << trial
                      << std::endl;
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



TEST(roundtrip_base64url) {
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
          source.data(), source.size(), buffer.data(), simdutf::base64_url);
      ASSERT_TRUE(size == implementation.base64_length_from_binary(len));
      simdutf::result r =
          implementation.base64_to_binary(buffer.data(), size, back.data(), simdutf::base64_url);
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            std::cerr << "Mismatch at position " << i << " trial " << trial
                      << std::endl;
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

TEST(roundtrip_base64url_16) {
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
      buffer16.resize(buffer.size());
      for (size_t i = 0; i < buffer.size(); i++) {
        buffer16[i] = buffer[i];
      }
      ASSERT_TRUE(size == implementation.base64_length_from_binary(len));
      simdutf::result r =
          implementation.base64_to_binary(buffer16.data(), size, back.data(), simdutf::base64_url);
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            std::cerr << "Mismatch at position " << i << " trial " << trial
                      << std::endl;
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
      size_t location = add_garbage(buffer, gen);
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      simdutf::result r =
          simdutf::base64_to_binary(buffer.data(), buffer.size(), back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
      ASSERT_EQUAL(r.count, location);
      size_t back_length = back.size();
      r = simdutf::base64_to_binary_safe(buffer.data(), buffer.size(),
                                         back.data(), back_length);
      ASSERT_EQUAL(r.error, simdutf::error_code::INVALID_BASE64_CHARACTER);
      ASSERT_EQUAL(r.count, location);
    }
  }
}

TEST(doomed_truncated_base64_roundtrip) {
  for (size_t len = 1; len < 2048; len++) {
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
      buffer.resize(size - 3);
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      simdutf::result r =
          simdutf::base64_to_binary(buffer.data(), buffer.size(), back.data());
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

TEST(doomed_truncated_base64_roundtrip_16) {
  for (size_t len = 1; len < 2048; len++) {
    std::vector<char> source(len, 0);
    std::vector<char> buffer;
    std::vector<char16_t> buffer16;
    buffer.resize(implementation.base64_length_from_binary(len));
    std::mt19937 gen((std::mt19937::result_type)(seed));
    std::uniform_int_distribution<int> byte_generator{0, 255};
    for (size_t trial = 0; trial < 10; trial++) {
      for (size_t i = 0; i < len; i++) {
        source[i] = byte_generator(gen);
      }
      size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
      buffer.resize(size - 3);
      buffer16.resize(buffer.size());
      for (size_t i = 0; i < buffer.size(); i++) {
        buffer16[i] = buffer[i];
      }
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer16.data(), buffer16.size()));
      simdutf::result r = simdutf::base64_to_binary(
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

TEST(roundtrip_base64_with_spaces) {
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
        add_space(buffer, gen);
      }
      std::vector<char> back(simdutf::maximal_binary_length_from_base64(
          buffer.data(), buffer.size()));
      simdutf::result r =
          simdutf::base64_to_binary(buffer.data(), buffer.size(), back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);

      back.resize(
          r.count); // resize the buffer according to actual number of bytes
      ASSERT_EQUAL(r.count, len);
      ASSERT_TRUE(back == source);
      back.resize(back.capacity());
      size_t back_length = back.size();
      r = simdutf::base64_to_binary_safe(buffer.data(), buffer.size(),
                                         back.data(), back_length);

      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);

      back.resize(
          back_length); // resize the buffer according to actual number of bytes
      ASSERT_EQUAL(r.count, buffer.size());
      ASSERT_TRUE(back == source);
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
      ASSERT_TRUE(size == implementation.base64_length_from_binary(len));
      simdutf::result r = implementation.base64_to_binary(
          buffer16.data(), buffer16.size(), back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      ASSERT_EQUAL(r.count, len);
      if (back != source) {
        printf("=====input size %zu\n", len);
        for (size_t i = 0; i < len; i++) {
          if (back[i] != source[i]) {
            std::cerr << "Mismatch at position " << i << " trial " << trial
                      << std::endl;
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
  for (size_t offset = 1; offset <= 16; offset+=3) {
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
  for (size_t offset = 1; offset <= 16; offset+=3) {
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
        ASSERT_TRUE(size == implementation.base64_length_from_binary(len));
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
  for (size_t offset = 1; offset <= 16; offset+=3) {
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
  for (size_t offset = 1; offset <= 16; offset+=3) {
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
        ASSERT_TRUE(size == implementation.base64_length_from_binary(len));
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
      simdutf::result r = simdutf::base64_to_binary(buffer.data() + pos, count,
                                                    back.data() + outpos);
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
      std::cerr << "Invalid base64 character at position " << pos + r.count
                << std::endl;
      return;
    }
    // If we arrived at the end of the base64 input, we must check that the
    // number of characters processed is a multiple of 4, or that we have a
    // remainder of 0, 2 or 3.
    if (count + pos == base64.size() &&
        r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
      std::cerr << "The base64 input contained an invalid number of characters "
                << std::endl;
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
                                           base64.size() - input_index,
                                           back.data(), limited_length2);
  ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
  back.resize(limited_length2);
  ASSERT_EQUAL(limited_length2 + limited_length, (len + 3) / 4 * 3);
}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    try {
      seed = std::stoi(argv[1]);
    } catch (const std::exception &e) {
      printf("%s\n", e.what());
      return EXIT_FAILURE;
    }
  }
  return simdutf::test::main(argc, argv);
}
