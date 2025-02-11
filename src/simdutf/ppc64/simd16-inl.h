// file included directly

template <typename T> struct simd16;

template <typename T> struct base16 {
  using vector_type = vector_u16_type_for_element<T>;
  static const int SIZE = sizeof(vector_type);
  static const int ELEMENTS = sizeof(vector_type) / sizeof(T);

  vector_type value;

  // Zero constructor
  simdutf_really_inline base16() : value{vector_type()} {}

  // Conversion from SIMD register
  simdutf_really_inline base16(const vector_type _value) : value{_value} {}

  // Splat for scalar
  simdutf_really_inline base16(T scalar) : value{vec_splats(scalar)} {}

  template <typename Pointer>
  simdutf_really_inline base16(const Pointer *ptr) : base16(vec_xl(0, ptr)) {}

  // Conversion to SIMD register
  simdutf_really_inline operator const vector_type() const {
    return this->value;
  }

  void dump() const {
    uint16_t tmp[ELEMENTS];
    vec_xst(value, 0, reinterpret_cast<vector_type *>(tmp));
    for (int i = 0; i < ELEMENTS; i++) {
      if (i == 0) {
        printf("[%04x", tmp[i]);
      } else if (i == ELEMENTS - 1) {
        printf(" %04x]", tmp[i]);
      } else {
        printf(" %04x", tmp[i]);
      }
    }
    putchar('\n');
  }
};

// Forward declaration
template <typename> struct simd16;

template <typename T>
simd16<bool> operator==(const simd16<T> a, const simd16<T> b);

template <typename T, typename U>
simd16<bool> operator==(const simd16<T> a, U b);

template <typename T> simd16<T> operator&(const simd16<T> a, const simd16<T> b);

template <typename T> simd16<T> operator|(const simd16<T> a, const simd16<T> b);

template <typename T> simd16<T> operator^(const simd16<T> a, const simd16<T> b);

// SIMD byte mask type (returned by things like eq and gt)
template <> struct simd16<bool> : base16<bool> {
  static simdutf_really_inline simd16<bool> splat(bool _value) {
    return (vector_type)vec_splats(uint16_t(-(!!_value)));
  }

  simdutf_really_inline simd16() : base16() {}
  simdutf_really_inline simd16(const vector_type _value)
      : base16<bool>(_value) {}

  // Splat constructor
  simdutf_really_inline simd16(bool _value) : base16<bool>(splat(_value)) {}

  simdutf_really_inline uint16_t to_bitmask() const {
    const vec_u8_t perm_mask = {15 * 8 + 7, 14 * 8 + 7, 13 * 8 + 7, 12 * 8 + 7,
                                11 * 8 + 7, 10 * 8 + 7, 9 * 8 + 7,  8 * 8 + 7,
                                7 * 8 + 7,  6 * 8 + 7,  5 * 8 + 7,  4 * 8 + 7,
                                3 * 8 + 7,  2 * 8 + 7,  1 * 8 + 7,  0 * 8 + 7};

    const vec_u64_t result =
        (vec_u64_t)vec_vbpermq((vec_u8_t)this->value, perm_mask);
#ifdef __LITTLE_ENDIAN__
    return static_cast<uint16_t>(result[1]);
#else
    return static_cast<uint16_t>(result[0]);
#endif
  }
  simdutf_really_inline bool any() const {
    const vec_u64_t tmp = (vec_u64_t)value;

    return tmp[0] || tmp[1]; // Note: logical or, not binary one
  }

  simdutf_really_inline simd16<bool> operator~() const {
    return (vec_bool16_t)vec_xor(this->value, vec_splats(uint16_t(0xffff)));
  }
};

