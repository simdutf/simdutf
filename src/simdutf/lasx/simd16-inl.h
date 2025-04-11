template <typename T> struct simd16;

template <typename T, typename Mask = simd16<bool>>
struct base16 : base<simd16<T>> {
  using bitmask_type = uint32_t;

  simdutf_really_inline base16() : base<simd16<T>>() {}
  simdutf_really_inline base16(const __m256i _value)
      : base<simd16<T>>(_value) {}
  template <typename Pointer>
  simdutf_really_inline base16(const Pointer *ptr)
      : base16(__lasx_xvld(reinterpret_cast<const __m256i *>(ptr), 0)) {}

  /// the size of vector in bytes
  static const int SIZE = sizeof(base<simd16<T>>::value);

  /// the number of elements of type T a vector can hold
  static const int ELEMENTS = SIZE / sizeof(T);
};

// SIMD byte mask type (returned by things like eq and gt)
template <> struct simd16<bool> : base16<bool> {
  static simdutf_really_inline simd16<bool> splat(bool _value) {
    return __lasx_xvreplgr2vr_h(uint8_t(-(!!_value)));
  }

  simdutf_really_inline simd16() : base16() {}
  simdutf_really_inline simd16(const __m256i _value) : base16<bool>(_value) {}
  // Splat constructor
  simdutf_really_inline simd16(bool _value) : base16<bool>(splat(_value)) {}

  simdutf_really_inline bitmask_type to_bitmask() const {
    __m256i mask = __lasx_xvmsknz_b(this->value);
    bitmask_type mask0 = __lasx_xvpickve2gr_wu(mask, 0);
    bitmask_type mask1 = __lasx_xvpickve2gr_wu(mask, 4);
    return (mask0 | (mask1 << 16));
  }
  simdutf_really_inline simd16<bool> operator~() const { return *this ^ true; }
};

template <typename T> struct base16_numeric : base16<T> {
  static simdutf_really_inline simd16<T> splat(T _value) {
    return __lasx_xvreplgr2vr_h((uint16_t)_value);
  }
  static simdutf_really_inline simd16<T> zero() { return __lasx_xvldi(0); }
  static simdutf_really_inline simd16<T> load(const T values[8]) {
    return __lasx_xvld(reinterpret_cast<const __m256i *>(values), 0);
  }

  simdutf_really_inline base16_numeric() : base16<T>() {}
  simdutf_really_inline base16_numeric(const __m256i _value)
      : base16<T>(_value) {}

  // Store to array
  simdutf_really_inline void store(T dst[8]) const {
    return __lasx_xvst(this->value, reinterpret_cast<__m256i *>(dst), 0);
  }

  // Override to distinguish from bool version
  simdutf_really_inline simd16<T> operator~() const { return *this ^ 0xFFFFu; }
};

// Unsigned code units
template <> struct simd16<uint16_t> : base16_numeric<uint16_t> {
  simdutf_really_inline simd16() : base16_numeric<uint16_t>() {}
  simdutf_really_inline simd16(const __m256i _value)
      : base16_numeric<uint16_t>(_value) {}

  // Splat constructor
  simdutf_really_inline simd16(uint16_t _value) : simd16(splat(_value)) {}

  // Array constructor
  simdutf_really_inline simd16(const uint16_t *values) : simd16(load(values)) {}
  simdutf_really_inline simd16(const char16_t *values)
      : simd16(load(reinterpret_cast<const uint16_t *>(values))) {}

  // Order-specific operations
  simdutf_really_inline simd16<bool>
  operator<=(const simd16<uint16_t> other) const {
    return __lasx_xvsle_hu(this->value, other.value);
  }
  simdutf_really_inline simd16<bool>
  operator>=(const simd16<uint16_t> other) const {
    return __lasx_xvsle_hu(other.value, this->value);
  }

  // Change the endianness
  simdutf_really_inline simd16<uint16_t> swap_bytes() const {
    return __lasx_xvshuf4i_b(this->value, 0b10110001);
  }

  template <unsigned N>
  static simdutf_really_inline simd8<uint8_t>
  pack_shifted_right(const simd16<uint16_t> &v0, const simd16<uint16_t> &v1) {
    return __lasx_xvpermi_d(__lasx_xvssrlni_bu_h(v1.value, v0.value, N),
                            0b11011000);
  }

  // Pack with the unsigned saturation of two uint16_t code units into single
  // uint8_t vector
  static simdutf_really_inline simd8<uint8_t> pack(const simd16<uint16_t> &v0,
                                                   const simd16<uint16_t> &v1) {

    return pack_shifted_right<0>(v0, v1);
  }
};

template <typename T> struct simd16x32 {
  static constexpr int NUM_CHUNKS = 64 / sizeof(simd16<T>);
  static_assert(NUM_CHUNKS == 2,
                "LASX kernel should use two registers per 64-byte block.");
  simd16<T> chunks[NUM_CHUNKS];

  simd16x32(const simd16x32<T> &o) = delete; // no copy allowed
  simd16x32<T> &
  operator=(const simd16<T> other) = delete; // no assignment allowed
  simd16x32() = delete;                      // no default constructor allowed

  simdutf_really_inline simd16x32(const simd16<T> chunk0,
                                  const simd16<T> chunk1)
      : chunks{chunk0, chunk1} {}
  simdutf_really_inline simd16x32(const T *ptr)
      : chunks{simd16<T>::load(ptr),
               simd16<T>::load(ptr + sizeof(simd16<T>) / sizeof(T))} {}

  simdutf_really_inline void store(T *ptr) const {
    this->chunks[0].store(ptr + sizeof(simd16<T>) * 0 / sizeof(T));
    this->chunks[1].store(ptr + sizeof(simd16<T>) * 1 / sizeof(T));
  }

  simdutf_really_inline uint64_t to_bitmask() const {
    uint64_t r_lo = uint32_t(this->chunks[0].to_bitmask());
    uint64_t r_hi = this->chunks[1].to_bitmask();
    return r_lo | (r_hi << 32);
  }

  simdutf_really_inline void store_ascii_as_utf16(char16_t *ptr) const {
    this->chunks[0].store_ascii_as_utf16(ptr + sizeof(simd16<T>) * 0);
    this->chunks[1].store_ascii_as_utf16(ptr + sizeof(simd16<T>));
  }

  simdutf_really_inline void swap_bytes() {
    this->chunks[0] = this->chunks[0].swap_bytes();
    this->chunks[1] = this->chunks[1].swap_bytes();
  }

  simdutf_really_inline uint64_t lteq(const T m) const {
    const simd16<T> mask = simd16<T>::splat(m);
    return simd16x32<bool>(this->chunks[0] <= mask, this->chunks[1] <= mask)
        .to_bitmask();
  }

  simdutf_really_inline uint64_t not_in_range(const T low, const T high) const {
    const simd16<T> mask_low = simd16<T>::splat(static_cast<T>(low - 1));
    const simd16<T> mask_high = simd16<T>::splat(static_cast<T>(high + 1));
    return simd16x32<bool>(
               (this->chunks[0] >= mask_high) | (this->chunks[0] <= mask_low),
               (this->chunks[1] >= mask_high) | (this->chunks[1] <= mask_low))
        .to_bitmask();
  }
}; // struct simd16x32<T>
