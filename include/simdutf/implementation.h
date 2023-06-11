#ifndef SIMDUTF_IMPLEMENTATION_H
#define SIMDUTF_IMPLEMENTATION_H
#include <string>
#if !defined(SIMDUTF_NO_THREADS)
#include <atomic>
#endif
#include <vector>
#include <tuple>
#include "simdutf/common_defs.h"
#include "simdutf/internal/isadetection.h"


namespace simdutf {

/**
 * Autodetect the encoding of the input, a single encoding is recommended.
 * E.g., the function might return simdutf::encoding_type::UTF8,
 * simdutf::encoding_type::UTF16_LE, simdutf::encoding_type::UTF16_BE, or
 * simdutf::encoding_type::UTF32_LE.
 *
 * @param input the string to analyze.
 * @param length the length of the string in bytes.
 * @return the detected encoding type
 */
simdutf_warn_unused simdutf::encoding_type autodetect_encoding(const char * input, size_t length) noexcept;
simdutf_really_inline simdutf_warn_unused simdutf::encoding_type autodetect_encoding(const uint8_t * input, size_t length) noexcept {
  return autodetect_encoding(reinterpret_cast<const char *>(input), length);
}

/**
 * Autodetect the possible encodings of the input in one pass.
 * E.g., if the input might be UTF-16LE or UTF-8, this function returns
 * the value (simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE).
 *
 * Overriden by each implementation.
 *
 * @param input the string to analyze.
 * @param length the length of the string in bytes.
 * @return the detected encoding type
 */
simdutf_warn_unused int detect_encodings(const char * input, size_t length) noexcept;
simdutf_really_inline simdutf_warn_unused int detect_encodings(const uint8_t * input, size_t length) noexcept {
  return detect_encodings(reinterpret_cast<const char *>(input), length);
}

/**
 * Convert Latin1 string into UTF-16BE string.
 *
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the Latin1 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the conversion is not possible
 */
simdutf_warn_unused size_t convert_latin1_to_utf16be(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert Latin1 string into UTF-16LE string.
 *
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the Latin1 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the conversion is not possible
 */
simdutf_warn_unused size_t convert_latin1_to_utf16le(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Validate the UTF-8 string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * validate_utf8_with_errors.
 *
 * Overridden by each implementation.
 *
 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return true if and only if the string is valid UTF-8.
 */
simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) noexcept;

/**
 * Validate the UTF-8 string and stop on error.
 *
 * Overridden by each implementation.
 *
 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
simdutf_warn_unused result validate_utf8_with_errors(const char *buf, size_t len) noexcept;

/**
 * Validate the ASCII string.
 *
 * Overridden by each implementation.
 *
 * @param buf the ASCII string to validate.
 * @param len the length of the string in bytes.
 * @return true if and only if the string is valid ASCII.
 */
simdutf_warn_unused bool validate_ascii(const char *buf, size_t len) noexcept;

/**
 * Validate the ASCII string and stop on error. It might be faster than
 * validate_utf8 when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * @param buf the ASCII string to validate.
 * @param len the length of the string in bytes.
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
simdutf_warn_unused result validate_ascii_with_errors(const char *buf, size_t len) noexcept;

/**
 * Using native endianness; Validate the UTF-16 string.
 * This function may be best when you expect the input to be almost always valid.
 * Otherwise, consider using validate_utf16_with_errors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16 string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return true if and only if the string is valid UTF-16.
 */
simdutf_warn_unused bool validate_utf16(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-16LE string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * validate_utf16le_with_errors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16LE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return true if and only if the string is valid UTF-16LE.
 */
simdutf_warn_unused bool validate_utf16le(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-16BE string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * validate_utf16be_with_errors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16BE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return true if and only if the string is valid UTF-16BE.
 */
simdutf_warn_unused bool validate_utf16be(const char16_t *buf, size_t len) noexcept;

/**
 * Using native endianness; Validate the UTF-16 string and stop on error.
 * It might be faster than validate_utf16 when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16 string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
simdutf_warn_unused result validate_utf16_with_errors(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-16LE string and stop on error. It might be faster than
 * validate_utf16le when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16LE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
simdutf_warn_unused result validate_utf16le_with_errors(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-16BE string and stop on error. It might be faster than
 * validate_utf16be when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16BE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
simdutf_warn_unused result validate_utf16be_with_errors(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-32 string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * validate_utf32_with_errors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-32 string to validate.
 * @param len the length of the string in number of 4-byte words (char32_t).
 * @return true if and only if the string is valid UTF-32.
 */
simdutf_warn_unused bool validate_utf32(const char32_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-32 string and stop on error. It might be faster than
 * validate_utf32 when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-32 string to validate.
 * @param len the length of the string in number of 4-byte words (char32_t).
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
simdutf_warn_unused result validate_utf32_with_errors(const char32_t *buf, size_t len) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-8 string into UTF-16 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
 */
simdutf_warn_unused size_t convert_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16LE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
 */
simdutf_warn_unused size_t convert_utf8_to_utf16le(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16BE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
 */
simdutf_warn_unused size_t convert_utf8_to_utf16be(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-8 string into UTF-16
 * string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
simdutf_warn_unused result convert_utf8_to_utf16_with_errors(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16LE string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
simdutf_warn_unused result convert_utf8_to_utf16le_with_errors(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16BE string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
simdutf_warn_unused result convert_utf8_to_utf16be_with_errors(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char32_t; 0 if the input was not valid UTF-8 string
 */
simdutf_warn_unused size_t convert_utf8_to_utf32(const char * input, size_t length, char32_t* utf32_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
 */
simdutf_warn_unused result convert_utf8_to_utf32_with_errors(const char * input, size_t length, char32_t* utf32_output) noexcept;

/**
 * Using native endianness; Convert valid UTF-8 string into UTF-16 string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-8 string into UTF-16LE string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf16le(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-8 string into UTF-16BE string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf16be(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-8 string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char32_t
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf32(const char * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Compute the number of 2-byte words that this UTF-8 string would require in UTF-16LE format.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return the number of char16_t words required to encode the UTF-8 string as UTF-16LE
 */
simdutf_warn_unused size_t utf16_length_from_utf8(const char * input, size_t length) noexcept;

/**
 * Compute the number of 4-byte words that this UTF-8 string would require in UTF-32 format.
 *
 * This function is equivalent to count_utf8
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return the number of char32_t words required to encode the UTF-8 string as UTF-32
 */
simdutf_warn_unused size_t utf32_length_from_utf8(const char * input, size_t length) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-16 string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
simdutf_warn_unused size_t convert_utf16le_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-16BE string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
simdutf_warn_unused size_t convert_utf16be_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-16 string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
 */
simdutf_warn_unused result convert_utf16_to_utf8_with_errors(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
 */
simdutf_warn_unused result convert_utf16le_to_utf8_with_errors(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-16BE string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
 */
simdutf_warn_unused result convert_utf16be_to_utf8_with_errors(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Using native endianness; Convert valid UTF-16 string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-16LE string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16le_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-16BE string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16BE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16be_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-16 string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
simdutf_warn_unused size_t convert_utf16_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
simdutf_warn_unused size_t convert_utf16le_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert possibly broken UTF-16BE string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
simdutf_warn_unused size_t convert_utf16be_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-16 string into
 * UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
 */
simdutf_warn_unused result convert_utf16_to_utf32_with_errors(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
 */
simdutf_warn_unused result convert_utf16le_to_utf32_with_errors(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert possibly broken UTF-16BE string into UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
 */
simdutf_warn_unused result convert_utf16be_to_utf32_with_errors(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Using native endianness; Convert valid UTF-16 string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-16 (native endianness).
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert valid UTF-16LE string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16le_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert valid UTF-16BE string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16be_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Using native endianness; Compute the number of bytes that this UTF-16
 * string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-8
 */
simdutf_warn_unused size_t utf8_length_from_utf16(const char16_t * input, size_t length) noexcept;

/**
 * Compute the number of bytes that this UTF-16LE string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-8
 */
simdutf_warn_unused size_t utf8_length_from_utf16le(const char16_t * input, size_t length) noexcept;

/**
 * Compute the number of bytes that this UTF-16BE string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16BE string as UTF-8
 */
simdutf_warn_unused size_t utf8_length_from_utf16be(const char16_t * input, size_t length) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32 string
 */
simdutf_warn_unused size_t convert_utf32_to_utf8(const char32_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
 */
simdutf_warn_unused result convert_utf32_to_utf8_with_errors(const char32_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-32 string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf32_to_utf8(const char32_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-32 string into UTF-16 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32 string
 */
simdutf_warn_unused size_t convert_utf32_to_utf16(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-16LE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32 string
 */
simdutf_warn_unused size_t convert_utf32_to_utf16le(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-16BE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32 string
 */
simdutf_warn_unused size_t convert_utf32_to_utf16be(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-32 string into UTF-16
 * string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
simdutf_warn_unused result convert_utf32_to_utf16_with_errors(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-16LE string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
simdutf_warn_unused result convert_utf32_to_utf16le_with_errors(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-16BE string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
simdutf_warn_unused result convert_utf32_to_utf16be_with_errors(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Using native endianness; Convert valid UTF-32 string into UTF-16 string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf32_to_utf16(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-32 string into UTF-16LE string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf32_to_utf16le(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-32 string into UTF-16BE string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf32_to_utf16be(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Change the endianness of the input. Can be used to go from UTF-16LE to UTF-16BE or
 * from UTF-16BE to UTF-16LE.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param output        the pointer to buffer that can hold the conversion result
 */
void change_endianness_utf16(const char16_t * input, size_t length, char16_t * output) noexcept;

/**
 * Compute the number of bytes that this UTF-32 string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @return the number of bytes required to encode the UTF-32 string as UTF-8
 */
simdutf_warn_unused size_t utf8_length_from_utf32(const char32_t * input, size_t length) noexcept;

/**
 * Compute the number of two-byte words that this UTF-32 string would require in UTF-16 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @return the number of bytes required to encode the UTF-32 string as UTF-16
 */
simdutf_warn_unused size_t utf16_length_from_utf32(const char32_t * input, size_t length) noexcept;

/**
 * Using native endianness; Compute the number of bytes that this UTF-16
 * string would require in UTF-32 format.
 *
 * This function is equivalent to count_utf16.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-32
 */
simdutf_warn_unused size_t utf32_length_from_utf16(const char16_t * input, size_t length) noexcept;

/**
 * Compute the number of bytes that this UTF-16LE string would require in UTF-32 format.
 *
 * This function is equivalent to count_utf16le.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-32
 */
simdutf_warn_unused size_t utf32_length_from_utf16le(const char16_t * input, size_t length) noexcept;

/**
 * Compute the number of bytes that this UTF-16BE string would require in UTF-32 format.
 *
 * This function is equivalent to count_utf16be.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16BE string as UTF-32
 */
simdutf_warn_unused size_t utf32_length_from_utf16be(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16 (native endianness).
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf16(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf16le(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16BE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf16be(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf8(const char * input, size_t length) noexcept;

/**
 * An implementation of simdutf for a particular CPU architecture.
 *
 * Also used to maintain the currently active implementation. The active implementation is
 * automatically initialized on first use to the most advanced implementation supported by the host.
 */
class implementation {
public:

  /**
   * The name of this implementation.
   *
   *     const implementation *impl = simdutf::active_implementation;
   *     cout << "simdutf is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &name() const { return _name; }

  /**
   * The description of this implementation.
   *
   *     const implementation *impl = simdutf::active_implementation;
   *     cout << "simdutf is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &description() const { return _description; }

  /**
   * The instruction sets this implementation is compiled against
   * and the current CPU match. This function may poll the current CPU/system
   * and should therefore not be called too often if performance is a concern.
   *
   *
   * @return true if the implementation can be safely used on the current system (determined at runtime)
   */
  bool supported_by_runtime_system() const;

  /**
   * This function will try to detect the encoding
   * @param input the string to identify
   * @param length the length of the string in bytes.
   * @return the encoding type detected
   */
  virtual encoding_type autodetect_encoding(const char * input, size_t length) const noexcept;

  /**
   * This function will try to detect the possible encodings in one pass
   * @param input the string to identify
   * @param length the length of the string in bytes.
   * @return the encoding type detected
   */
  virtual int detect_encodings(const char * input, size_t length) const noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * The instruction sets this implementation is compiled against.
   *
   * @return a mask of all required `internal::instruction_set::` values
   */
  virtual uint32_t required_instruction_sets() const { return _required_instruction_sets; }


  /**
   * Validate the UTF-8 string.
   *
   * Overridden by each implementation.
   *
   * @param buf the UTF-8 string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid UTF-8.
   */
  simdutf_warn_unused virtual bool validate_utf8(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-8 string and stop on errors.
   *
   * Overridden by each implementation.
   *
   * @param buf the UTF-8 string to validate.
   * @param len the length of the string in bytes.
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  simdutf_warn_unused virtual result validate_utf8_with_errors(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the ASCII string.
   *
   * Overridden by each implementation.
   *
   * @param buf the ASCII string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid ASCII.
   */
  simdutf_warn_unused virtual bool validate_ascii(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the ASCII string and stop on error.
   *
   * Overridden by each implementation.
   *
   * @param buf the ASCII string to validate.
   * @param len the length of the string in bytes.
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  simdutf_warn_unused virtual result validate_ascii_with_errors(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16LE string.This function may be best when you expect
   * the input to be almost always valid. Otherwise, consider using
   * validate_utf16le_with_errors.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-16LE string to validate.
   * @param len the length of the string in number of 2-byte words (char16_t).
   * @return true if and only if the string is valid UTF-16LE.
   */
  simdutf_warn_unused virtual bool validate_utf16le(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16BE string. This function may be best when you expect
   * the input to be almost always valid. Otherwise, consider using
   * validate_utf16be_with_errors.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-16BE string to validate.
   * @param len the length of the string in number of 2-byte words (char16_t).
   * @return true if and only if the string is valid UTF-16BE.
   */
  simdutf_warn_unused virtual bool validate_utf16be(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16LE string and stop on error.  It might be faster than
 * validate_utf16le when an error is expected to occur early.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-16LE string to validate.
   * @param len the length of the string in number of 2-byte words (char16_t).
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  simdutf_warn_unused virtual result validate_utf16le_with_errors(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16BE string and stop on error. It might be faster than
   * validate_utf16be when an error is expected to occur early.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-16BE string to validate.
   * @param len the length of the string in number of 2-byte words (char16_t).
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  simdutf_warn_unused virtual result validate_utf16be_with_errors(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-32 string.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-32 string to validate.
   * @param len the length of the string in number of 4-byte words (char32_t).
   * @return true if and only if the string is valid UTF-32.
   */
  simdutf_warn_unused virtual bool validate_utf32(const char32_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-32 string and stop on error.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-32 string to validate.
   * @param len the length of the string in number of 4-byte words (char32_t).
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  simdutf_warn_unused virtual result validate_utf32_with_errors(const char32_t *buf, size_t len) const noexcept = 0;

  /**
   * Convert Latin1 string into UTF8 string.
   *
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the Latin1 string to convert
   * @param length        the length of the string in bytes
   * @param latin1_output  the pointer to buffer that can hold conversion result
   * @return the number of written char; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_latin1_to_utf8(const char * input, size_t length, char* utf8_output) const noexcept = 0;
  

    /**
   * Convert possibly Latin1 string into UTF-16LE string.
   *
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the Latin1  string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_latin1_to_utf16le(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert Latin1 string into UTF-16BE string.
   *
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the Latin1 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_latin1_to_utf16be(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert Latin1 string into UTF-32 string.
   *
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the Latin1 string to convert
   * @param length        the length of the string in bytes
   * @param utf32_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char32_t; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_latin1_to_utf32(const char * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

 /**
   * Convert possibly broken UTF-8 string into latin1 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param latin1_output  the pointer to buffer that can hold conversion result
   * @return the number of written char; 0 if the input was not valid UTF-8 string
   */
  simdutf_warn_unused virtual size_t convert_utf8_to_latin1(const char * input, size_t length, char* latin1_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into latin1 string. with errors
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param latin1_output  the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  simdutf_warn_unused virtual result convert_utf8_to_latin1_with_errors(const char * input, size_t length, char* latin1_output) const noexcept = 0;

    /**
   * Convert valid UTF-8 string into latin1 string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * This function is not BOM-aware.
   *    
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param latin1_output  the pointer to buffer that can hold conversion result
   * @return the number of written char; 0 if the input was not valid UTF-8 string
   */
  simdutf_warn_unused virtual size_t convert_valid_utf8_to_latin1(const char * input, size_t length, char* latin1_output) const noexcept = 0;


  /**
   * Convert possibly broken UTF-8 string into UTF-16LE string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
   */
  simdutf_warn_unused virtual size_t convert_utf8_to_utf16le(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-16BE string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
   */
  simdutf_warn_unused virtual size_t convert_utf8_to_utf16be(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-16LE string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  simdutf_warn_unused virtual result convert_utf8_to_utf16le_with_errors(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-16BE string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  simdutf_warn_unused virtual result convert_utf8_to_utf16be_with_errors(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-32 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf32_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
   */
  simdutf_warn_unused virtual size_t convert_utf8_to_utf32(const char * input, size_t length, char32_t* utf32_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-32 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf32_buffer  the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
   */
  simdutf_warn_unused virtual result convert_utf8_to_utf32_with_errors(const char * input, size_t length, char32_t* utf32_output) const noexcept = 0;

  /**
   * Convert valid UTF-8 string into UTF-16LE string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t
   */
  simdutf_warn_unused virtual size_t convert_valid_utf8_to_utf16le(const char * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

/**
   * Convert valid UTF-8 string into UTF-16BE string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t
   */
  simdutf_warn_unused virtual size_t convert_valid_utf8_to_utf16be(const char * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-8 string into UTF-32 string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char32_t
   */
  simdutf_warn_unused virtual size_t convert_valid_utf8_to_utf32(const char * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Compute the number of 2-byte words that this UTF-8 string would require in UTF-16LE format.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-8 string to process
   * @param length        the length of the string in bytes
   * @return the number of char16_t words required to encode the UTF-8 string as UTF-16LE
   */
  simdutf_warn_unused virtual size_t utf16_length_from_utf8(const char * input, size_t length) const noexcept = 0;

   /**
   * Compute the number of 4-byte words that this UTF-8 string would require in UTF-32 format.
   *
   * This function is equivalent to count_utf8.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-8 string to process
   * @param length        the length of the string in bytes
   * @return the number of char32_t words required to encode the UTF-8 string as UTF-32
   */
  simdutf_warn_unused virtual size_t utf32_length_from_utf8(const char * input, size_t length) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into Latin1 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param latin1_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16LE string
   */
  simdutf_warn_unused virtual size_t convert_utf16le_to_latin1(const char16_t * input, size_t length, char* latin1_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into Latin1 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param latin1_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  simdutf_warn_unused virtual size_t convert_utf16be_to_latin1(const char16_t * input, size_t length, char* latin1_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into Latin1 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.   
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param latin1_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  simdutf_warn_unused virtual result convert_utf16le_to_latin1_with_errors(const char16_t * input, size_t length, char* latin1_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into Latin1 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.   
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param latin1_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  simdutf_warn_unused virtual result convert_utf16be_to_latin1_with_errors(const char16_t * input, size_t length, char* latin1_buffer) const noexcept = 0;
  /**
   * Convert valid UTF-16LE string into Latin1 string.
   *
   * This function assumes that the input string is valid UTF-8.

   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param latin1_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf16le_to_latin1(const char16_t * input, size_t length, char* latin1_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16BE string into Latin1 string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param latin1_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf16be_to_latin1(const char16_t * input, size_t length, char* latin1_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into UTF-8 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16LE string
   */
  simdutf_warn_unused virtual size_t convert_utf16le_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into UTF-8 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16BE string
   */
  simdutf_warn_unused virtual size_t convert_utf16be_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into UTF-8 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  simdutf_warn_unused virtual result convert_utf16le_to_utf8_with_errors(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into UTF-8 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  simdutf_warn_unused virtual result convert_utf16be_to_utf8_with_errors(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16LE string into UTF-8 string.
   *
   * This function assumes that the input string is valid UTF-16LE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf16le_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16BE string into UTF-8 string.
   *
   * This function assumes that the input string is valid UTF-16BE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf16be_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into UTF-32 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16LE string
   */
  simdutf_warn_unused virtual size_t convert_utf16le_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into UTF-32 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16BE string
   */
  simdutf_warn_unused virtual size_t convert_utf16be_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into UTF-32 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
   */
  simdutf_warn_unused virtual result convert_utf16le_to_utf32_with_errors(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into UTF-32 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
   */
  simdutf_warn_unused virtual result convert_utf16be_to_utf32_with_errors(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16LE string into UTF-32 string.
   *
   * This function assumes that the input string is valid UTF-16LE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf16le_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16LE string into UTF-32BE string.
   *
   * This function assumes that the input string is valid UTF-16BE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf16be_to_utf32(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Compute the number of bytes that this UTF-16LE string would require in UTF-8 format.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16LE string as UTF-8
   */
  simdutf_warn_unused virtual size_t utf8_length_from_utf16le(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Compute the number of bytes that this UTF-16BE string would require in UTF-8 format.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16BE string as UTF-8
   */
  simdutf_warn_unused virtual size_t utf8_length_from_utf16be(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into Latin1 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param latin1_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-32 string
   */

  simdutf_warn_unused virtual size_t convert_utf32_to_latin1(const char32_t * input, size_t length, char* latin1_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into Latin1 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param latin1_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */

  simdutf_warn_unused virtual result convert_utf32_to_latin1_with_errors(const char32_t * input, size_t length, char* latin1_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-32 string into Latin1 string.
   *
   * This function assumes that the input string is valid UTF-32.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param latin1_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf32_to_latin1(const char32_t * input, size_t length, char* latin1_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-8 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-32 string
   */
  simdutf_warn_unused virtual size_t convert_utf32_to_utf8(const char32_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-8 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  simdutf_warn_unused virtual result convert_utf32_to_utf8_with_errors(const char32_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-32 string into UTF-8 string.
   *
   * This function assumes that the input string is valid UTF-32.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf8_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf32_to_utf8(const char32_t * input, size_t length, char* utf8_buffer) const noexcept = 0;


    /**
   * Return the number of bytes that this UTF-16 string would require in Latin1 format.
   *
   *
   * @param input         the UTF-16 string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16 string as Latin1
   */
    simdutf_warn_unused virtual size_t utf16_length_from_latin1(const char * input, size_t length) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-16LE string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-32 string
   */
  simdutf_warn_unused virtual size_t convert_utf32_to_utf16le(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-16BE string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-32 string
   */
  simdutf_warn_unused virtual size_t convert_utf32_to_utf16be(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-16LE string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
   */
  simdutf_warn_unused virtual result convert_utf32_to_utf16le_with_errors(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-16BE string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
   */
  simdutf_warn_unused virtual result convert_utf32_to_utf16be_with_errors(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-32 string into UTF-16LE string.
   *
   * This function assumes that the input string is valid UTF-32.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf32_to_utf16le(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-32 string into UTF-16BE string.
   *
   * This function assumes that the input string is valid UTF-32.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf32_to_utf16be(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Change the endianness of the input. Can be used to go from UTF-16LE to UTF-16BE or
   * from UTF-16BE to UTF-16LE.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16 string to process
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param output        the pointer to buffer that can hold the conversion result
   */
  virtual void change_endianness_utf16(const char16_t * input, size_t length, char16_t * output) const noexcept = 0;

 /**
   * Return the number of bytes that this Latin1 string would require in UTF-8 format.
   *   
   *   This function does not validate the input.
   * 
   * @param input         the Latin1 string to convert
   * @param length        the length of the string bytes
   * @return the number of bytes required to encode the Latin1 string as UTF-8
   */
    simdutf_warn_unused virtual size_t utf8_length_from_latin1(const char * input, size_t length) const noexcept = 0;

  /**
   * Compute the number of bytes that this UTF-32 string would require in UTF-8 format.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @return the number of bytes required to encode the UTF-32 string as UTF-8
   */
  simdutf_warn_unused virtual size_t utf8_length_from_utf32(const char32_t * input, size_t length) const noexcept = 0;


 

  /**
   * Compute the number of bytes that this UTF-32 string would require in Latin1 format.
   *
   * This function does not validate the input.
   * 
   * This function is not BOM-aware.
   *    
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @return the number of bytes required to encode the UTF-32 string as Latin1
   */
    simdutf_warn_unused virtual size_t latin1_length_from_utf32(const char32_t * input, size_t length) const noexcept = 0;

  /**
   * Compute the number of bytes that this UTF-8 string would require in Latin1 format.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   * 
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in byte
   * @return the number of bytes required to encode the UTF-8 string as Latin1
   */
    simdutf_warn_unused virtual size_t latin1_length_from_utf8(const char * input, size_t length) const noexcept = 0;

/*
   * Compute the number of bytes that this UTF-16LE/BE string would require in Latin1 format.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16LE string as Latin1
   */
  simdutf_warn_unused virtual size_t latin1_length_from_utf16(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Compute the number of two-byte words that this UTF-32 string would require in UTF-16 format.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @return the number of bytes required to encode the UTF-32 string as UTF-16
   */
  simdutf_warn_unused virtual size_t utf16_length_from_utf32(const char32_t * input, size_t length) const noexcept = 0;


    /**
   * Return the number of bytes that this UTF-32 string would require in Latin1 format.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @return the number of bytes required to encode the UTF-32 string as Latin1
   */
    simdutf_warn_unused virtual size_t utf32_length_from_latin1(const char * input, size_t length) const noexcept = 0;
  
  /*
   * Compute the number of bytes that this UTF-16LE string would require in UTF-32 format.
   *
   * This function is equivalent to count_utf16le.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16LE string as UTF-32
   */
  simdutf_warn_unused virtual size_t utf32_length_from_utf16le(const char16_t * input, size_t length) const noexcept = 0;

  /*
   * Compute the number of bytes that this UTF-16BE string would require in UTF-32 format.
   *
   * This function is equivalent to count_utf16be.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16BE string as UTF-32
   */
  simdutf_warn_unused virtual size_t utf32_length_from_utf16be(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-16LE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to process
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return number of code points
   */
  simdutf_warn_unused virtual size_t count_utf16le(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-16BE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to process
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return number of code points
   */
  simdutf_warn_unused virtual size_t count_utf16be(const char16_t * input, size_t length) const noexcept = 0;


  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to process
   * @param length        the length of the string in bytes
   * @return number of code points
   */
  simdutf_warn_unused virtual size_t count_utf8(const char * input, size_t length) const noexcept = 0;



protected:
  /** @private Construct an implementation with the given name and description. For subclasses. */
  simdutf_really_inline implementation(
    std::string name,
    std::string description,
    uint32_t required_instruction_sets
  ) :
    _name(name),
    _description(description),
    _required_instruction_sets(required_instruction_sets)
  {
  }
  virtual ~implementation()=default;

private:
  /**
   * The name of this implementation.
   */
  const std::string _name;

  /**
   * The description of this implementation.
   */
  const std::string _description;

  /**
   * Instruction sets required for this implementation.
   */
  const uint32_t _required_instruction_sets;
};

/** @private */
namespace internal {

/**
 * The list of available implementations compiled into simdutf.
 */
class available_implementation_list {
public:
  /** Get the list of available implementations compiled into simdutf */
  simdutf_really_inline available_implementation_list() {}
  /** Number of implementations */
  size_t size() const noexcept;
  /** STL const begin() iterator */
  const implementation * const *begin() const noexcept;
  /** STL const end() iterator */
  const implementation * const *end() const noexcept;

  /**
   * Get the implementation with the given name.
   *
   * Case sensitive.
   *
   *     const implementation *impl = simdutf::available_implementations["westmere"];
   *     if (!impl) { exit(1); }
   *     if (!imp->supported_by_runtime_system()) { exit(1); }
   *     simdutf::active_implementation = impl;
   *
   * @param name the implementation to find, e.g. "westmere", "haswell", "arm64"
   * @return the implementation, or nullptr if the parse failed.
   */
  const implementation * operator[](const std::string &name) const noexcept {
    for (const implementation * impl : *this) {
      if (impl->name() == name) { return impl; }
    }
    return nullptr;
  }

  /**
   * Detect the most advanced implementation supported by the current host.
   *
   * This is used to initialize the implementation on startup.
   *
   *     const implementation *impl = simdutf::available_implementation::detect_best_supported();
   *     simdutf::active_implementation = impl;
   *
   * @return the most advanced supported implementation for the current host, or an
   *         implementation that returns UNSUPPORTED_ARCHITECTURE if there is no supported
   *         implementation. Will never return nullptr.
   */
  const implementation *detect_best_supported() const noexcept;
};

template<typename T>
class atomic_ptr {
public:
  atomic_ptr(T *_ptr) : ptr{_ptr} {}

#if defined(SIMDUTF_NO_THREADS)
  operator const T*() const { return ptr; }
  const T& operator*() const { return *ptr; }
  const T* operator->() const { return ptr; }

  operator T*() { return ptr; }
  T& operator*() { return *ptr; }
  T* operator->() { return ptr; }
  atomic_ptr& operator=(T *_ptr) { ptr = _ptr; return *this; }

#else
  operator const T*() const { return ptr.load(); }
  const T& operator*() const { return *ptr; }
  const T* operator->() const { return ptr.load(); }

  operator T*() { return ptr.load(); }
  T& operator*() { return *ptr; }
  T* operator->() { return ptr.load(); }
  atomic_ptr& operator=(T *_ptr) { ptr = _ptr; return *this; }

#endif

private:
#if defined(SIMDUTF_NO_THREADS)
  T* ptr;
#else
  std::atomic<T*> ptr;
#endif
};

class detect_best_supported_implementation_on_first_use;

} // namespace internal

/**
 * The list of available implementations compiled into simdutf.
 */
extern SIMDUTF_DLLIMPORTEXPORT const internal::available_implementation_list& get_available_implementations();

/**
  * The active implementation.
  *
  * Automatically initialized on first use to the most advanced implementation supported by this hardware.
  */
extern SIMDUTF_DLLIMPORTEXPORT internal::atomic_ptr<const implementation>& get_active_implementation();


} // namespace simdutf

#endif // SIMDUTF_IMPLEMENTATION_H