template <typename T> struct base16_numeric : base16<T> {
  using super = base16<T>;
  using vector_type = typename super::vector_type;

  static simdutf_really_inline simd16<T> splat(T _value) {
    return vec_splats(_value);
  }
  static simdutf_really_inline simd16<T> zero() { return splat(0); }
  static simdutf_really_inline simd16<T> load(const T values[8]) {
    return vec_xl(0, reinterpret_cast<const T *>(values));
  }

  simdutf_really_inline base16_numeric() : base16<T>() {}
  simdutf_really_inline base16_numeric(const vector_type _value)
      : base16<T>(_value) {}

  // Store to array
  simdutf_really_inline void store(T dst[8]) const {
    return vec_xst(this->value, 0, reinterpret_cast<vector_type *>(dst));
  }

  // Override to distinguish from bool version
  simdutf_really_inline simd16<T> operator~() const { return *this ^ 0xffff; }

  // Addition/subtraction are the same for signed and unsigned
  simdutf_really_inline simd16<T> operator+(const simd16<T> other) const {
    return _mm_add_epi16(*this, other);
  }
  simdutf_really_inline simd16<T> operator-(const simd16<T> other) const {
    return _mm_sub_epi16(*this, other);
  }
  simdutf_really_inline simd16<T> &operator+=(const simd16<T> other) {
    *this = *this + other;
    return *static_cast<simd16<T> *>(this);
  }
  simdutf_really_inline simd16<T> &operator-=(const simd16<T> other) {
    *this = *this - other;
    return *static_cast<simd16<T> *>(this);
  }
};

// Signed code units
template <> struct simd16<int16_t> : base16_numeric<int16_t> {
  simdutf_really_inline simd16() : base16_numeric<int16_t>() {}
  simdutf_really_inline simd16(const vector_type _value)
      : base16_numeric<int16_t>(_value) {}
  // Splat constructor
  simdutf_really_inline simd16(int16_t _value) : simd16(splat(_value)) {}
  // Array constructor
  simdutf_really_inline simd16(const int16_t *values) : simd16(load(values)) {}
  simdutf_really_inline simd16(const char16_t *values)
      : simd16(load(reinterpret_cast<const int16_t *>(values))) {}
  // Member-by-member initialization
  simdutf_really_inline simd16(int16_t v0, int16_t v1, int16_t v2, int16_t v3,
                               int16_t v4, int16_t v5, int16_t v6, int16_t v7)
      : simd16((vector_type){v0, v1, v2, v3, v4, v5, v6, v7}) {}
  simdutf_really_inline operator simd16<uint16_t>() const;

  // Order-sensitive comparisons
  simdutf_really_inline simd16<int16_t>
  max_val(const simd16<int16_t> other) const {
    return vec_max(this->value, other.value);
  }
  simdutf_really_inline simd16<int16_t>
  min_val(const simd16<int16_t> other) const {
    return vec_min(this->value, other.value);
  }
  simdutf_really_inline simd16<bool>
  operator>(const simd16<int16_t> other) const {
    return vec_cmpgt(this->value, other.value);
  }
  simdutf_really_inline simd16<bool>
  operator<(const simd16<int16_t> other) const {
    return vec_cmplt(this->value, other.value);
  }
};

