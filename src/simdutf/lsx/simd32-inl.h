template <typename T> struct simd32;

template <> struct simd32<uint32_t> {
  __m128i value;
  static const int SIZE = sizeof(value);
  static const int ELEMENTS = SIZE / sizeof(uint32_t);

  // constructors
  simdutf_really_inline simd32(__m128i v) : value(v) {}

  template <typename Ptr>
  simdutf_really_inline simd32(Ptr *ptr) : value(__lsx_vld(ptr, 0)) {}

  // in-place operators
  simdutf_really_inline simd32 &operator-=(const simd32 other) {
    value = __lsx_vsub_w(value, other.value);
    return *this;
  }

  // members
  simdutf_really_inline uint64_t sum() const {
    return uint64_t(__lsx_vpickve2gr_wu(value, 0)) +
           uint64_t(__lsx_vpickve2gr_wu(value, 1)) +
           uint64_t(__lsx_vpickve2gr_wu(value, 2)) +
           uint64_t(__lsx_vpickve2gr_wu(value, 3));
  }

  // static members
  static simdutf_really_inline simd32<uint32_t> splat(uint32_t x) {
    return __lsx_vreplgr2vr_w(x);
  }

  static simdutf_really_inline simd32<uint32_t> zero() {
    return __lsx_vrepli_w(0);
  }
};

// ------------------------------------------------------------

template <> struct simd32<bool> {
  __m128i value;
  static const int SIZE = sizeof(value);

  // constructors
  simdutf_really_inline simd32(__m128i v) : value(v) {}
};

// ------------------------------------------------------------

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> a,
                                                 const simd32<uint32_t> b) {
  return __lsx_vor_v(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator<(const simd32<uint32_t> a,
                                             const simd32<uint32_t> b) {
  return __lsx_vslt_wu(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator>(const simd32<uint32_t> a,
                                             const simd32<uint32_t> b) {
  return __lsx_vslt_wu(b.value, a.value);
}

// ------------------------------------------------------------

simdutf_really_inline simd32<uint32_t> as_vector_u32(const simd32<bool> v) {
  return v.value;
}
