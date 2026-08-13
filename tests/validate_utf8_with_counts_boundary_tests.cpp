// Deterministic boundary coverage for validate_utf8_with_counts.
//
// The brute-force test corrupts random positions, so it only exercises the
// 64-byte block boundaries by chance. The delicate part of this function is the
// rewind path: when an error is detected in a block, the counts accumulated for
// that block have to be discarded and the scalar code has to backtrack,
// possibly into the previous block, to find the leading byte. The tests below
// walk an error across every offset, validate every truncation, and check every
// buffer length, so that every phase relative to a block boundary is covered.

#include "simdutf.h"
#include "simdutf/error.h"

#include <tests/helpers/random_utf8.h>
#include <tests/helpers/test.h>

#include <cstdint>
#include <vector>

namespace {

// Compare the active implementation against the scalar reference.
//
// We deliberately do not compare non_ascii_count: it is not needed to derive
// either the UTF-16 length (input - continuations + four_byte) or the number of
// code points (input - continuations).
template <typename T>
void compare_with_scalar(T &implementation, const uint8_t *data, size_t len) {
  const simdutf::utf8_result res =
      implementation.validate_utf8_with_counts((const char *)data, len);
  const simdutf::utf8_result ref =
      simdutf::scalar::utf8::validate_with_counts((const char *)data, len);

  ASSERT_EQUAL(res.error, ref.error);
  ASSERT_EQUAL(res.input_count, ref.input_count);
  ASSERT_EQUAL(res.continuation_count, ref.continuation_count);
  ASSERT_EQUAL(res.four_byte_count, ref.four_byte_count);

  // Cross-check against the plain validation entry point and against the
  // independent UTF-16 length routine.
  const simdutf::result errors =
      implementation.validate_utf8_with_errors((const char *)data, len);
  ASSERT_EQUAL(res.error, errors.error);
  ASSERT_EQUAL(res.input_count, errors.count);
  ASSERT_EQUAL(res.utf16_length(), implementation.utf16_length_from_utf8(
                                       (const char *)data, errors.count));
}

// A buffer spanning several 64-byte blocks, mixing all four sequence lengths so
// that multibyte sequences straddle block boundaries.
std::vector<uint8_t> sample_utf8(size_t bytes) {
  simdutf::tests::helpers::random_utf8 gen(1234, 1, 1, 1, 1);
  return gen.generate(bytes);
}

// Build a valid UTF-8 buffer of exactly `len` bytes. We repeat a 10-byte
// pattern covering 1-, 2-, 3- and 4-byte sequences and pad the remainder with
// ASCII, so that the phase of the multibyte sequences relative to the 64-byte
// blocks shifts as `len` grows.
std::vector<uint8_t> exact_length_utf8(size_t len) {
  // 'a' | U+00E9 | U+65E5 | U+1F600  == 1 + 2 + 3 + 4 == 10 bytes
  static const uint8_t pattern[10] = {'a',  0xc3, 0xa9, 0xe6, 0x97,
                                      0xa5, 0xf0, 0x9f, 0x98, 0x80};
  std::vector<uint8_t> buf;
  buf.reserve(len + 1); // keep data() non-null even when len == 0
  while (buf.size() + sizeof(pattern) <= len) {
    buf.insert(buf.end(), pattern, pattern + sizeof(pattern));
  }
  while (buf.size() < len) {
    buf.push_back('x'); // ASCII padding keeps the buffer valid at any length
  }
  return buf;
}

} // namespace

// Walk an invalid byte across every offset of a multi-block buffer.
TEST(counts_error_at_every_offset) {
  // 0xff is never valid; 0x80 is a stray continuation byte; 0xc0 is an invalid
  // lead; 0xf5 is a lead beyond U+10FFFF.
  const uint8_t bad_bytes[] = {0xff, 0x80, 0xc0, 0xf5};
  std::vector<uint8_t> utf8 = sample_utf8(400);
  ASSERT_TRUE(
      implementation.validate_utf8((const char *)utf8.data(), utf8.size()));
  for (size_t offset = 0; offset < utf8.size(); offset++) {
    const uint8_t restore = utf8[offset];
    for (const uint8_t bad : bad_bytes) {
      utf8[offset] = bad;
      compare_with_scalar(implementation, utf8.data(), utf8.size());
    }
    utf8[offset] = restore;
  }
}

// Every truncation: exercises the tail block and incomplete sequences at EOF.
TEST(counts_every_truncation) {
  std::vector<uint8_t> utf8 = sample_utf8(400);
  ASSERT_TRUE(
      implementation.validate_utf8((const char *)utf8.data(), utf8.size()));
  for (size_t len = 0; len <= utf8.size(); len++) {
    compare_with_scalar(implementation, utf8.data(), len);
  }
}

// Valid inputs of every length, across several block boundaries.
TEST(counts_valid_all_lengths) {
  for (size_t len = 0; len <= 300; len++) {
    const std::vector<uint8_t> utf8 = exact_length_utf8(len);
    ASSERT_EQUAL(utf8.size(), len);
    ASSERT_TRUE(
        implementation.validate_utf8((const char *)utf8.data(), utf8.size()));
    compare_with_scalar(implementation, utf8.data(), utf8.size());
  }
}

TEST_MAIN
