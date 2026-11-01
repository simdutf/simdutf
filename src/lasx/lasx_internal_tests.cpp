#include <cstdio>
#include <cstdlib>

namespace simd8tests {
// Byte i holds i, so comparing with a splat of i selects byte i alone.
const uint8_t byte_index[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

// Reports on stderr: stdout is still buffered when abort() runs under ctest.
void check_any(const simdutf::lasx::simd::simd8<bool> mask, bool want,
               const char *what, size_t i) {
  const bool got = mask.any();
  if (got != want) {
    fprintf(stderr, "simd8<bool>::any() = %d, want %d for %s (byte %zu)\n", got,
            want, what, i);
    abort();
  }
}
} // namespace simd8tests

simdutf_maybe_unused static void
simd8_bool_any(const simdutf::implementation &) {
  using simd8tests::byte_index;
  using simd8tests::check_any;
  using simdutf::lasx::simd::simd8;
  static_assert(sizeof(simd8<uint8_t>) == sizeof(byte_index),
                "one index per byte");

  check_any(simd8<bool>(false), false, "no byte set", 0);
  check_any(simd8<bool>(true), true, "every byte set", 0);
  const simd8<uint8_t> index(byte_index);
  for (size_t i = 0; i < sizeof(byte_index); i++) {
    const simd8<bool> only_i = index == simd8<uint8_t>(uint8_t(i));
    check_any(only_i, true, "only one byte set", i);
    check_any(~only_i, true, "all but one byte set", i);
  }
}
