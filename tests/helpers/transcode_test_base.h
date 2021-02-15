#pragma once

#include <cstdint>
#include <vector>
#include <functional>

namespace simdutf::tests::helpers {

  enum class Conversion {
    utf8_to_utf16,
    utf16_to_utf8,
  };

  class TranscodeTestBase {
  public:
    using GenerateCodepoint = std::function<uint32_t()>;

    std::vector<char> input; // source-encoded mesage: what we're going to transcode
    std::vector<char> output; // what the procedure under test produced
    std::vector<char> reference_output; // what we are expecting
    static constexpr size_t output_size_margin = 128; // extra room for buggy procedures

  public:
    TranscodeTestBase(GenerateCodepoint generate, size_t input_size, Conversion conversion);

    template <typename COLLECTION>
    TranscodeTestBase(COLLECTION&& collection, Conversion conversion) {
      for (const uint32_t codepoint: collection) {
        prepare_input(codepoint, conversion);
      }

      output.resize(reference_output.size() + output_size_margin);
    }

    template <typename PROCEDURE>
    bool operator()(PROCEDURE procedure) {
      const size_t saved_bytes = procedure(input.data(), input.size(), output.data());
      return validate(saved_bytes);
    }

  private:
    void prepare_input(uint32_t codepoint, Conversion conversion);
    void encode_utf8(uint32_t codepoint, std::vector<char>& target);
    void encode_utf16(uint32_t codepoint, std::vector<char>& target);
    bool validate(size_t procedure_result) const;
  };

} // namespace simdutf::tests::helpers
