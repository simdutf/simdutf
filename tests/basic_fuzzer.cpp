#include "simdutf.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <random>

#include <tests/reference/encode_utf8.h>
#include <tests/helpers/test.h>

/**
 * We should be able to receive random data without any problem
 * when using the validating transcoder. It is difficult to test
 * extensively, but it is easy to try many thousands of random test
 * cases.
 */

namespace {
std::array<size_t, 10> input_size{7, 16, 12, 64, 67, 128, 129, 256, 1024, 1025};
}

//  Possible states.
//  Format: xxx_yyy where xxx is the number of bytes (in UTF-8) and yyy is the error encoded (if any).
enum state {
  ONE_VALID = 0,
  ONE_TOO_LONG,
  ONE_TOO_LARGE,
  TWO_VALID,  // 3
  TWO_HEADER,
  TWO_TOO_SHORT,
  TWO_TOO_LONG,
  TWO_OVERLONG,
  THREE_VALID,  // 8
  THREE_HEADER,
  THREE_TOO_SHORT,
  THREE_TOO_LONG,
  THREE_OVERLONG,
  THREE_SURROGATE,
  FOUR_VALID, // 14
  FOUR_HEADER,
  FOUR_TOO_SHORT,
  FOUR_TOO_LONG,
  FOUR_OVERLONG,
  FOUR_TOO_LARGE
};

struct state_tracker {
  private:
  enum state current_state;
  std::mt19937 gen;

  public:
  state_tracker(uint32_t lo, uint32_t hi, uint64_t seed) noexcept
      : gen(std::mt19937::result_type(seed)) {
    current_state = next_state();
  }

