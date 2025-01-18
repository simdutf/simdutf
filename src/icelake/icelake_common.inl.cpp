// file included directly
/**
 * Store the last N bytes of previous followed by 512-N bytes from input.
 */
template <int N> __m512i prev(__m512i input, __m512i previous) {
  static_assert(N <= 32, "N must be no larger than 32");
  const __m512i movemask =
      _mm512_setr_epi32(28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
  const __m512i rotated = _mm512_permutex2var_epi32(input, movemask, previous);
#if SIMDUTF_GCC8 || SIMDUTF_GCC9
  constexpr int shift = 16 - N; // workaround for GCC8,9
  return _mm512_alignr_epi8(input, rotated, shift);
#else
  return _mm512_alignr_epi8(input, rotated, 16 - N);
#endif // SIMDUTF_GCC8 || SIMDUTF_GCC9
}

template <unsigned idx0, unsigned idx1, unsigned idx2, unsigned idx3>
__m512i shuffle_epi128(__m512i v) {
  static_assert((idx0 >= 0 && idx0 <= 3), "idx0 must be in range 0..3");
  static_assert((idx1 >= 0 && idx1 <= 3), "idx1 must be in range 0..3");
  static_assert((idx2 >= 0 && idx2 <= 3), "idx2 must be in range 0..3");
  static_assert((idx3 >= 0 && idx3 <= 3), "idx3 must be in range 0..3");

  constexpr unsigned shuffle = idx0 | (idx1 << 2) | (idx2 << 4) | (idx3 << 6);
  return _mm512_shuffle_i32x4(v, v, shuffle);
}

template <unsigned idx> constexpr __m512i broadcast_epi128(__m512i v) {
  return shuffle_epi128<idx, idx, idx, idx>(v);
}
