#include "simdutf.h"

#include <array>
#include <memory>
#include <utility>
#include <vector>

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_int.h>
#include <tests/reference/encode_utf8.h>
#include <tests/helpers/test.h>
#include <tests/helpers/transcode_test_base.h>

namespace {
std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

using simdutf::tests::helpers::transcode_utf32_to_utf8_test_base;

} // namespace

#if !SIMDUTF_IS_BIG_ENDIAN
TEST(issue_convert_utf32_to_utf8_with_errors_1b8034ed546f4bf7) {
  alignas(4) const unsigned char data[] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xff, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd5,
      0xd5, 0xd5, 0xd5, 0xd8, 0x00, 0xe2, 0x00, 0xda, 0x59, 0xdc, 0x00, 0x00};
  constexpr size_t data_len_bytes = sizeof(data);
  constexpr size_t data_len = data_len_bytes / sizeof(char32_t);
  const auto validation1 = implementation.validate_utf32_with_errors(
      (const char32_t *)data, data_len);
  ASSERT_EQUAL(validation1.count, 11);
  ASSERT_EQUAL(validation1.error, simdutf::error_code::TOO_LARGE);

  const bool validation2 =
      implementation.validate_utf32((const char32_t *)data, data_len);
  ASSERT_EQUAL(validation1.error == simdutf::error_code::SUCCESS, validation2);

  const auto outlen =
      implementation.utf8_length_from_utf32((const char32_t *)data, data_len);
  std::vector<char> output(outlen);
  const auto r = implementation.convert_utf32_to_utf8_with_errors(
      (const char32_t *)data, data_len, output.data());
  ASSERT_EQUAL(r.error, simdutf::error_code::TOO_LARGE);
  ASSERT_EQUAL(r.count, 11);
}

TEST(issue_convert_utf32_to_utf8_with_errors_cbf29ce484222315) {
  const unsigned char data[] = {
      0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
      0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
      0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
      0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x80, 0x20, 0x00, 0x00, 0x00,
      0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
      0x20, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20};
  constexpr std::size_t data_len_bytes = sizeof(data);
  constexpr std::size_t data_len = data_len_bytes / sizeof(char32_t);
  std::vector<char> output(4 * data_len);
  const auto r = implementation.convert_utf32_to_utf8_with_errors(
      (const char32_t *)data, data_len, output.data());
  /*
  got return [count=10, error=TOO_LARGE] from implementation icelake
  got return [count=10, error=TOO_LARGE] from implementation haswell
  got return [count=16, error=TOO_LARGE] from implementation westmere
  got return [count=10, error=TOO_LARGE] from implementation fallbackend
  errormessage
  */
  ASSERT_EQUAL(r.count, 10);
  ASSERT_EQUAL(r.error, simdutf::error_code::TOO_LARGE);
}
#endif

