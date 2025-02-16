// file included directly

template <typename T> struct simd32;

template <typename T> struct base32 {
  using vector_type = vector_u32_type_for_element<T>;
  static const int SIZE = sizeof(vector_type);
  static const int ELEMENTS = sizeof(vector_type) / sizeof(T);

  vector_type value;

  // Zero constructor
  simdutf_really_inline base32() : value{vector_type()} {}

  // Conversion from SIMD register
  simdutf_really_inline base32(const vector_type _value) : value{_value} {}

  // Splat for scalar
  simdutf_really_inline base32(T scalar) : value{vec_splats(scalar)} {}

  template <typename Pointer>
  simdutf_really_inline base32(const Pointer *ptr)
      : base32(vec_xl(0, reinterpret_cast<const T *>(ptr))) {}

  // Store to array
  template <typename U> simdutf_really_inline void store(U *dst) const {
    return vec_xst(this->value, 0, reinterpret_cast<vector_type *>(dst));
  }

  void dump() const {
    uint32_t tmp[4];
    vec_xst(value, 0, reinterpret_cast<vector_type *>(tmp));
    for (int i = 0; i < 4; i++) {
      if (i == 0) {
        printf("[%08x", tmp[i]);
      } else if (i == 4 - 1) {
        printf(" %08x]", tmp[i]);
      } else {
        printf(" %08x", tmp[i]);
      }
    }
    putchar('\n');
  }
};

template <typename T> struct base32_numeric : base32<T> {
  using super = base32<T>;
  using vector_type = typename super::vector_type;

  static simdutf_really_inline simd32<T> splat(T _value) {
    return vec_splats(_value);
  }
  static simdutf_really_inline simd32<T> zero() { return splat(0); }

  template <typename U>
  static simdutf_really_inline simd32<T> load(const U *values) {
    return vec_xl(0, reinterpret_cast<const T *>(values));
  }

  simdutf_really_inline base32_numeric() : base32<T>() {}
  simdutf_really_inline base32_numeric(const vector_type _value)
      : base32<T>(_value) {}

  // Override to distinguish from bool version
  simdutf_really_inline simd32<T> operator~() const {
    return *this ^ 0xffffffffffu;
  }

  // Addition/subtraction are the same for signed and unsigned
  simdutf_really_inline simd32<T> operator+(const simd32<T> other) const {
    return vec_add(this->value, other.value);
  }

  simdutf_really_inline simd32<T> operator-(const simd32<T> other) const {
    return vec_sub(this->value, other.value);
  }

  simdutf_really_inline simd32<T> &operator+=(const simd32<T> other) {
    *this = *this + other;
    return *static_cast<simd32<T> *>(this);
  }

  simdutf_really_inline simd32<T> &operator-=(const simd32<T> other) {
    *this = *this - other;
    return *static_cast<simd32<T> *>(this);
  }
};

// Forward declaration
template <typename> struct simd32;

template <typename T>
simd32<bool> operator==(const simd32<T> a, const simd32<T> b);

template <typename T>
simd32<bool> operator!=(const simd32<T> a, const simd32<T> b);

template <typename T>
simd32<bool> operator>(const simd32<T> a, const simd32<T> b);

template <typename T> simd32<bool> operator==(const simd32<T> a, T b);

template <typename T> simd32<bool> operator!=(const simd32<T> a, T b);

template <typename T> simd32<T> operator&(const simd32<T> a, const simd32<T> b);

template <typename T> simd32<T> operator|(const simd32<T> a, const simd32<T> b);

template <typename T> simd32<T> operator^(const simd32<T> a, const simd32<T> b);

// SIMD byte mask type (returned by things like eq and gt)
template <> struct simd32<bool> : base32<bool> {
  static simdutf_really_inline simd32<bool> splat(bool _value) {
    return (vector_type)vec_splats(uint32_t(-(!!_value)));
  }

  simdutf_really_inline simd32() : base32() {}

  simdutf_really_inline simd32(const vector_type _value)
      : base32<bool>(_value) {}

  // Splat constructor
  simdutf_really_inline simd32(bool _value) : base32<bool>(splat(_value)) {}

  simdutf_really_inline uint16_t to_bitmask() const {
    return move_mask_u8(value);
  }

  simdutf_really_inline bool any() const {
    const vec_u64_t tmp = (vec_u64_t)value;

    return tmp[0] || tmp[1]; // Note: logical or, not binary one
  }

  simdutf_really_inline bool is_zero() const {
    const vec_u64_t tmp = (vec_u64_t)value;

    return (tmp[0] | tmp[1]) == 0;
  }

  simdutf_really_inline simd32<bool> operator~() const {
    return (vec_bool32_t)vec_xor(this->value, vec_splats(uint32_t(0xffffffff)));
  }
};

