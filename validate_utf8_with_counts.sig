#if SIMDUTF_FEATURE_UTF8
/**
 * Finds the pointer to the first byte of invalid utf8 while counting continuation bytes and four-byte sequences.

 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return a utf8_result struct. It contains the length of the valid utf8 segment, the error code and the count of continuation bytes and four-byte sequences.
 *
 * Returns a utf8_result
 */
simdutf_warn_unused utf8_result validate_utf8_with_counts(
    const char *buf, size_t len) noexcept;
#endif
