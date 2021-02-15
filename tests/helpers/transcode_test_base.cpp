#include "transcode_test_base.h"

#include <stdexcept>
#include <algorithm>

#include <tests/reference/encode_utf8.h>
#include <tests/reference/encode_utf16.h>

namespace simdutf::tests::helpers {

  TranscodeTestBase::TranscodeTestBase(GenerateCodepoint generate,
                                       size_t input_size, Conversion conversion) {
    while (input.size() < input_size) {
      const uint32_t codepoint = generate();
      prepare_input(codepoint, conversion);
    }

    output.resize(reference_output.size() + output_size_margin);
  }

  void TranscodeTestBase::prepare_input(uint32_t codepoint, Conversion conversion) {
    if (conversion == Conversion::utf8_to_utf16) {
      encode_utf8(codepoint, input);
      encode_utf16(codepoint, reference_output);
    } else {
      encode_utf16(codepoint, input);
      encode_utf8(codepoint, reference_output);
    }
  }

  void TranscodeTestBase::encode_utf8(uint32_t codepoint, std::vector<char>& target) {
    ::simdutf::tests::reference::utf8::encode(codepoint, [&target](uint8_t byte) {
      target.push_back(byte);
    });
  }

 
  bool TranscodeTestBase::validate(size_t saved_bytes) const {
    if (saved_bytes != reference_output.size()) {
      printf("wrong saved bytes value: procedure returned %lu bytes, it should be %lu\n",
             saved_bytes, reference_output.size());
      return false;
    }

    auto [it1, it2] = std::mismatch(output.begin(), output.end(),
                                    reference_output.begin(), reference_output.end());
    if (it1 != input.end()) {
      printf("mismatched output at %lu: actual value 0x%02x, expected 0x%02x\n",
             std::distance(input.begin(), it1), *it1, *it2);
      return false;
    }

    return true;
  }

  void TranscodeTestBase::encode_utf16(uint32_t codepoint, std::vector<char>& target) {
    uint16_t W1;
    uint16_t W2;
    switch (::simdutf::tests::reference::utf16::encode(codepoint, W1, W2)) {
      case 1:
        target.push_back(W1 & 0xff);
        target.push_back(W1 >> 8);
        break;

      case 2:
        target.push_back(W1 & 0xff);
        target.push_back(W1 >> 8);
        target.push_back(W2 & 0xff);
        target.push_back(W2 >> 8);
        break;

      default:
        throw std::invalid_argument("Value can't be encoded as UTF16");
    }
  }
}