// Unsigned code units
template <> struct simd32<uint32_t> : base32_numeric<uint32_t> {
  simdutf_really_inline simd32() : base32_numeric<uint32_t>() {}
  simdutf_really_inline simd32(const vector_type _value)
      : base32_numeric<uint32_t>(_value) {}

  // Splat constructor
  simdutf_really_inline simd32(uint32_t _value) : simd32(splat(_value)) {}
  // Array constructor
  simdutf_really_inline simd32(const uint32_t *values) : simd32(load(values)) {}
  simdutf_really_inline simd32(const char32_t *values)
      : simd32(load(reinterpret_cast<const uint32_t *>(values))) {}
  // Member-by-member initialization
  simdutf_really_inline simd32(uint32_t v0, uint32_t v1, uint32_t v2,
                               uint32_t v3)
      : simd32((vector_type){v0, v1, v2, v3}) {}

  // Repeat 32 values as many times as necessary (usually for lookup tables)
  simdutf_really_inline static simd32<uint32_t>
  repeat_32(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3) {
    return simd32<uint32_t>(v0, v1, v2, v3);
  }

  // Saturated math
  simdutf_really_inline simd32<uint32_t>
  saturating_add(const simd32<uint32_t> other) const {
    return vec_adds(this->value, other.value);
  }
  simdutf_really_inline simd32<uint32_t>
  saturating_sub(const simd32<uint32_t> other) const {
    return vec_subs(this->value, other.value);
  }

  // Order-specific operations
  simdutf_really_inline simd32<uint32_t>
  max_val(const simd32<uint32_t> other) const {
    return vec_max(this->value, other.value);
  }
  simdutf_really_inline simd32<uint32_t>
  min_val(const simd32<uint32_t> other) const {
    return vec_min(this->value, other.value);
  }
  // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
  simdutf_really_inline simd32<uint32_t>
  gt_bits(const simd32<uint32_t> other) const {
    return this->saturating_sub(other);
  }
  // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
  simdutf_really_inline simd32<uint32_t>
  lt_bits(const simd32<uint32_t> other) const {
    return other.saturating_sub(*this);
  }

  // Bit-specific operations
  simdutf_really_inline simd32<bool> bits_not_set() const {
    return *this == uint32_t(0);
  }
  simdutf_really_inline simd32<bool> bits_not_set(simd32<uint32_t> bits) const {
    return (*this & bits).bits_not_set();
  }
  simdutf_really_inline simd32<bool> any_bits_set() const {
    return ~this->bits_not_set();
  }
  simdutf_really_inline simd32<bool> any_bits_set(simd32<uint32_t> bits) const {
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
  bits_not_set_anywhere(simd32<uint32_t> bits) const {
    const vec_u64_t tmp = vec_u64_t(vec_and(this->value, bits.value));

    return (tmp[0] | tmp[1]) == 0;
  }
  simdutf_really_inline bool
  any_bits_set_anywhere(simd32<uint32_t> bits) const {
    return !bits_not_set_anywhere(bits);
  }
  template <int N> simdutf_really_inline simd32<uint32_t> shr() const {
    return vec_sr(value, vec_splats(uint32_t(N)));
  }
  template <int N> simdutf_really_inline simd32<uint32_t> shl() const {
    return vec_sl(value, vec_splats(uint32_t(N)));
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
  simdutf_really_inline simd32<uint32_t> swap_bytes() const {
    return vec_revb(value);
  }

  static simdutf_really_inline simd16<uint16_t>
  pack(const simd32<uint32_t> &v0, const simd32<uint32_t> &v1) {
    return vec_packs(v0.value, v1.value);
  }
};

template <typename T>
simd32<bool> operator==(const simd32<T> a, const simd32<T> b) {
  return vec_cmpeq(a.value, b.value);
}

template <typename T>
simd32<bool> operator!=(const simd32<T> a, const simd32<T> b) {
  return vec_cmpne(a.value, b.value);
}

template <typename T> simd32<bool> operator==(const simd32<T> a, T b) {
  return vec_cmpeq(a.value, vec_splats(b));
}

template <typename T> simd32<bool> operator!=(const simd32<T> a, T b) {
  return vec_cmpne(a.value, vec_splats(b));
}

template <typename T>
simd32<bool> operator>(const simd32<T> a, const simd32<T> b) {
  return vec_cmpgt(a.value, b.value);
}

template <typename T>
simd32<T> operator&(const simd32<T> a, const simd32<T> b) {
  return vec_and(a.value, b.value);
}

template <typename T, typename U> simd32<T> operator&(const simd32<T> a, U b) {
  return vec_and(a.value, vec_splats(T(b)));
}

template <typename T>
simd32<T> operator|(const simd32<T> a, const simd32<T> b) {
  return vec_or(a.value, b.value);
}

template <typename T>
simd32<T> operator^(const simd32<T> a, const simd32<T> b) {
  return vec_xor(a.value, b.value);
}

template <typename T, typename U> simd32<T> operator^(const simd32<T> a, U b) {
  return vec_xor(a.value, vec_splats(T(b)));
}

template <typename T> simd32<T> max_val(const simd32<T> a, const simd32<T> b) {
  return vec_max(a.value, b.value);
}
