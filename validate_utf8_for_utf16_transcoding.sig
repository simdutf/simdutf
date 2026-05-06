#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
/**
 * Finds the pointer to the first byte of invalid utf8.
 * Additionally it determines how much space the text up to that point requires when transcoded to utf16.

 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return a result pair struct (of type simdutf::result containing the two fields error and count). We use with an error code and either position of the error (in the input in code units) if any, or the number of code units validated if successful.
 *
 * Returns a result
 */
simdutf_warn_unused full_result validate_utf8_for_utf16_transcoding(
    const char *buf, size_t len) noexcept;
#endif
