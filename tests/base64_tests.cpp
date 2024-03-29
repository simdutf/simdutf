#include "simdutf.h"

#include <array>
#include <iostream>

#include <memory>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

using random_generator = std::mt19937;
static random_generator::result_type seed = 42;

TEST(decode_base64_cases) {
  std::vector<std::vector<char>> cases = {{0x53, 0x53}};
  std::vector<simdutf::error_code> codes = {simdutf::error_code::SUCCESS};
  std::vector<size_t> counts = {1};

  for(size_t i = 0; i < cases.size(); i++) {
    std::vector<char> buffer(implementation.maximal_binary_length_from_base64(cases[i].data(), cases[i].size()));
    simdutf::result r = implementation.base64_to_binary(cases[i].data(), cases[i].size(), buffer.data());
    ASSERT_EQUAL(r.error,codes[i]);
    ASSERT_EQUAL(r.count, counts[i]);
  }
}

TEST(encode_base64_cases) {
  std::vector<std::pair<std::string,std::string>> cases = {
    {"Hello, World!", "SGVsbG8sIFdvcmxkIQ=="},
    {"GeeksforGeeks", "R2Vla3Nmb3JHZWVrcw=="},
    {"123456", "MTIzNDU2"},
    {"Base64 Encoding", "QmFzZTY0IEVuY29kaW5n"}};
  std::vector<simdutf::error_code> codes = {simdutf::error_code::SUCCESS};
  std::vector<size_t> counts = {1};

  for(std::pair<std::string,std::string> p : cases) {
    std::vector<char> buffer(implementation.base64_length_from_binary(p.first.size()));
    ASSERT_EQUAL(buffer.size(), p.second.size());
    size_t s = implementation.binary_to_base64(p.first.data(),p.first.size(), buffer.data());
    ASSERT_EQUAL(s, p.second.size());
    ASSERT_TRUE(std::string(buffer.data(), buffer.size()) == p.second);
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
      if(back != source) {
        printf("=====input size %zu\n", len);
        for(size_t i = 0; i < len; i++) {
          if(back[i] != source[i]) {
            std::cerr << "Mismatch at position " << i << " trial " << trial << std::endl;
          }
          printf("%zu: %02x %02x\n", i, uint8_t(back[i]), uint8_t(source[i]));
        }
        printf("=====base64 size %zu\n", size);
        for(size_t i = 0; i < size; i++) {
          printf("%zu: %02x %c\n", i, uint8_t(buffer[i]), buffer[i]);
        }
      }
      ASSERT_TRUE(back == source);
    }
  }
}

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

size_t add_space(std::vector<char> &v, std::mt19937 &gen) {
  const static std::array<char, 4> space = {' ', '\t', '\n', '\r'};
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

size_t add_garbage(std::vector<char> &v, std::mt19937 &gen) {
  int padding = 0;
  if (v.size() > 0 && v[v.size() - 1] == '=') {
    padding++;
    if (v.size() > 0 && v[v.size() - 1] == '=') {
      padding++;
    }
  }
  std::uniform_int_distribution<int> index_dist(0, v.size() - padding);
  size_t i = index_dist(gen);
  std::uniform_int_distribution<int> char_dist(0, 255);
  uint8_t c = char_dist(gen);
  while(to_base64_value[uint8_t(c)] != 255) {
    c = char_dist(gen);
  }
  v.insert(v.begin() + i, c);
  return i;
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
      simdutf::result r = simdutf::base64_to_binary(
          buffer.data(), buffer.size(), back.data());
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
      simdutf::result r = simdutf::base64_to_binary(
          buffer.data(), buffer.size(), back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::BASE64_INPUT_REMAINDER);
      ASSERT_EQUAL(r.count, (size-4)/4*3);
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
      simdutf::result r = simdutf::base64_to_binary(
          buffer.data(), buffer.size(), back.data());
      ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);

      back.resize(
          r.count); // resize the buffer according to actual number of bytes
      ASSERT_EQUAL(r.count, len);
      ASSERT_TRUE(back == source);
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
  size_t size = implementation.binary_to_base64(
          source.data(), source.size(), buffer.data());
  buffer.resize(size);
  for (size_t window = 16; window <= 2048; window += 7) {
    // build a buffer with enough space to receive the decoded base64
    std::vector<char> back(len);
    size_t outpos = 0;
    for(size_t pos = 0; pos < buffer.size(); pos += window) {
      size_t count = std::min(window, buffer.size() - pos);
      simdutf::result r = simdutf::base64_to_binary(
          buffer.data() + pos, count, back.data() + outpos);
      ASSERT_TRUE(r.error != simdutf::error_code::INVALID_BASE64_CHARACTER);
      if(count + pos == buffer.size()) {
        // We must check that the last call to base64_to_binary did not
        // end with an BASE64_INPUT_REMAINDER error.
        ASSERT_EQUAL(r.error, simdutf::error_code::SUCCESS);
      } else {
        size_t tail_bytes_to_reprocess = 0;
        if(r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
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
  std::vector<char> back((len+3)/4*3);
  size_t outpos = 0;
  size_t window = 512;
  for(size_t pos = 0; pos < base64.size(); pos += window) {
    // how many base64 characters we can process in this iteration
    size_t count = std::min(window, base64.size() - pos);
    simdutf::result r = simdutf::base64_to_binary(
        base64.data() + pos, count, back.data() + outpos);
    if(r.error == simdutf::error_code::INVALID_BASE64_CHARACTER) {
      std::cerr << "Invalid base64 character at position " << pos + r.count << std::endl;
      return;
    }
    // If we arrived at the end of the base64 input, we must check that the number
    // of characters processed is a multiple of 4, or that we have a remainder of 0, 2 or 3.
    if(count + pos == base64.size() && r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
      std::cerr << "The base64 input contained an invalid number of characters " << std::endl;
    }
    // If we are not at then end, we may have to reprocess either 1, 2 or 3 bytes, and
    // to drop the last 0, 2 or 3 bytes decoded.
    size_t tail_bytes_to_reprocess = 0;
    if(r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
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

int main(int argc, char *argv[]) {
  if (argc == 2) {
    try {
      seed = std::stoi(argv[1]);
    } catch (const std::exception& e) {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }
  }
  return simdutf::test::main(argc, argv);
}