  void next(std::vector<char>& output) {
    // Write current state to output
    auto consume = [&output](uint8_t byte) {
      output.push_back(byte);
    };
    switch(current_state)
    {
      case ONE_VALID: {
        simdutf::tests::reference::utf8::encode(generate(0x0, 0x7f), consume);
        break;
      }
      case ONE_TOO_LONG: {
        simdutf::tests::reference::utf8::encode(generate(0x0, 0x7f), consume);
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        break;
      }
      case ONE_TOO_LARGE: {
        output.push_back(generate(0x80, 0xff));
        break;
      }
      case TWO_VALID: {
        simdutf::tests::reference::utf8::encode(generate(0x80, 0x7ff), consume);
        break;
      }
      case TWO_HEADER: {
        uint32_t codepoint = generate(0x80, 0x7ff);
        output.push_back(0xf8 | (codepoint >> 6));  // Corrupt leading byte
        output.push_back(0x80 | (codepoint & 0x3f));
        break;
      }
      case TWO_TOO_SHORT: {
        output.push_back(generate(0xc1, 0xdf)); // Only produce normal leading byte
        break;
      }
      case TWO_TOO_LONG: {
        simdutf::tests::reference::utf8::encode(generate(0x80, 0x7ff), consume);
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        break;
      }
      case TWO_OVERLONG: {
        output.push_back(0xc0); // Add "empty" leading byte
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        break;
      }
      case THREE_VALID: {
        uint32_t codepoint = generate(0x800, 0xffff);
        // Hacky, but there is only a ~3.2% to generate a codepoint in the forbidden range each time
        while (codepoint >= 0xd800 && codepoint <= 0xdfff) {
          codepoint = generate(0x800, 0xffff);
        }
        simdutf::tests::reference::utf8::encode(generate(0x80, 0x7ff), consume);
        break;
      }
      case THREE_HEADER: {
        uint32_t codepoint = generate(0x800, 0xffff);
        // Hacky, but there is only a ~3.2% to generate a codepoint in the forbidden range each time
        while (codepoint >= 0xd800 && codepoint <= 0xdfff) {
          codepoint = generate(0x800, 0xffff);
        }
        output.push_back(0xf8 | (codepoint >> 12)); // Corrupt leading byte
        output.push_back(0x80 | ((codepoint >> 6) & 0x3f));
        output.push_back(0x80 | (codepoint & 0x3f));
        break;
      }
      case THREE_TOO_SHORT: {
        uint32_t codepoint = generate(0x800, 0xffff);
        // Hacky, but there is only a ~3.2% to generate a codepoint in the forbidden range each time
        while (codepoint >= 0xd800 && codepoint <= 0xdfff) {
          codepoint = generate(0x800, 0xffff);
        }
        output.push_back(0xe0 | (codepoint >> 12)); // Corrupt leading byte
        output.push_back(0x80 | ((codepoint >> 6) & 0x3f));
        break;
      }
      case THREE_TOO_LONG: {
        uint32_t codepoint = generate(0x800, 0xffff);
        // Hacky, but there is only a ~3.2% to generate a codepoint in the forbidden range each time
        while (codepoint >= 0xd800 && codepoint <= 0xdfff) {
          codepoint = generate(0x800, 0xffff);
        }
        simdutf::tests::reference::utf8::encode(generate(0x80, 0x7ff), consume);
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        break;
      }
      case THREE_OVERLONG: {
        output.push_back(0xe0); // Add "empty" leading byte
        output.push_back(generate(0x80, 0x9f)); // First continuation byte must start by 0x8_ or 0x9_
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        break;
      }
      case THREE_SURROGATE: {
        simdutf::tests::reference::utf8::encode(generate(0xd800, 0xdfff), consume);
        break;
      }
      case FOUR_VALID: {
        simdutf::tests::reference::utf8::encode(generate(0x10000, 0x10ffff), consume);
        break;
      }
      case FOUR_HEADER: {
        uint32_t codepoint = generate(0x10000, 0x10ffff);
        output.push_back(0xf8 | (codepoint >> 18));   // Corrupt leading byte
        output.push_back(0x80 | ((codepoint >> 12) & 0x3f));
        output.push_back(0x80 | ((codepoint >> 6) & 0x3f));
        output.push_back(0x80 | (codepoint & 0x3f));
        break;
      }
      case FOUR_TOO_SHORT: {
        uint32_t codepoint = generate(0x10000, 0x10ffff);
        output.push_back(0xf0 | (codepoint >> 18));
        output.push_back(0x80 | ((codepoint >> 12) & 0x3f));
        output.push_back(0x80 | ((codepoint >> 6) & 0x3f));
        break;
      }
      case FOUR_TOO_LONG: {
        simdutf::tests::reference::utf8::encode(generate(0x10000, 0x10ffff), consume);
        output.push_back(generate(0x80, 0xbf)); // Add random continuation byte
        break;
      }
      case FOUR_OVERLONG: {
        output.push_back(0xf0); // Add "empty" leading byte
        output.push_back(generate(0x80, 0x8f)); // First continuation byte must have start by 0x8_
        output.push_back(generate(0x80, 0xbf)); // Add two random continuation bytes
        output.push_back(generate(0x80, 0xbf));
        break;
      }
      case FOUR_TOO_LARGE: {
        simdutf::tests::reference::utf8::encode(generate(0x110000, 0x1fffff), consume);
        break;
      }
    }
    // Move to next state
    next_state();
  }

  private:
  uint32_t generate(uint32_t lo, uint32_t hi) {
    return std::uniform_int_distribution<uint32_t>{lo, hi}(gen);
  }

  enum state next_state() {
    // Valid indexes are 0, 3, 8 and 14, (20 states in total)
    return static_cast<state>(std::discrete_distribution<>{20,1,1,20,1,1,1,1,20,1,1,1,1,1,20,1,1,1,1,1}(gen));
  }
};

struct buffer {
  std::vector<char> input;
  std::vector<char> output;
  buffer(size_t size) : input(size), output(4 * size) {}
  void print_input() const {
    for (size_t i = 0; i < input.size(); i++) {
      printf("%x ", uint8_t(input[i]));
    }
    printf("\n");
  }
  void randomize(std::mt19937 &gen) {
    for (size_t i = 0; i < input.size(); i++) {
      input[i] = char(gen());
    }
  }
};

