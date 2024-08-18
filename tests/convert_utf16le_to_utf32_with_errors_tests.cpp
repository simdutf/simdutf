#include "simdutf.h"

#include <array>

#include <tests/helpers/transcode_test_base.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/test.h>


namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using simdutf::tests::helpers::transcode_utf16_to_utf32_test_base;

  constexpr int trials = 1000;
}

TEST(issue_532)
{
    alignas(2) const unsigned char data[] = {
        0x71, 0x71, 0x71, 0x71, 0xab, 0xc8, 0xab, 0xc8, 0xc8, 0xc8, 0xb1, 0xc8, 0xab, 0xb1, 0xc8,
        0xab, 0xc8, 0xb1, 0xc8, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xc8, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8,
        0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x0b, 0xa8, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0x23, 0x47, 0xc8, 0xab,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0x27, 0xc8, 0xab,
        0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8,
        0xc8, 0xab, 0xb1, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xab, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0x0d, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xbe, 0x23, 0x47, 0xd8,
        0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xbd, 0xb1, 0xc8, 0xab, 0xc8, 0xb1,
        0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xc8, 0xb1, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab,
        0x68, 0xfe, 0xc9, 0xf4, 0x49, 0xd9, 0xf4, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xb1, 0x3d, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0x48, 0xab,
        0xc8, 0xab, 0xab, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0x00,
        0x00, 0x00, 0xff, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0x0d, 0x00, 0x00, 0x00, 0x00, 0xb1,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xab, 0xc8, 0xab, 0xc8, 0xbe, 0x23, 0x47, 0xd8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8,
        0xab, 0xc8, 0xbd, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xc8,
        0xb1, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0x68, 0xfe, 0xc9, 0xf4, 0x49, 0xd9, 0xf4,
        0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0x3d, 0xab, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0x48, 0xab, 0xc8, 0xab, 0xab, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8,
        0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xab, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0x00, 0x10, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xcc, 0xa4, 0xa4, 0x23, 0x47, 0xd8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8,
        0xab, 0xc8, 0xab, 0xab, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xab, 0xc8,
        0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xbe, 0x23, 0x47, 0xd8, 0xce, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0x00,
        0x10, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xb1, 0xc8, 0xab, 0x00, 0x08, 0x00, 0x00,
        0x00, 0xff, 0xc8, 0xc8, 0xc8, 0xc8, 0xab, 0xab, 0xb1, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0b, 0xa8, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0x00, 0xff, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8,
        0x6e, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xbe, 0x23, 0x47,
        0xd8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8,
        0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xc8, 0xab, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xab, 0xc8,
        0x47, 0xd8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xab, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab,
        0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8,
        0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xbe, 0x23, 0x47, 0xd8, 0xce,
        0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab,
        0xc8, 0xb1, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0x00, 0x10, 0xb1, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xb1, 0xc8, 0xab, 0x00, 0x08, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xc8, 0xc8, 0xc8,
        0xab, 0xab, 0xb1, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0xa8,
        0xe6, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0x00, 0xff, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8,
        0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xbe, 0x23, 0x47, 0xd8, 0xb1, 0xc8, 0xab, 0xc8, 0xab,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab,
        0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0x00, 0x10, 0xb1, 0xc8, 0xab, 0xc8,
        0xb1, 0xc8, 0xab, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xab, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0x47,
        0xd8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8,
        0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xab, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1,
        0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xbe, 0x23, 0x47, 0xd8, 0xce, 0xb1, 0xc8, 0xab,
        0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8,
        0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0x00, 0x10, 0xb1, 0xc8, 0xab,
        0xc8, 0xb1, 0xc8, 0xab, 0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xab, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc8, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8,
        0xb1, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xab, 0xc8, 0xb1, 0xc8, 0x26};
    constexpr std::size_t data_len_bytes = sizeof(data);
    constexpr std::size_t data_len = data_len_bytes / sizeof(char16_t);
    const auto validation1 = implementation.validate_utf16le_with_errors((const char16_t *) data,
                                                                         data_len);
    ASSERT_EQUAL(validation1.count, 137);
    ASSERT_EQUAL(validation1.error, simdutf::error_code::SURROGATE);

    const bool validation2 = implementation.validate_utf16le((const char16_t *) data, data_len);
    ASSERT_EQUAL(validation1.error == simdutf::error_code::SUCCESS, validation2);
    const auto outlen = implementation.utf32_length_from_utf16le((const char16_t *) data, data_len);
    ASSERT_EQUAL(outlen, 660);
    std::vector<char32_t> output(outlen);
    const auto r = implementation.convert_utf16le_to_utf32_with_errors((const char16_t *) data,
                                                                       data_len,
                                                                       output.data());
    ASSERT_EQUAL(r.error, simdutf::error_code::SURROGATE);
    ASSERT_EQUAL(r.count, 137);
}

TEST(allow_empty_input) {
    std::vector<char16_t> emptydata;
    std::vector<char32_t> output(10);

    auto ret = implementation.convert_utf16le_to_utf32_with_errors(emptydata.data(),
                                                                   emptydata.size(),
                                                                   output.data());
    ASSERT_EQUAL(ret.error, simdutf::error_code::SUCCESS);
}

