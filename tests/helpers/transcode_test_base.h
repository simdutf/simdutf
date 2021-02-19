#pragma once

#include <cstdint>
#include <vector>
#include <functional>

namespace simdutf::tests::helpers {

  /**
   * It would be nice to use a single helper for both UTF8 => UTF16
   * and UTF8 <= UTF16, but the function signatures differ when you
   * represent UTF8 with char8_t and UTF16 with char16_t.
   */


  class transcode_test_base {
  protected:
    void encode_utf8(uint32_t codepoint, std::vector<char>& target);
    void encode_utf16(uint32_t codepoint, std::vector<char16_t>& target);
  };

  /**
   * This class can be used to test UTF8 => UTF16 transcoding.
   */ 
  class transcode_utf8_to_utf16_test_base : transcode_test_base {
  public:
    using GenerateCodepoint = std::function<uint32_t()>;

    std::vector<char> input_utf8; // source-encoded mesage: what we're going to transcode
    std::vector<char16_t> output_utf16; // what the procedure under test produced
    std::vector<char16_t> reference_output_utf16; // what we are expecting

    static constexpr size_t output_size_margin = 128; // extra room for buggy procedures

  public:
    transcode_utf8_to_utf16_test_base(GenerateCodepoint generate, size_t input_size);

    template <typename COLLECTION>
    transcode_utf8_to_utf16_test_base(COLLECTION&& collection) {
      for (const uint32_t codepoint: collection) {
        prepare_input(codepoint);
      }
      output_utf16.resize(reference_output_utf16.size() + output_size_margin);
    }

    template <typename PROCEDURE>
    bool operator()(PROCEDURE procedure) {
      size_t saved_chars = procedure(input_utf8.data(), input_utf8.size(), output_utf16.data());
      return validate(saved_chars);
    }

  private:
    void prepare_input(uint32_t codepoint);
    bool validate(size_t procedure_result) const;
  };


  /**
   * This class can be used to test UTF16 => UTF8 transcoding.
   */
  class transcode_utf16_to_utf8_test_base : transcode_test_base {
  public:
    using GenerateCodepoint = std::function<uint32_t()>;

    std::vector<char> output_utf8; // what the procedure under test produced
    std::vector<char> reference_output_utf8; // what we are expecting

    std::vector<char16_t> input_utf16; // source-encoded mesage: what we're going to transcode

    static constexpr size_t output_size_margin = 128; // extra room for buggy procedures

  public:
    transcode_utf16_to_utf8_test_base(GenerateCodepoint generate, size_t input_size);

    template <typename COLLECTION>
    transcode_utf16_to_utf8_test_base(COLLECTION&& collection) {
      for (const uint32_t codepoint: collection) {
        prepare_input(codepoint);
      }
      output_utf8.resize(reference_output_utf8.size() + output_size_margin);
    }

    template <typename PROCEDURE>
    bool operator()(PROCEDURE procedure) {
      size_t saved_chars = procedure(input_utf16.data(), input_utf16.size(), output_utf8.data());
      return validate(saved_chars);
    }

  private:
    void prepare_input(uint32_t codepoint);
    bool validate(size_t procedure_result) const;
  };


} // namespace simdutf::tests::helpers