TEST(basic_fuzz) {
  std::vector<buffer> buffers;
  for (size_t size : input_size) {
    buffers.emplace_back(size);
  }
  std::mt19937 gen(124); // We want deterministic results.
  size_t counter{0};
  while (counter < 100000) {
    for (buffer &buf : buffers) {
      buf.randomize(gen);
      counter++;
      if ((counter % 10000) == 0) {
        printf("-");
        fflush(NULL);
      }
      bool is_ok_utf8 =
          implementation.validate_utf8(buf.input.data(), buf.input.size());
      bool is_ok_utf16 = implementation.validate_utf16le(
          reinterpret_cast<char16_t *>(buf.input.data()),
          buf.input.size() / sizeof(char16_t));
      bool is_ok_utf32 = implementation.validate_utf32(
          reinterpret_cast<char32_t *>(buf.input.data()),
          buf.input.size() / sizeof(char32_t));
      size_t utf8_to_utf16 = implementation.convert_utf8_to_utf16le(
          buf.input.data(), buf.input.size(),
          reinterpret_cast<char16_t *>(buf.output.data()));
      size_t utf8_to_utf32 = implementation.convert_utf8_to_utf32(
          buf.input.data(), buf.input.size(),
          reinterpret_cast<char32_t *>(buf.output.data()));
      size_t utf16_to_utf8 = implementation.convert_utf16le_to_utf8(
          reinterpret_cast<char16_t *>(buf.input.data()),
          buf.input.size() / sizeof(char16_t), buf.output.data());
      size_t utf16_to_utf32 = implementation.convert_utf16le_to_utf32(
          reinterpret_cast<char16_t *>(buf.input.data()),
          buf.input.size() / sizeof(char16_t), reinterpret_cast<char32_t *>(buf.output.data()));
      size_t utf32_to_utf8 = implementation.convert_utf32_to_utf8(
          reinterpret_cast<char32_t *>(buf.input.data()),
          buf.input.size() / sizeof(char32_t), buf.output.data());
      size_t utf32_to_utf16 = implementation.convert_utf32_to_utf16le(
          reinterpret_cast<char32_t *>(buf.input.data()),
          buf.input.size() / sizeof(char32_t), reinterpret_cast<char16_t *>(buf.output.data()));
      if(is_ok_utf8 ? (utf8_to_utf16 == 0 || utf8_to_utf32 == 0) : (utf8_to_utf16 > 0 || utf8_to_utf32 > 0)) {
        std::cout << (is_ok_utf8 ? "UTF-8 is ok" : "UTF-8 is not ok") << std::endl;
        std::cout << " size = " << buf.input.size() << std::endl;
        std::cout << "  implementation.convert_utf8_to_utf16 return " << utf8_to_utf16 << std::endl;
        std::cout << "  implementation.convert_utf8_to_utf32 return " << utf8_to_utf32 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf8 ? (utf8_to_utf16 > 0 && utf8_to_utf32 > 0) : (utf8_to_utf16 == 0 && utf8_to_utf32 == 0));
      if(is_ok_utf16 ? (utf16_to_utf8 == 0 || utf16_to_utf32 == 0) : (utf16_to_utf8 > 0 || utf16_to_utf32 > 0)) {
        std::cout << (is_ok_utf8 ? "UTF-16 is ok" : "UTF-16 is not ok") << std::endl;
        std::cout << " size = " << buf.input.size() / sizeof(char16_t) << std::endl;
        std::cout << "  implementation.convert_utf16_to_utf8 return " << utf16_to_utf8 << std::endl;
        std::cout << "  implementation.convert_utf16_to_utf32 return " << utf16_to_utf32 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf16 ? (utf16_to_utf8 > 0 && utf16_to_utf32 > 0) : (utf16_to_utf8 == 0 && utf16_to_utf32 == 0));
      if(is_ok_utf32 ? (utf32_to_utf8 == 0 || utf32_to_utf16 == 0) : (utf32_to_utf8 > 0 || utf32_to_utf16 > 0)) {
        std::cout << (is_ok_utf8 ? "UTF-16 is ok" : "UTF-16 is not ok") << std::endl;
        std::cout << " size = " << buf.input.size() / sizeof(char32_t) << std::endl;
        std::cout << "  implementation.convert_utf32_to_utf8 return " << utf32_to_utf8 << std::endl;
        std::cout << "  implementation.convert_utf32_to_utf16 return " << utf32_to_utf16 << std::endl;
      }
      ASSERT_TRUE(is_ok_utf32 ? (utf32_to_utf8 > 0 && utf32_to_utf16 > 0) : (utf32_to_utf8 == 0 && utf32_to_utf16 == 0));
    }
  }
}

