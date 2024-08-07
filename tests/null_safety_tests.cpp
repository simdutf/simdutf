#include "simdutf.h"
#include <memory>
#include <iostream>

#include <tests/helpers/test.h>
TEST(test_empty) {
    //inputs
    const std::vector<char> i8data;
    const std::vector<char16_t> i16data;
    const std::vector<char32_t> i32data;
    //outputs
    std::vector<char> o8data;
    std::vector<char16_t> o16data;
    std::vector<char32_t> o32data;

    const auto *i8 = i8data.data();
    const auto *i16 = i16data.data();
    const auto *i32 = i32data.data();
    auto *o8 = o8data.data();
    auto *o16 = o16data.data();
    auto *o32 = o32data.data();

    std::ignore = implementation.autodetect_encoding(i8, 0);
    std::ignore = implementation.base64_length_from_binary(0);
    std::ignore = implementation.base64_to_binary(i16, 0, o8);
    std::ignore = implementation.base64_to_binary(i8, 0, o8);
    std::ignore = implementation.binary_to_base64(i8, 0, o8);
    /*std::ignore =*/implementation.change_endianness_utf16(i16, 0, o16);
    std::ignore = implementation.convert_latin1_to_utf16be(i8, 0, o16);
    std::ignore = implementation.convert_latin1_to_utf16le(i8, 0, o16);
    std::ignore = implementation.convert_utf32_to_utf16be(i32, 0, o16);
    std::ignore = implementation.convert_utf32_to_utf16le(i32, 0, o16);
    std::ignore = implementation.convert_utf8_to_utf16be(i8, 0, o16);
    std::ignore = implementation.convert_utf8_to_utf16le(i8, 0, o16);
    std::ignore = implementation.convert_valid_utf16be_to_latin1(i16, 0, o8);
    std::ignore = implementation.convert_valid_utf16be_to_utf32(i16, 0, o32);
    std::ignore = implementation.convert_valid_utf16be_to_utf8(i16, 0, o8);
    std::ignore = implementation.convert_valid_utf16le_to_latin1(i16, 0, o8);
    std::ignore = implementation.convert_valid_utf16le_to_utf32(i16, 0, o32);
    std::ignore = implementation.convert_valid_utf16le_to_utf8(i16, 0, o8);
    std::ignore = implementation.convert_valid_utf32_to_latin1(i32, 0, o8);
    std::ignore = implementation.convert_valid_utf32_to_utf16be(i32, 0, o16);
    std::ignore = implementation.convert_valid_utf32_to_utf16le(i32, 0, o16);
    std::ignore = implementation.convert_valid_utf32_to_utf8(i32, 0, o8);
    std::ignore = implementation.convert_valid_utf8_to_latin1(i8, 0, o8);
    std::ignore = implementation.convert_valid_utf8_to_utf16be(i8, 0, o16);
    std::ignore = implementation.convert_valid_utf8_to_utf16le(i8, 0, o16);
    std::ignore = implementation.convert_valid_utf8_to_utf32(i8, 0, o32);
    std::ignore = implementation.count_utf16be(i16, 0);
    std::ignore = implementation.count_utf16le(i16, 0);
    std::ignore = implementation.count_utf8(i8, 0);
    std::ignore = implementation.latin1_length_from_utf16(0);
    std::ignore = implementation.latin1_length_from_utf32(0);
    std::ignore = implementation.latin1_length_from_utf8(i8, 0);
    std::ignore = implementation.maximal_binary_length_from_base64(i16, 0);
    std::ignore = implementation.utf16_length_from_latin1(0);
    std::ignore = implementation.utf16_length_from_utf32(i32, 0);
    std::ignore = implementation.utf16_length_from_utf8(i8, 0);
    std::ignore = implementation.utf32_length_from_latin1(0);
    std::ignore = implementation.utf32_length_from_utf16be(i16, 0);
    std::ignore = implementation.utf32_length_from_utf16le(i16, 0);
    std::ignore = implementation.utf32_length_from_utf8(i8, 0);
    std::ignore = implementation.utf8_length_from_latin1(i8, 0);
    std::ignore = implementation.utf8_length_from_utf16be(i16, 0);
    std::ignore = implementation.utf8_length_from_utf16le(i16, 0);
    std::ignore = implementation.utf8_length_from_utf32(i32, 0);
    std::ignore = implementation.validate_utf16be(i16, 0);
    std::ignore = implementation.validate_utf16be_with_errors(i16, 0);
    std::ignore = implementation.validate_utf16le(i16, 0);
    std::ignore = implementation.validate_utf16le_with_errors(i16, 0);
    std::ignore = implementation.validate_utf32(i32, 0);
    std::ignore = implementation.validate_utf32_with_errors(i32, 0);
    std::ignore = implementation.validate_utf8(i8, 0);
    std::ignore = implementation.validate_utf8_with_errors(i8, 0);
}

TEST_MAIN
