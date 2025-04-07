template <typename T> struct simd64;

template <> struct simd64<uint64_t> {
  __m256i value;
  static const int SIZE = sizeof(value);
  static const int ELEMENTS = SIZE / sizeof(uint64_t);

  // constructors
  simdutf_really_inline simd64(__m256i v) : value(v) {}

  template <typename Ptr>
  simdutf_really_inline simd64(Ptr *ptr) : value(__lasx_xvld(ptr, 0)) {}

  // in-place operators
  simdutf_really_inline simd64 &operator+=(const simd64 other) {
    value = __lasx_xvadd_d(value, other.value);
    return *this;
  }

  // members
  simdutf_really_inline uint64_t sum() const {
    return uint64_t(__lasx_xvpickve2gr_du(value, 0)) +
           uint64_t(__lasx_xvpickve2gr_du(value, 1)) +
           uint64_t(__lasx_xvpickve2gr_du(value, 2)) +
           uint64_t(__lasx_xvpickve2gr_du(value, 3));
  }

  // static members
  static simdutf_really_inline simd64<uint64_t> zero() {
    return __lasx_xvrepli_d(0);
  }
};

// ------------------------------------------------------------

template <> struct simd64<bool> {
  __m256i value;
  static const int SIZE = sizeof(value);

  // constructors
  simdutf_really_inline simd64(__m256i v) : value(v) {}
};

// ------------------------------------------------------------

simd64<uint64_t> sum_8bytes(const simd8<uint8_t> v) {
  const auto sum_u16 = __lasx_xvhaddw_hu_bu(v, v);
  const auto sum_u32 = __lasx_xvhaddw_wu_hu(sum_u16, sum_u16);
  const auto sum_u64 = __lasx_xvhaddw_du_wu(sum_u32, sum_u32);

  return simd64<uint64_t>(sum_u64);
}
