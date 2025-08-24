#include "random_utf16.h"
#include "simdutf.h"
#include "../reference/encode_utf16.h"

#include <stdexcept>
#include <vector>

namespace simdutf {
namespace tests {
namespace helpers {

std::vector<char16_t> random_utf16::generate_le(size_t size) {
  auto result = generate_counted(size).first;
  if (!match_system(endianness::LITTLE)) {
    change_endianness_utf16(result.data(), result.size(), result.data());
  }

  return result;
}

void random_utf16::to_ascii_le(std::vector<char16_t> &output) {
  char16_t mask = 0x7F;
  if (!match_system(endianness::LITTLE)) {
    mask = 0x7F00;
  }
  for (auto &ch : output) {
    ch &= mask;
  }
}

void random_utf16::to_ascii_be(std::vector<char16_t> &output) {
  char16_t mask = 0x7F;
  if (!match_system(endianness::BIG)) {
    mask = 0x7F00;
  }
  for (auto &ch : output) {
    ch &= mask;
  }
}

std::vector<char16_t> random_utf16::generate_be(size_t size) {
  auto result = generate_counted(size).first;
  if (!match_system(endianness::BIG)) {
    change_endianness_utf16(result.data(), result.size(), result.data());
  }

  return result;
}

std::vector<char16_t> random_utf16::generate_le(size_t size, long seed) {
  gen.seed(seed);
  return generate_le(size);
}

std::vector<char16_t> random_utf16::generate_be(size_t size, long seed) {
  gen.seed(seed);
  return generate_be(size);
}

std::pair<std::vector<char16_t>, size_t>
random_utf16::generate_counted_le(size_t size) {
  auto res = generate_counted(size);
  if (!match_system(endianness::LITTLE)) {
    change_endianness_utf16(res.first.data(), res.first.size(),
                            res.first.data());
  }

  return res;
}

std::pair<std::vector<char16_t>, size_t>
random_utf16::generate_counted_be(size_t size) {
  auto res = generate_counted(size);
  if (!match_system(endianness::BIG)) {
    change_endianness_utf16(res.first.data(), res.first.size(),
                            res.first.data());
  }

  return res;
}

std::pair<std::vector<char16_t>, size_t>
random_utf16::generate_counted(size_t size) {
  std::vector<char16_t> result;
  result.reserve(size);

  char16_t W1;
  char16_t W2;
  size_t count{0};
  while (result.size() < size) {
    count++;
    const uint32_t value = generate();
    switch (simdutf::tests::reference::utf16::encode(value, W1, W2)) {
    case 0:
      throw std::runtime_error("Random UTF-16 generator is broken");
    case 1:
      result.push_back(W1);
      break;
    case 2:
      result.push_back(W1);
      result.push_back(W2);
      break;
    }
  }
  return make_pair(result, count);
}

uint32_t random_utf16::generate() {
  switch (utf16_length(gen)) {
  case 0:
    return single_word0(gen);
  case 1:
    return single_word1(gen);
  case 2:
    return two_words(gen);
  default:
    abort();
  }
}

} // namespace helpers
} // namespace tests
} // namespace simdutf