TEST(convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t { return counter++ & 0x7f; };

  auto procedure = [&implementation](const char32_t *utf32, size_t size,
                                     char *utf8) -> size_t {
    simdutf::result res =
        implementation.convert_utf32_to_utf8_with_errors(utf32, size, utf8);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };
  auto size_procedure = [&implementation](const char32_t *utf32,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_utf32(utf32, size);
  };
  std::array<size_t, 4> input_size{7, 16, 24, 67};
  for (size_t size : input_size) {
    transcode_utf32_to_utf8_test_base test(generator, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(convert_into_1_or_2_UTF8_bytes) {
  simdutf::tests::helpers::RandomInt random(
      0x0000, 0x07ff, seed); // range for 1 or 2 UTF-8 bytes

  auto procedure = [&implementation](const char32_t *utf32, size_t size,
                                     char *utf8) -> size_t {
    simdutf::result res =
        implementation.convert_utf32_to_utf8_with_errors(utf32, size, utf8);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };
  auto size_procedure = [&implementation](const char32_t *utf32,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_utf32(utf32, size);
  };
  for (size_t size : input_size) {
    transcode_utf32_to_utf8_test_base test(random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(convert_into_1_or_2_or_3_UTF8_bytes) {
  // range for 1, 2 or 3 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0000, 0x007f}, {0x0080, 0x07ff}, {0x0800, 0xd7ff}, {0xe000, 0xffff}},
      seed);

  auto procedure = [&implementation](const char32_t *utf32, size_t size,
                                     char *utf8) -> size_t {
    simdutf::result res =
        implementation.convert_utf32_to_utf8_with_errors(utf32, size, utf8);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };
  auto size_procedure = [&implementation](const char32_t *utf32,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_utf32(utf32, size);
  };
  for (size_t size : input_size) {
    transcode_utf32_to_utf8_test_base test(random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST_LOOP(convert_into_3_or_4_UTF8_bytes) {
  // range for 3 or 4 UTF-8 bytes
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x0800, 0xd800 - 1}, {0xe000, 0x10ffff}}, seed);

  auto procedure = [&implementation](const char32_t *utf32, size_t size,
                                     char *utf8) -> size_t {
    simdutf::result res =
        implementation.convert_utf32_to_utf8_with_errors(utf32, size, utf8);
    ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
    return res.count;
  };
  auto size_procedure = [&implementation](const char32_t *utf32,
                                          size_t size) -> size_t {
    return implementation.utf8_length_from_utf32(utf32, size);
  };
  for (size_t size : input_size) {
    transcode_utf32_to_utf8_test_base test(random, size);
    ASSERT_TRUE(test(procedure));
    ASSERT_TRUE(test.check_size(size_procedure));
  }
}

TEST(convert_all_mixed_width_plans_into_exact_output) {
  const std::array<char32_t, 4> codepoints{{0x41, 0x00a2, 0x20ac, 0x1f600}};
  const std::array<std::array<char, 4>, 4> encodings{{
      {{0x41, 0, 0, 0}},
      {{char(0xc2), char(0xa2), 0, 0}},
      {{char(0xe2), char(0x82), char(0xac), 0}},
      {{char(0xf0), char(0x9f), char(0x98), char(0x80)}},
  }};
  const std::array<size_t, 4> widths{{1, 2, 3, 4}};

  std::vector<char32_t> input;
  std::vector<char> expected;
  input.reserve(256 * 4 + 12);
  for (uint16_t plan = 0; plan < 256; plan++) {
    for (size_t lane = 0; lane < 4; lane++) {
      const size_t width = (plan >> (lane * 2)) & 0x3;
      input.push_back(codepoints[width]);
      expected.insert(expected.end(), encodings[width].begin(),
                      encodings[width].begin() + widths[width]);
    }
  }
  // Keep the last four plans in the vector region: the converter intentionally
  // reserves a 12-code-point scalar tail for wide stores.
  for (size_t i = 0; i < 12; i++) {
    input.push_back(codepoints[0]);
    expected.push_back(encodings[0][0]);
  }

  std::unique_ptr<char[]> output(new char[expected.size()]);
  const simdutf::result result =
      implementation.convert_utf32_to_utf8_with_errors(
          input.data(), input.size(), output.get());
  ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(result.count, expected.size());
  for (size_t i = 0; i < expected.size(); i++) {
    ASSERT_EQUAL(output[i], expected[i]);
  }
}

TEST(convert_mixed_width_packed_tail_uses_exact_output) {
  std::vector<char32_t> input(28, 0x41);
  input[0] = 0x1f600;
  std::vector<char> expected;
  expected.reserve(4 * input.size());
  for (const char32_t codepoint : input) {
    simdutf::tests::reference::utf8::encode(
        static_cast<uint32_t>(codepoint), [&expected](uint8_t byte) {
          expected.push_back(static_cast<char>(byte));
        });
  }

  std::unique_ptr<char[]> output(new char[expected.size()]);
  const simdutf::result result =
      implementation.convert_utf32_to_utf8_with_errors(
          input.data(), input.size(), output.get());
  ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(result.count, expected.size());
  for (size_t i = 0; i < expected.size(); i++) {
    ASSERT_EQUAL(output[i], expected[i]);
  }
}

TEST(convert_mixed_width_error_uses_only_the_valid_output_prefix) {
  std::array<char32_t, 28> input{};
  input.fill(0x41);
  input[0] = 0x1f600;
  input[16] = 0x110000;
  std::array<char, 19> output{};

  const simdutf::result result =
      implementation.convert_utf32_to_utf8_with_errors(
          input.data(), input.size(), output.data());
  ASSERT_EQUAL(result.error, simdutf::error_code::TOO_LARGE);
  ASSERT_EQUAL(result.count, 16);
  ASSERT_EQUAL(output[0], char(0xf0));
  ASSERT_EQUAL(output[1], char(0x9f));
  ASSERT_EQUAL(output[2], char(0x98));
  ASSERT_EQUAL(output[3], char(0x80));
  for (size_t i = 4; i < output.size(); i++) {
    ASSERT_EQUAL(output[i], char(0x41));
  }
}

TEST(convert_mixed_width_packer_checks_ahead_of_exact_error_prefix) {
  std::array<char32_t, 56> input{};
  input.fill(0x41);
  input[0] = 0x1f600;
  input[43] = 0x110000;
  std::array<char, 46> output{};

  const simdutf::result result =
      implementation.convert_utf32_to_utf8_with_errors(
          input.data(), input.size(), output.data());
  ASSERT_EQUAL(result.error, simdutf::error_code::TOO_LARGE);
  ASSERT_EQUAL(result.count, 43);
  ASSERT_EQUAL(output[0], char(0xf0));
  ASSERT_EQUAL(output[1], char(0x9f));
  ASSERT_EQUAL(output[2], char(0x98));
  ASSERT_EQUAL(output[3], char(0x80));
  for (size_t i = 4; i < output.size(); i++) {
    ASSERT_EQUAL(output[i], char(0x41));
  }
}

TEST(convert_non_bmp_prefix_then_bmp_into_exact_output) {
  std::vector<char32_t> input(64, 0x41);
  input[0] = 0x1f600;
  std::vector<char> expected;
  expected.reserve(4 * input.size());
  for (const char32_t codepoint : input) {
    simdutf::tests::reference::utf8::encode(
        static_cast<uint32_t>(codepoint), [&expected](uint8_t byte) {
          expected.push_back(static_cast<char>(byte));
        });
  }

  std::unique_ptr<char[]> output(new char[expected.size()]);
  const simdutf::result result =
      implementation.convert_utf32_to_utf8_with_errors(
          input.data(), input.size(), output.get());
  ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(result.count, expected.size());
  for (size_t i = 0; i < expected.size(); i++) {
    ASSERT_EQUAL(output[i], expected[i]);
  }
}

TEST(convert_alternating_bmp_and_mixed_blocks_into_exact_output) {
  const std::array<char32_t, 4> bmp{{0x41, 0x7f, 0x80, 0xffff}};
  const std::array<char32_t, 4> mixed{{0x41, 0x00a2, 0x20ac, 0x1f600}};
  std::vector<char32_t> input(140);
  std::vector<char> expected;
  expected.reserve(4 * input.size());
  for (size_t i = 0; i < input.size(); i++) {
    const bool mixed_block = (i / 16) % 2 != 0;
    const char32_t codepoint =
        i < 128 ? (mixed_block ? mixed[i % mixed.size()] : bmp[i % bmp.size()])
                : bmp[i % bmp.size()];
    input[i] = codepoint;
    simdutf::tests::reference::utf8::encode(
        static_cast<uint32_t>(codepoint), [&expected](uint8_t byte) {
          expected.push_back(static_cast<char>(byte));
        });
  }

  std::unique_ptr<char[]> output(new char[expected.size()]);
  const simdutf::result result =
      implementation.convert_utf32_to_utf8_with_errors(
          input.data(), input.size(), output.get());
  ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(result.count, expected.size());
  for (size_t i = 0; i < expected.size(); i++) {
    ASSERT_EQUAL(output[i], expected[i]);
  }
}

TEST(convert_mixed_width_after_bmp_prefix_into_exact_output) {
  const std::array<char32_t, 8> bmp{
      {0, 0x7f, 0x80, 0x7ff, 0x800, 0xd7ff, 0xe000, 0xffff}};
  const std::array<char32_t, 4> mixed{{0x7f, 0x80, 0xffff, 0x10ffff}};
  std::vector<char32_t> input(128);
  std::vector<char> expected;
  expected.reserve(4 * input.size());
  for (size_t i = 0; i < input.size(); i++) {
    const char32_t codepoint =
        i < 16 ? bmp[i % bmp.size()] : mixed[i % mixed.size()];
    input[i] = codepoint;
    simdutf::tests::reference::utf8::encode(
        static_cast<uint32_t>(codepoint), [&expected](uint8_t byte) {
          expected.push_back(static_cast<char>(byte));
        });
  }

  std::unique_ptr<char[]> output(new char[expected.size()]);
  const simdutf::result result =
      implementation.convert_utf32_to_utf8_with_errors(
          input.data(), input.size(), output.get());
  ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(result.count, expected.size());
  for (size_t i = 0; i < expected.size(); i++) {
    ASSERT_EQUAL(output[i], expected[i]);
  }
}

TEST(convert_four_byte_prefix_then_mixed_into_exact_output) {
  const std::array<char32_t, 4> four_byte{
      {0x10000, 0x1f600, 0x10fffd, 0x10ffff}};
  const std::array<char32_t, 4> mixed{{0x7f, 0x80, 0xffff, 0x10000}};
  // Exercise the four-byte kernel and its mixed-width handoff.
  std::vector<char32_t> input(128);
  std::vector<char> expected;
  expected.reserve(4 * input.size());
  for (size_t i = 0; i < input.size(); i++) {
    const char32_t codepoint = i < 16   ? four_byte[i % four_byte.size()]
                               : i < 32 ? mixed[i % mixed.size()]
                                        : char32_t(0x41);
    input[i] = codepoint;
    simdutf::tests::reference::utf8::encode(
        static_cast<uint32_t>(codepoint), [&expected](uint8_t byte) {
          expected.push_back(static_cast<char>(byte));
        });
  }

  std::unique_ptr<char[]> output(new char[expected.size()]);
  const simdutf::result result =
      implementation.convert_utf32_to_utf8_with_errors(
          input.data(), input.size(), output.get());
  ASSERT_EQUAL(result.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(result.count, expected.size());
  for (size_t i = 0; i < expected.size(); i++) {
    ASSERT_EQUAL(output[i], expected[i]);
  }
}

TEST(convert_mixed_width_wide_lookahead_preserves_exact_error_prefix) {
  const std::array<char32_t, 4> valid{{0x41, 0x00a2, 0x20ac, 0x1f600}};
  for (const std::pair<char32_t, simdutf::error_code> invalid : {
           std::make_pair(char32_t(0xd800), simdutf::error_code::SURROGATE),
           std::make_pair(char32_t(0x110000), simdutf::error_code::TOO_LARGE),
       }) {
    for (size_t error_position = 0; error_position < 140; error_position++) {
      std::vector<char32_t> input(140);
      std::vector<char> expected;
      expected.reserve(4 * error_position);
      for (size_t i = 0; i < input.size(); i++) {
        input[i] = valid[i % valid.size()];
        if (i < error_position) {
          simdutf::tests::reference::utf8::encode(
              static_cast<uint32_t>(input[i]), [&expected](uint8_t byte) {
                expected.push_back(static_cast<char>(byte));
              });
        }
      }
      input[error_position] = invalid.first;
      std::unique_ptr<char[]> output(new char[expected.size()]);
      const simdutf::result result =
          implementation.convert_utf32_to_utf8_with_errors(
              input.data(), input.size(), output.get());
      ASSERT_EQUAL(result.error, invalid.second);
      ASSERT_EQUAL(result.count, error_position);
      for (size_t i = 0; i < expected.size(); i++) {
        ASSERT_EQUAL(output[i], expected[i]);
      }
    }
  }
}

TEST(convert_four_byte_errors_preserve_positions) {
  const std::array<char32_t, 4> valid{{0x10000, 0x1f600, 0x10fffd, 0x10ffff}};
  for (const std::pair<char32_t, simdutf::error_code> invalid : {
           std::make_pair(char32_t(0xd800), simdutf::error_code::SURROGATE),
           std::make_pair(char32_t(0x110000), simdutf::error_code::TOO_LARGE),
       }) {
    for (size_t error_position = 0; error_position < 32; error_position++) {
      std::vector<char32_t> input(64);
      for (size_t i = 0; i < input.size(); i++) {
        input[i] = valid[i % valid.size()];
      }
      input[error_position] = invalid.first;
      std::vector<char> output(4 * input.size());
      const simdutf::result result =
          implementation.convert_utf32_to_utf8_with_errors(
              input.data(), input.size(), output.data());
      ASSERT_EQUAL(result.error, invalid.second);
      ASSERT_EQUAL(result.count, error_position);
    }
  }
}

TEST(convert_mixed_width_errors_preserve_positions) {
  const std::array<char32_t, 4> valid{{0x41, 0x00a2, 0x20ac, 0x1f600}};
  for (const size_t bmp_prefix : {size_t(0), size_t(16)}) {
    for (const std::pair<char32_t, simdutf::error_code> invalid : {
             std::make_pair(char32_t(0xd800), simdutf::error_code::SURROGATE),
             std::make_pair(char32_t(0x110000), simdutf::error_code::TOO_LARGE),
         }) {
      for (size_t error_position = 0; error_position < 32; error_position++) {
        std::vector<char32_t> input(64);
        for (size_t i = 0; i < input.size(); i++) {
          input[i] = i < bmp_prefix ? valid[i % 3] : valid[i % valid.size()];
        }
        input[error_position] = invalid.first;
        std::vector<char> output(4 * input.size());
        const simdutf::result result =
            implementation.convert_utf32_to_utf8_with_errors(
                input.data(), input.size(), output.data());
        ASSERT_EQUAL(result.error, invalid.second);
        ASSERT_EQUAL(result.count, error_position);
      }
    }
  }
}

TEST(convert_fails_if_there_is_surrogate) {
  const size_t size = 64;
  transcode_utf32_to_utf8_test_base test([]() { return '*'; }, size + 32);

  for (char32_t surrogate = 0xd800; surrogate <= 0xdfff; surrogate++) {
    for (size_t i = 0; i < size; i++) {
      auto procedure = [&implementation, &i](const char32_t *utf32, size_t size,
                                             char *utf8) -> size_t {
        simdutf::result res =
            implementation.convert_utf32_to_utf8_with_errors(utf32, size, utf8);
        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };
      const auto old = test.input_utf32[i];
      test.input_utf32[i] = surrogate;
      ASSERT_TRUE(test(procedure));
      test.input_utf32[i] = old;
    }
  }
}

TEST(convert_fails_if_input_too_large) {
  uint32_t seed{1234};
  simdutf::tests::helpers::RandomInt generator(0x110000, 0xffffffff, seed);

  const size_t size = 64;
  transcode_utf32_to_utf8_test_base test([]() { return '*'; }, size + 32);

  for (size_t j = 0; j < 1000; j++) {
    uint32_t wrong_value = generator();
    for (size_t i = 0; i < size; i++) {
      auto procedure = [&implementation, &i](const char32_t *utf32, size_t size,
                                             char *utf8) -> size_t {
        simdutf::result res =
            implementation.convert_utf32_to_utf8_with_errors(utf32, size, utf8);
        ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LARGE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };
      auto old = test.input_utf32[i];
      test.input_utf32[i] = wrong_value;
      ASSERT_TRUE(test(procedure));
      test.input_utf32[i] = old;
    }
  }
}

TEST(special_cases) {
  const uint32_t utf32[] = {0x0000, 0x0054, 0x0001, 0x0000, 0x0000,
                            0x0007, 0x005d, 0x027f, 0x001a};
  const char expected[] = "\x00\x54\x01\x00\x00\x07\x5d\xc9\xbf\x1a";
  size_t utf8len =
      implementation.utf8_length_from_utf32((const char32_t *)utf32, 9);
  std::unique_ptr<char[]> utf8(new char[utf8len]);
  simdutf::result res = implementation.convert_utf32_to_utf8_with_errors(
      (const char32_t *)utf32, 9, utf8.get());
  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  size_t utf8size = res.count;
  for (size_t i = 0; i < utf8len; i++) {
    ASSERT_EQUAL(utf8[i], expected[i]);
  }
  ASSERT_EQUAL(utf8size, utf8len);
}

#if SIMDUTF_CPLUSPLUS23

namespace {
template <auto input> constexpr auto size() {
  return simdutf::utf8_length_from_utf32(input);
}

template <auto input> constexpr auto convert() {
  using namespace simdutf::tests::helpers;
  CTString<char8_t, size<input>()> tmp;
  const auto ret = simdutf::convert_utf32_to_utf8_with_errors(input, tmp);
  if (ret.count != tmp.size()) {
    throw "unexpected write size";
  }
  return tmp;
}
} // namespace

TEST(compile_time_convert_utf32_to_utf8_with_errors) {
  using namespace simdutf::tests::helpers;
  constexpr auto input = U"köttbulle"_utf32;
  constexpr auto expected = u8"köttbulle"_utf8;
  constexpr auto output = convert<input>();
  static_assert(output == expected);
}

#endif

TEST_MAIN
