#include "transcode_test_base.h"

#include <stdexcept>
#include <algorithm>
#include <string>

#include <tests/reference/encode_utf8.h>
#include <tests/reference/encode_utf16.h>
#include <tests/reference/decode_utf16.h>
#include <tests/reference/validate_utf16.h>

#include <cstdio>

namespace simdutf::tests::helpers {

  void transcode_test_base::encode_utf8(uint32_t codepoint, std::vector<char>& target) {
    ::simdutf::tests::reference::utf8::encode(codepoint, [&target](uint8_t byte) {
      target.push_back(byte);
    });
  }

  void transcode_test_base::encode_utf16(uint32_t codepoint, std::vector<char16_t>& target) {
    char16_t W1;
    char16_t W2;
    switch (::simdutf::tests::reference::utf16::encode(codepoint, W1, W2)) {
      case 1:
        target.push_back(W1);
        break;

      case 2:
        target.push_back(W1);
        target.push_back(W2);
        break;

      default:
        throw std::invalid_argument(std::string("Value can't be encoded as UTF16 code-point : ") + std::to_string(codepoint));
    }
  }

  /**
   * transcode_utf8_to_utf16_test_base can be used to test UTF8 => UTF16 transcoding.
   */

  transcode_utf8_to_utf16_test_base::transcode_utf8_to_utf16_test_base(GenerateCodepoint generate,
                                       size_t input_size) {
    while (input_utf8.size() < input_size) {
      const uint32_t codepoint = generate();
      prepare_input(codepoint);
    }

    output_utf16.resize(reference_output_utf16.size() + output_size_margin);
  }

  void transcode_utf8_to_utf16_test_base::prepare_input(uint32_t codepoint) {
      encode_utf8(codepoint, input_utf8);
      encode_utf16(codepoint, reference_output_utf16);
  }


  bool transcode_utf8_to_utf16_test_base::validate(size_t saved_chars) const {
    if (saved_chars != reference_output_utf16.size()) {
      printf("wrong saved bytes value: procedure returned %lu bytes, it should be %lu\n",
             saved_chars, reference_output_utf16.size());
      return false;
    }

    // Note that, in general, output_utf16.size() will not matched saved_chars.

    // At this point, we know that the lengths are the same so std::mismatch is enough
    // to tell us whether the strings are identical.
    auto [it1, it2] = std::mismatch(output_utf16.begin(), output_utf16.begin() + saved_chars,
                                    reference_output_utf16.begin(), reference_output_utf16.end());
    if (it1 != output_utf16.begin() + saved_chars) {
      printf("mismatched output at %lu: actual value 0x%04x, expected 0x%04x\n",
             std::distance(output_utf16.begin(), it1), uint16_t(*it1), uint16_t(*it2));
      for(size_t i = 0; i < output_utf16.size(); i++) {
        if(reference_output_utf16[i] != output_utf16[i]) { printf(" ==> "); }
        printf("at %zu expected 0x%04x and got 0x%04x\n ", i, uint16_t(reference_output_utf16[i]), uint16_t(output_utf16[i]));
      }

      return false;
    }

    return true;
  }


  /**
   * transcode_utf16_to_utf8_test_base can be used to test UTF16 => UTF8 transcoding.
   */
  transcode_utf16_to_utf8_test_base::transcode_utf16_to_utf8_test_base(GenerateCodepoint generate,
                                       size_t input_size) {
    while (input_utf16.size() < input_size) {
      const uint32_t codepoint = generate();
      prepare_input(codepoint);
    }

    output_utf8.resize(reference_output_utf8.size() + output_size_margin);
  }

  transcode_utf16_to_utf8_test_base::transcode_utf16_to_utf8_test_base(const std::vector<char16_t>& input_utf16)
    : input_utf16{input_utf16} {

    auto consume = [this](const uint32_t codepoint) {
      ::simdutf::tests::reference::utf8::encode(codepoint, [this](uint8_t byte) {
        reference_output_utf8.push_back(byte);
      });
    };

    auto error_handler = [](const char16_t*, const char16_t*,  simdutf::tests::reference::utf16::Error) -> bool {
      throw std::invalid_argument("Wrong UTF-16 input");
    };
    simdutf::tests::reference::utf16::decode(input_utf16.data(), input_utf16.size(), consume, error_handler);
    output_utf8.resize(reference_output_utf8.size() + output_size_margin);
  }

  void transcode_utf16_to_utf8_test_base::prepare_input(uint32_t codepoint) {
      encode_utf16(codepoint, input_utf16);
      encode_utf8(codepoint, reference_output_utf8);
  }

  bool transcode_utf16_to_utf8_test_base::is_input_valid() const {
    return simdutf::tests::reference::validate_utf16(input_utf16.data(), input_utf16.size());
  }

  bool transcode_utf16_to_utf8_test_base::validate(size_t saved_chars) const {
    if (!is_input_valid()) {
      if (saved_chars != 0) {
        printf("input UTF-16 string is not valid, but conversion routine returned %zu, indicating a valid input\n", saved_chars);
        return false;
      }
    }
    if (saved_chars == 0) {
      if (is_input_valid()) {
        printf("input UTF-16 string is valid, but conversion routine returned 0, indicating input error");
        return false;
      }

      return true;
    }

    auto dump = [this, saved_chars](const char* title, const std::vector<char>& array) {
      printf("%s", title);
      for (size_t i=0; i < saved_chars; i++) {
        printf(" %02x", (uint8_t)array[i]);
      }
      putchar('\n');
    };

    if (saved_chars != reference_output_utf8.size()) {
      printf("wrong saved bytes value: procedure returned %lu bytes, it should be %lu\n",
             saved_chars, reference_output_utf8.size());

      dump("expected :", reference_output_utf8);
      dump("actual   :", output_utf8);
      return false;
    }
    // Note that, in general, output_utf8.size() will not matched saved_chars.

    // At this point, we know that the lengths are the same so std::mismatch is enough
    // to tell us whether the strings are identical.
    auto [it1, it2] = std::mismatch(output_utf8.begin(), output_utf8.begin() + saved_chars,
                                    reference_output_utf8.begin(), reference_output_utf8.end());
    if (it1 != output_utf8.begin() + saved_chars) {
      printf("mismatched output at %lu: actual value 0x%02x, expected 0x%02x\n",
             std::distance(output_utf8.begin(), it1), uint8_t(*it1), uint8_t(*it2));

      dump("expected :", reference_output_utf8);
      dump("actual   :", output_utf8);
      for(size_t i = 0; i < reference_output_utf8.size(); i++) {
        if(reference_output_utf8[i] != output_utf8[i]) { printf(" ==> "); }
        printf("at %zu expected 0x%02x and got 0x%02x\n ", i, uint8_t(reference_output_utf8[i]), uint8_t(output_utf8[i]));
      }
      return false;
    }

    return true;
  }

}
