#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

namespace simdutf {
namespace tests {
namespace helpers {

  /*
    Generates valid random UTF-16

    It might generate streams consisting:
    - only single 16-bit words (random_utf16(..., 1, 0));
    - only surrogate pairs, two 16-bit words (random_utf16(..., 0, 1))
    - mixed, depending on given probabilities (random_utf16(..., 1, 1))
  */
  class random_utf16 {
    std::mt19937 gen;

  public:
    random_utf16(std::random_device& rd, int single_word_prob, int two_words_probability)
      : gen{rd()}
      , utf16_length({double(single_word_prob),
                      double(single_word_prob),
                      double(2 * two_words_probability)}) {}

    std::vector<char16_t> generate(size_t size);
    std::vector<char16_t> generate(size_t size, long seed);
    std::pair<std::vector<char16_t>,size_t> generate_counted(size_t size);
  private:
    std::discrete_distribution<> utf16_length;
    std::uniform_int_distribution<uint32_t> single_word0{0x0000'0000, 0x0000'd7ff};
    std::uniform_int_distribution<uint32_t> single_word1{0x0000'e000, 0x0000'ffff};
    std::uniform_int_distribution<uint32_t> two_words   {0x0001'0000, 0x0010'ffff};
    uint32_t generate();
  };

} // namespace helpers
} // namespace tests
} // namespace simdutf
