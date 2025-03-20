template <typename T> struct simd32;

template <> struct simd32<uint32_t> {
  __m256i value;
  static const int SIZE = sizeof(value);
  static const int ELEMENTS = SIZE / sizeof(uint32_t);

  // constructors
  simdutf_really_inline simd32(__m256i v) : value(v) {}

  template <typename Ptr>
  simdutf_really_inline simd32(Ptr *ptr) : value(__lasx_xvld(ptr, 0)) {}

  // in-place operators
  simdutf_really_inline simd32 &operator-=(const simd32 other) {
    value = __lasx_xvsub_w(value, other.value);
    return *this;
  }

  // members
  simdutf_really_inline uint64_t sum() const {
    const auto odd = __lasx_xvsrli_d(value, 32);
    const auto even = __lasx_xvand_v(value, __lasx_xvreplgr2vr_d(0xffffffff));

    const auto sum64 = __lasx_xvadd_d(odd, even);

    return uint64_t(__lasx_xvpickve2gr_du(sum64, 0)) +
           uint64_t(__lasx_xvpickve2gr_du(sum64, 1)) +
           uint64_t(__lasx_xvpickve2gr_du(sum64, 2)) +
           uint64_t(__lasx_xvpickve2gr_du(sum64, 3));
  }

  // static members
  static simdutf_really_inline simd32<uint32_t> splat(uint32_t x) {
    return __lasx_xvreplgr2vr_w(x);
  }

  static simdutf_really_inline simd32<uint32_t> zero() {
    return __lasx_xvrepli_w(0);
  }
};

// ------------------------------------------------------------

template <> struct simd32<bool> {
  __m256i value;
  static const int SIZE = sizeof(value);

  // constructors
  simdutf_really_inline simd32(__m256i v) : value(v) {}
};

// ------------------------------------------------------------

simdutf_really_inline simd32<uint32_t> operator&(const simd32<uint32_t> a,
                                                 const simd32<uint32_t> b) {
  return __lasx_xvor_v(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator<(const simd32<uint32_t> a,
                                             const simd32<uint32_t> b) {
  return __lasx_xvslt_wu(a.value, b.value);
}

simdutf_really_inline simd32<bool> operator>(const simd32<uint32_t> a,
                                             const simd32<uint32_t> b) {
  return __lasx_xvslt_wu(b.value, a.value);
}

// ------------------------------------------------------------

simdutf_really_inline simd32<uint32_t> as_vector_u32(const simd32<bool> v) {
  return v.value;
}