// Unsigned code units
template <> struct simd16<uint16_t> : base16_numeric<uint16_t> {
  simdutf_really_inline simd16() : base16_numeric<uint16_t>() {}
  simdutf_really_inline simd16(const vector_type _value)
      : base16_numeric<uint16_t>(_value) {}

  // Splat constructor
  simdutf_really_inline simd16(uint16_t _value) : simd16(splat(_value)) {}
  // Array constructor
  simdutf_really_inline simd16(const uint16_t *values) : simd16(load(values)) {}
  simdutf_really_inline simd16(const char16_t *values)
      : simd16(load(reinterpret_cast<const uint16_t *>(values))) {}
  // Member-by-member initialization
  simdutf_really_inline simd16(uint16_t v0, uint16_t v1, uint16_t v2,
                               uint16_t v3, uint16_t v4, uint16_t v5,
                               uint16_t v6, uint16_t v7)
      : simd16((vector_type){v0, v1, v2, v3, v4, v5, v6, v7}) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  simdutf_really_inline static simd16<uint16_t>
  repeat_16(uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4,
            uint16_t v5, uint16_t v6, uint16_t v7) {
    return simd16<uint16_t>(v0, v1, v2, v3, v4, v5, v6, v7);
  }

  // Saturated math
  simdutf_really_inline simd16<uint16_t>
  saturating_add(const simd16<uint16_t> other) const {
    return vec_adds(this->value, other.value);
  }
  simdutf_really_inline simd16<uint16_t>
  saturating_sub(const simd16<uint16_t> other) const {
    return vec_subs(this->value, other.value);
  }

  // Order-specific operations
  simdutf_really_inline simd16<uint16_t>
  max_val(const simd16<uint16_t> other) const {
    return vec_max(this->value, other.value);
  }
  simdutf_really_inline simd16<uint16_t>
  min_val(const simd16<uint16_t> other) const {
    return vec_min(this->value, other.value);
  }
  // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
  simdutf_really_inline simd16<uint16_t>
  gt_bits(const simd16<uint16_t> other) const {
    return this->saturating_sub(other);
  }
  // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
  simdutf_really_inline simd16<uint16_t>
  lt_bits(const simd16<uint16_t> other) const {
    return other.saturating_sub(*this);
  }
  simdutf_really_inline simd16<bool>
  operator<=(const simd16<uint16_t> other) const {
    return other.max_val(*this) == other;
  }
  simdutf_really_inline simd16<bool>
  operator>=(const simd16<uint16_t> other) const {
    return other.min_val(*this) == other;
  }
  simdutf_really_inline simd16<bool>
  operator>(const simd16<uint16_t> other) const {
    return this->gt_bits(other).any_bits_set();
  }
  simdutf_really_inline simd16<bool>
  operator<(const simd16<uint16_t> other) const {
    return this->gt_bits(other).any_bits_set();
  }

  // Bit-specific operations
  simdutf_really_inline simd16<bool> bits_not_set() const { return *this == 0; }
  simdutf_really_inline simd16<bool> bits_not_set(simd16<uint16_t> bits) const {
    return (*this & bits).bits_not_set();
  }
  simdutf_really_inline simd16<bool> any_bits_set() const {
    return ~this->bits_not_set();
  }
  simdutf_really_inline simd16<bool> any_bits_set(simd16<uint16_t> bits) const {
    return ~this->bits_not_set(bits);
  }

  simdutf_really_inline bool bits_not_set_anywhere() const {
    const vec_u64_t tmp = vec_u64_t(this->value);

    return (tmp[0] | tmp[1]) == 0;
  }
  simdutf_really_inline bool any_bits_set_anywhere() const {
    return !bits_not_set_anywhere();
  }
  simdutf_really_inline bool
  bits_not_set_anywhere(simd16<uint16_t> bits) const {
    const vec_u64_t tmp = vec_u64_t(vec_and(this->value, bits.value));

    return (tmp[0] | tmp[1]) == 0;
  }
  simdutf_really_inline bool
  any_bits_set_anywhere(simd16<uint16_t> bits) const {
    return !bits_not_set_anywhere(bits);
  }
  template <int N> simdutf_really_inline simd16<uint16_t> shr() const {
    return vec_sr(value, vec_splats(uint16_t(N)));
  }
  template <int N> simdutf_really_inline simd16<uint16_t> shl() const {
    return vec_sl(value, vec_splats(uint16_t(N)));
  }
  // Get one of the bits and make a bitmask out of it.
  // e.g. value.get_bit<7>() gets the high bit
  template <int N> simdutf_really_inline int get_bit() const {
    if (N >= 0 and N <= 15) {
      const vec_u8_t perm_mask = {
          0 * 8 + N,  1 * 8 + N,  2 * 8 + N,  3 * 8 + N, 4 * 8 + N,  5 * 8 + N,
          6 * 8 + N,  7 * 8 + N,  8 * 8 + N,  9 * 8 + N, 10 * 8 + N, 11 * 8 + N,
          12 * 8 + N, 13 * 8 + N, 14 * 8 + N, 15 * 8 + N};

      const vec_u64_t result =
          (vec_u64_t)vec_vbpermq((vec_u8_t)this->value, perm_mask);
#ifdef __LITTLE_ENDIAN__
      return static_cast<int>(result[1]);
#else
      return static_cast<int>(result[0]);
#endif
    } else {
      return 0;
    }
  }

  // Change the endianness
  simdutf_really_inline simd16<uint16_t> swap_bytes() const {
    return vec_reve(value);
  }

  // Pack with the unsigned saturation of two uint16_t code units into single
  // uint8_t vector
  static simdutf_really_inline simd8<uint8_t> pack(const simd16<uint16_t> &v0,
                                                   const simd16<uint16_t> &v1) {
    return vec_packs(v0.value, v1.value);
  }

  // Change the endianness
  simdutf_really_inline simd16<uint16_t> swap_bytes() {
    return vec_reve(value);
  }
};