TEST(overflow_fuzz) {
  std::vector<buffer> buffers;
  for (size_t size : input_size) {
    buffers.emplace_back(size);
  }
  std::mt19937 gen(124); // We want deterministic results.
  size_t counter{0};
  while (counter < 100000) {
    for (buffer &buf : buffers) {
      buf.randomize(gen);
      counter++;
      if ((counter % 10000) == 0) {
        printf("-");
        fflush(NULL);
      }
      bool is_ok_utf8 =
          implementation.validate_utf8(buf.input.data(), buf.input.size());
      bool is_ok_utf16 = implementation.validate_utf16le(
          reinterpret_cast<char16_t *>(buf.input.data()),
          buf.input.size() / sizeof(char16_t));
      bool is_ok_utf32 = implementation.validate_utf32(
          reinterpret_cast<char32_t *>(buf.input.data()),
          buf.input.size() / sizeof(char32_t));
      if (is_ok_utf8) {
        size_t expected_length = implementation.utf16_length_from_utf8(buf.input.data(), buf.input.size());
        buf.output.resize(expected_length);
        size_t utf8_to_utf16 = implementation.convert_utf8_to_utf16le(
            buf.input.data(), buf.input.size(),
            reinterpret_cast<char16_t *>(buf.output.data()));
        ASSERT_TRUE(expected_length > 0 && expected_length == buf.output.size() && expected_length == utf8_to_utf16);

        expected_length = implementation.utf32_length_from_utf8(buf.input.data(), buf.input.size());
        buf.output.resize(expected_length);
        size_t utf8_to_utf32 = implementation.convert_utf8_to_utf32(
            buf.input.data(), buf.input.size(),
            reinterpret_cast<char32_t *>(buf.output.data()));
        ASSERT_TRUE(expected_length > 0 && expected_length == buf.output.size() && expected_length == utf8_to_utf32);
      }
      if (is_ok_utf16) {
        size_t expected_length = implementation.utf8_length_from_utf16le(reinterpret_cast<char16_t *>(buf.input.data()), buf.input.size() / sizeof(char16_t));
        buf.output.resize(expected_length);
        size_t utf16_to_utf8 = implementation.convert_utf16le_to_utf8(
            reinterpret_cast<char16_t *>(buf.input.data()),
            buf.input.size() / sizeof(char16_t), buf.output.data());
        ASSERT_TRUE(expected_length > 0 && expected_length == buf.output.size() && expected_length == utf16_to_utf8);

        expected_length = implementation.utf32_length_from_utf16le(reinterpret_cast<char16_t *>(buf.input.data()), buf.input.size() / sizeof(char16_t));
        buf.output.resize(expected_length);
        size_t utf16_to_utf32 = implementation.convert_utf16le_to_utf32(
            reinterpret_cast<char16_t *>(buf.input.data()),
            buf.input.size() / sizeof(char16_t), reinterpret_cast<char32_t *>(buf.output.data()));
        ASSERT_TRUE(expected_length > 0 && expected_length == buf.output.size() && expected_length == utf16_to_utf32);
      }
      if (is_ok_utf32) {
        size_t expected_length = implementation.utf8_length_from_utf32(reinterpret_cast<char32_t *>(buf.input.data()), buf.input.size() / sizeof(char32_t));
        buf.output.resize(expected_length);
        size_t utf32_to_utf8 = implementation.convert_utf32_to_utf8(
            reinterpret_cast<char32_t *>(buf.input.data()),
            buf.input.size() / sizeof(char32_t), buf.output.data());
        ASSERT_TRUE(expected_length > 0 && expected_length == buf.output.size() && expected_length == utf32_to_utf8);

        expected_length = implementation.utf16_length_from_utf32(reinterpret_cast<char32_t *>(buf.input.data()), buf.input.size() / sizeof(char32_t));
        buf.output.resize(expected_length);
        size_t utf32_to_utf16 = implementation.convert_utf32_to_utf16le(
            reinterpret_cast<char32_t *>(buf.input.data()),
            buf.input.size() / sizeof(char32_t), reinterpret_cast<char16_t *>(buf.output.data()));
        ASSERT_TRUE(expected_length > 0 && expected_length == buf.output.size() && expected_length == utf32_to_utf16);
      }
    }
  }
}


int main(int argc, char *argv[]) { return simdutf::test::main(argc, argv); }