TEST_LOOP(trials, convert_2_UTF16_bytes) {
    // range for 1, 2 or 3 UTF-8 bytes
    simdutf::tests::helpers::RandomIntRanges random({{0x0000, 0x007f},
                                                     {0x0080, 0x07ff},
                                                     {0x0800, 0xd7ff},
                                                     {0xe000, 0xffff}}, seed);

    auto procedure = [&implementation](const char16_t* utf16, size_t size, char32_t* utf32) -> size_t {
      simdutf::result res = implementation.convert_utf16le_to_utf32_with_errors(utf16, size, utf32);
      ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
      return res.count;
    };
    auto size_procedure = [&implementation](const char16_t* utf16, size_t size) -> size_t {
      return implementation.utf32_length_from_utf16le(utf16, size);
    };
    for (size_t size: input_size) {
      transcode_utf16_to_utf32_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
}

TEST_LOOP(trials, convert_with_surrogates) {
    simdutf::tests::helpers::RandomIntRanges random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, seed);

    auto procedure = [&implementation](const char16_t* utf16, size_t size, char32_t* utf32) -> size_t {
      simdutf::result res = implementation.convert_utf16le_to_utf32_with_errors(utf16, size, utf32);
      ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
      return res.count;
    };
    auto size_procedure = [&implementation](const char16_t* utf16, size_t size) -> size_t {
      return implementation.utf32_length_from_utf16le(utf16, size);
    };
    for (size_t size: input_size) {
      transcode_utf16_to_utf32_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
      ASSERT_TRUE(test.check_size(size_procedure));
    }
}

#if SIMDUTF_IS_BIG_ENDIAN
// todo: port the next test.
#else
TEST(convert_fails_if_there_is_sole_low_surrogate) {
  const size_t size = 64;
  transcode_utf16_to_utf32_test_base test([](){return '*';}, size + 32);

  for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff; low_surrogate++) {
    for (size_t i=0; i < size; i++) {
      auto procedure = [&implementation, &i](const char16_t* utf16, size_t size, char32_t* utf32) -> size_t {
        simdutf::result res = implementation.convert_utf16le_to_utf32_with_errors(utf16, size, utf32);
        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };
      const auto old = test.input_utf16[i];
      test.input_utf16[i] = low_surrogate;
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}
#endif

#if SIMDUTF_IS_BIG_ENDIAN
// todo: port the next test.
#else
TEST(convert_fails_if_there_is_sole_high_surrogate) {
  const size_t size = 64;
  transcode_utf16_to_utf32_test_base test([](){return '*';}, size + 32);

  for (char16_t high_surrogate = 0xdc00; high_surrogate <= 0xdfff; high_surrogate++) {
    for (size_t i=0; i < size; i++) {
      auto procedure = [&implementation, &i](const char16_t* utf16, size_t size, char32_t* utf32) -> size_t {
        simdutf::result res = implementation.convert_utf16le_to_utf32_with_errors(utf16, size, utf32);
        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };
      const auto old = test.input_utf16[i];
      test.input_utf16[i] = high_surrogate;
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i] = old;
    }
  }
}
#endif

#if SIMDUTF_IS_BIG_ENDIAN
// todo: port the next test.
#else
TEST(convert_fails_if_there_is_low_surrogate_is_followed_by_another_low_surrogate) {
  const size_t size = 64;
  transcode_utf16_to_utf32_test_base test([](){return '*';}, size + 32);

  for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff; low_surrogate++) {
    for (size_t i=0; i < size - 1; i++) {
      auto procedure = [&implementation, &i](const char16_t* utf16, size_t size, char32_t* utf32) -> size_t {
        simdutf::result res = implementation.convert_utf16le_to_utf32_with_errors(utf16, size, utf32);
        ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);
        return 0;
      };
      const auto old0 = test.input_utf16[i + 0];
      const auto old1 = test.input_utf16[i + 1];
      test.input_utf16[i + 0] = low_surrogate;
      test.input_utf16[i + 1] = low_surrogate;
      ASSERT_TRUE(test(procedure));
      test.input_utf16[i + 0] = old0;
      test.input_utf16[i + 1] = old1;
    }
  }
}
#endif

#if SIMDUTF_IS_BIG_ENDIAN
// todo: port the next test.
#else
TEST(convert_fails_if_there_is_surrogate_pair_is_followed_by_high_surrogate) {
  const size_t size = 64;
  transcode_utf16_to_utf32_test_base test([](){return '*';}, size + 32);

  const char16_t low_surrogate = 0xd801;
  const char16_t high_surrogate = 0xdc02;
  for (size_t i=0; i < size - 2; i++) {
    auto procedure = [&implementation, &i](const char16_t* utf16, size_t size, char32_t* utf32) -> size_t {
      simdutf::result res = implementation.convert_utf16le_to_utf32_with_errors(utf16, size, utf32);
      ASSERT_EQUAL(res.error, simdutf::error_code::SURROGATE);
      ASSERT_EQUAL(res.count, i + 2);
      return 0;
    };
    const auto old0 = test.input_utf16[i + 0];
    const auto old1 = test.input_utf16[i + 1];
    const auto old2 = test.input_utf16[i + 2];
    test.input_utf16[i + 0] = low_surrogate;
    test.input_utf16[i + 1] = high_surrogate;
    test.input_utf16[i + 2] = high_surrogate;
    ASSERT_TRUE(test(procedure));
    test.input_utf16[i + 0] = old0;
    test.input_utf16[i + 1] = old1;
    test.input_utf16[i + 2] = old2;
  }
}
#endif

TEST_MAIN