template <typename T>
simd16<bool> operator==(const simd16<T> a, const simd16<T> b) {
  return vec_cmpeq(a.value, b.value);
}

template <typename T>
simd16<T> operator&(const simd16<T> a, const simd16<T> b) {
  return vec_and(a.value, b.value);
}

template <typename T, typename U> simd16<T> operator&(const simd16<T> a, U b) {
  return vec_and(a.value, vec_splats(T(b)));
}

template <typename T>
simd16<T> operator|(const simd16<T> a, const simd16<T> b) {
  return vec_or(a.value, b.value);
}

template <typename T>
simd16<T> operator^(const simd16<T> a, const simd16<T> b) {
  return vec_xor(a.value, b.value);
}

template <typename T, typename U> simd16<T> operator^(const simd16<T> a, U b) {
  return vec_xor(a.value, vec_splats(T(b)));
}

simdutf_really_inline simd16<int16_t>::operator simd16<uint16_t>() const {
  return (vec_u16_t)(value);
}

template <typename T> struct simd16x32 {
  static constexpr int NUM_CHUNKS = 64 / sizeof(simd16<T>);
  static_assert(NUM_CHUNKS == 4,
                "AltiVec kernel should use four registers per 64-byte block.");
  simd16<T> chunks[NUM_CHUNKS];

  simd16x32(const simd16x32<T> &o) = delete; // no copy allowed
  simd16x32<T> &
  operator=(const simd16<T> other) = delete; // no assignment allowed
  simd16x32() = delete;                      // no default constructor allowed

  simdutf_really_inline
  simd16x32(const simd16<T> chunk0, const simd16<T> chunk1,
            const simd16<T> chunk2, const simd16<T> chunk3)
      : chunks{chunk0, chunk1, chunk2, chunk3} {}
  simdutf_really_inline simd16x32(const T *ptr)
      : chunks{simd16<T>::load(ptr),
               simd16<T>::load(ptr + sizeof(simd16<T>) / sizeof(T)),
               simd16<T>::load(ptr + 2 * sizeof(simd16<T>) / sizeof(T)),
               simd16<T>::load(ptr + 3 * sizeof(simd16<T>) / sizeof(T))} {}

  simdutf_really_inline void store(T *ptr) const {
    this->chunks[0].store(ptr + sizeof(simd16<T>) * 0 / sizeof(T));
    this->chunks[1].store(ptr + sizeof(simd16<T>) * 1 / sizeof(T));
    this->chunks[2].store(ptr + sizeof(simd16<T>) * 2 / sizeof(T));
    this->chunks[3].store(ptr + sizeof(simd16<T>) * 3 / sizeof(T));
  }

  simdutf_really_inline simd16<T> reduce_or() const {
    return (this->chunks[0] | this->chunks[1]) |
           (this->chunks[2] | this->chunks[3]);
  }

  simdutf_really_inline bool is_ascii() const {
    return this->reduce_or().is_ascii();
  }

  simdutf_really_inline void store_ascii_as_utf16(char16_t *ptr) const {
    this->chunks[0].store_ascii_as_utf16(ptr + sizeof(simd16<T>) * 0);
    this->chunks[1].store_ascii_as_utf16(ptr + sizeof(simd16<T>) * 1);
    this->chunks[2].store_ascii_as_utf16(ptr + sizeof(simd16<T>) * 2);
    this->chunks[3].store_ascii_as_utf16(ptr + sizeof(simd16<T>) * 3);
  }

  simdutf_really_inline uint64_t to_bitmask() const {
    uint64_t r0 = uint32_t(this->chunks[0].to_bitmask());
    uint64_t r1 = this->chunks[1].to_bitmask();
    uint64_t r2 = this->chunks[2].to_bitmask();
    uint64_t r3 = this->chunks[3].to_bitmask();
    return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
  }

  simdutf_really_inline void swap_bytes() {
    this->chunks[0] = this->chunks[0].swap_bytes();
    this->chunks[1] = this->chunks[1].swap_bytes();
    this->chunks[2] = this->chunks[2].swap_bytes();
    this->chunks[3] = this->chunks[3].swap_bytes();
  }

  simdutf_really_inline uint64_t eq(const T m) const {
    const simd16<T> mask = simd16<T>::splat(m);
    return simd16x32<bool>(this->chunks[0] == mask, this->chunks[1] == mask,
                           this->chunks[2] == mask, this->chunks[3] == mask)
        .to_bitmask();
  }

  simdutf_really_inline uint64_t eq(const simd16x32<uint16_t> &other) const {
    return simd16x32<bool>(this->chunks[0] == other.chunks[0],
                           this->chunks[1] == other.chunks[1],
                           this->chunks[2] == other.chunks[2],
                           this->chunks[3] == other.chunks[3])
        .to_bitmask();
  }

  simdutf_really_inline uint64_t lteq(const T m) const {
    const simd16<T> mask = simd16<T>::splat(m);
    return simd16x32<bool>(this->chunks[0] <= mask, this->chunks[1] <= mask,
                           this->chunks[2] <= mask, this->chunks[3] <= mask)
        .to_bitmask();
  }

  simdutf_really_inline uint64_t in_range(const T low, const T high) const {
    const simd16<T> mask_low = simd16<T>::splat(low);
    const simd16<T> mask_high = simd16<T>::splat(high);

    return simd16x32<bool>(
               (this->chunks[0] <= mask_high) & (this->chunks[0] >= mask_low),
               (this->chunks[1] <= mask_high) & (this->chunks[1] >= mask_low),
               (this->chunks[2] <= mask_high) & (this->chunks[2] >= mask_low),
               (this->chunks[3] <= mask_high) & (this->chunks[3] >= mask_low))
        .to_bitmask();
  }
  simdutf_really_inline uint64_t not_in_range(const T low, const T high) const {
    const simd16<T> mask_low = simd16<T>::splat(static_cast<T>(low - 1));
    const simd16<T> mask_high = simd16<T>::splat(static_cast<T>(high + 1));
    return simd16x32<bool>(
               (this->chunks[0] >= mask_high) | (this->chunks[0] <= mask_low),
               (this->chunks[1] >= mask_high) | (this->chunks[1] <= mask_low),
               (this->chunks[2] >= mask_high) | (this->chunks[2] <= mask_low),
               (this->chunks[3] >= mask_high) | (this->chunks[3] <= mask_low))
        .to_bitmask();
  }
  simdutf_really_inline uint64_t lt(const T m) const {
    const simd16<T> mask = simd16<T>::splat(m);
    return simd16x32<bool>(this->chunks[0] < mask, this->chunks[1] < mask,
                           this->chunks[2] < mask, this->chunks[3] < mask)
        .to_bitmask();
  }
}; // struct simd16x32<T>
