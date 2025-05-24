// file included directly

template <typename T> struct base8 {
  using vector_type = vector_u8_type_for_element<T>;
  vector_type value;
  static const int SIZE = sizeof(vector_type);
  static const int ELEMENTS = sizeof(vector_type) / sizeof(T);

  // Zero constructor
  simdutf_really_inline base8() : value{vec_splats(T(0))} {}

  // Conversion from SIMD register
  simdutf_really_inline base8(const vector_type _value) : value{_value} {}

  // Splat scalar
  simdutf_really_inline base8(T v) : value{vec_splats(v)} {}

  // Conversion to SIMD register
  simdutf_really_inline operator const vector_type &() const {
    return this->value;
  }

  template <typename U> simdutf_really_inline void store(U *ptr) const {
    vec_xst(value, 0, reinterpret_cast<T *>(ptr));
  }

  template <typename SIMD8> void operator|=(const SIMD8 other) {
    this->value = vec_or(this->value, other.value);
  }

  template <int N = 1> vector_type prev_aux(vector_type prev_chunk) const {
    vector_type chunk = this->value;
#if !SIMDUTF_IS_BIG_ENDIAN
    chunk = (vector_type)vec_reve(this->value);
    prev_chunk = (vector_type)vec_reve((vector_type)prev_chunk);
#endif
    chunk = (vector_type)vec_sld((vector_type)prev_chunk, (vector_type)chunk,
                                 16 - N);
#if !SIMDUTF_IS_BIG_ENDIAN
    chunk = (vector_type)vec_reve((vector_type)chunk);
#endif
    return chunk;
  }

  simdutf_really_inline bool is_ascii() const {
    return move_mask_u8(this->value) == 0;
  }

  simdutf_really_inline uint16_t to_bitmask() const {
    return move_mask_u8(value);
  }

  template <endianness big_endian>
  simdutf_really_inline void store_bytes_as_utf16(char16_t *p) const {
    const vector_type zero = vec_splats(T(0));

    if (big_endian) {
      const vec_u8_t perm_lo = {16, 0, 16, 1, 16, 2, 16, 3,
                                16, 4, 16, 5, 16, 6, 16, 7};
      const vec_u8_t perm_hi = {16, 8,  16, 9,  16, 10, 16, 11,
                                16, 12, 16, 13, 16, 14, 16, 15};

      const vector_type v0 = vec_perm(value, zero, perm_lo);
      const vector_type v1 = vec_perm(value, zero, perm_hi);

#if defined(__clang__)
      vec_xst(v0, 0, reinterpret_cast<T *>(p));
      vec_xst(v1, 16, reinterpret_cast<T *>(p));
#else
      vec_xst(v0, 0, reinterpret_cast<vector_type *>(p));
      vec_xst(v1, 16, reinterpret_cast<vector_type *>(p));
#endif // defined(__clang__)
    } else {
      const vec_u8_t perm_lo = {0, 16, 1, 16, 2, 16, 3, 16,
                                4, 16, 5, 16, 6, 16, 7, 16};
      const vec_u8_t perm_hi = {8,  16, 9,  16, 10, 16, 11, 16,
                                12, 16, 13, 16, 14, 16, 15, 16};

      const vector_type v0 = vec_perm(value, zero, perm_lo);
      const vector_type v1 = vec_perm(value, zero, perm_hi);

#if defined(__clang__)
      vec_xst(v0, 0, reinterpret_cast<T *>(p));
      vec_xst(v1, 16, reinterpret_cast<T *>(p));
#else
      vec_xst(v0, 0, reinterpret_cast<vector_type *>(p));
      vec_xst(v1, 16, reinterpret_cast<vector_type *>(p));
#endif // defined(__clang__)
    }
  }

  template <endianness big_endian>
  simdutf_really_inline void store_ascii_as_utf16(char16_t *p) const {
    store_bytes_as_utf16<big_endian>(p);
  }

  simdutf_really_inline void store_bytes_as_utf32(char32_t *p) const {
    const vector_type zero = vec_splats(T(0));

#if SIMDUTF_IS_BIG_ENDIAN
    const vec_u8_t perm0 = {16, 16, 16, 0, 16, 16, 16, 1,
                            16, 16, 16, 2, 16, 16, 16, 3};

    const vec_u8_t perm1 = {16, 16, 16, 4, 16, 16, 16, 5,
                            16, 16, 16, 6, 16, 16, 16, 7};

    const vec_u8_t perm2 = {16, 16, 16, 8,  16, 16, 16, 9,
                            16, 16, 16, 10, 16, 16, 16, 11};

    const vec_u8_t perm3 = {16, 16, 16, 12, 16, 16, 16, 13,
                            16, 16, 16, 14, 16, 16, 16, 15};
#else
    const vec_u8_t perm0 = {0, 16, 16, 16, 1, 16, 16, 16,
                            2, 16, 16, 16, 3, 16, 16, 16};

    const vec_u8_t perm1 = {4, 16, 16, 16, 5, 16, 16, 16,
                            6, 16, 16, 16, 7, 16, 16, 16};

    const vec_u8_t perm2 = {8,  16, 16, 16, 9,  16, 16, 16,
                            10, 16, 16, 16, 11, 16, 16, 16};

    const vec_u8_t perm3 = {12, 16, 16, 16, 13, 16, 16, 16,
                            14, 16, 16, 16, 15, 16, 16, 16};
#endif // SIMDUTF_IS_BIG_ENDIAN

    const vector_type v0 = vec_perm(value, zero, perm0);
    const vector_type v1 = vec_perm(value, zero, perm1);
    const vector_type v2 = vec_perm(value, zero, perm2);
    const vector_type v3 = vec_perm(value, zero, perm3);

    constexpr size_t n = base8<T>::SIZE;

#if defined(__clang__)
    vec_xst(v0, 0 * n, reinterpret_cast<T *>(p));
    vec_xst(v1, 1 * n, reinterpret_cast<T *>(p));
    vec_xst(v2, 2 * n, reinterpret_cast<T *>(p));
    vec_xst(v3, 3 * n, reinterpret_cast<T *>(p));
#else
    vec_xst(v0, 0 * n, reinterpret_cast<vector_type *>(p));
    vec_xst(v1, 1 * n, reinterpret_cast<vector_type *>(p));
    vec_xst(v2, 2 * n, reinterpret_cast<vector_type *>(p));
    vec_xst(v3, 3 * n, reinterpret_cast<vector_type *>(p));
#endif // defined(__clang__)
  }

  simdutf_really_inline void store_words_as_utf32(char32_t *p) const {
    const vector_type zero = vec_splats(T(0));

#if SIMDUTF_IS_BIG_ENDIAN
    const vec_u8_t perm0 = {16, 16, 0, 1, 16, 16, 2, 3,
                            16, 16, 4, 5, 16, 16, 6, 7};
    const vec_u8_t perm1 = {16, 16, 8,  9,  16, 16, 10, 11,
                            16, 16, 12, 13, 16, 16, 14, 15};
#else
    const vec_u8_t perm0 = {0, 1, 16, 16, 2, 3, 16, 16,
                            4, 5, 16, 16, 6, 7, 16, 16};
    const vec_u8_t perm1 = {8,  9,  16, 16, 10, 11, 16, 16,
                            12, 13, 16, 16, 14, 15, 16, 16};
#endif // SIMDUTF_IS_BIG_ENDIAN

    const vector_type v0 = vec_perm(value, zero, perm0);
    const vector_type v1 = vec_perm(value, zero, perm1);

    constexpr size_t n = base8<T>::SIZE;

#if defined(__clang__)
    vec_xst(v0, 0 * n, reinterpret_cast<T *>(p));
    vec_xst(v1, 1 * n, reinterpret_cast<T *>(p));
#else
    vec_xst(v0, 0 * n, reinterpret_cast<vector_type *>(p));
    vec_xst(v1, 1 * n, reinterpret_cast<vector_type *>(p));
#endif // defined(__clang__)
  }

  simdutf_really_inline void store_ascii_as_utf32(char32_t *p) const {
    store_bytes_as_utf32(p);
  }
};

// Forward declaration
template <typename T> struct simd8;

template <typename T>
simd8<bool> operator==(const simd8<T> a, const simd8<T> b);

template <typename T>
simd8<bool> operator!=(const simd8<T> a, const simd8<T> b);

template <typename T> simd8<T> operator&(const simd8<T> a, const simd8<T> b);

template <typename T> simd8<T> operator|(const simd8<T> a, const simd8<T> b);

template <typename T> simd8<T> operator^(const simd8<T> a, const simd8<T> b);

template <typename T> simd8<T> operator+(const simd8<T> a, const simd8<T> b);

template <typename T> simd8<bool> operator<(const simd8<T> a, const simd8<T> b);

// SIMD byte mask type (returned by things like eq and gt)
template <> struct simd8<bool> : base8<bool> {
  using super = base8<bool>;

  static simdutf_really_inline simd8<bool> splat(bool _value) {
    return (vector_type)vec_splats((unsigned char)(-(!!_value)));
  }

  simdutf_really_inline simd8() : super(vector_type()) {}
  simdutf_really_inline simd8(const vector_type _value) : super(_value) {}
  // Splat constructor
  simdutf_really_inline simd8(bool _value) : base8<bool>(splat(_value)) {}

  template <typename T>
  simdutf_really_inline simd8(simd8<T> other)
      : simd8(vector_type(other.value)) {}

  simdutf_really_inline uint16_t to_bitmask() const {
    return move_mask_u8(value);
  }

  simdutf_really_inline bool any() const {
    return !vec_all_eq(this->value, (vector_type)vec_splats(0));
  }

  simdutf_really_inline bool all() const { return to_bitmask() == 0xffff; }

  simdutf_really_inline simd8<bool> operator~() const {
    return this->value ^ (vector_type)splat(true);
  }
};

template <typename T> struct base8_numeric : base8<T> {
  using super = base8<T>;
  using vector_type = typename super::vector_type;

  static simdutf_really_inline simd8<T> splat(T value) {
    return (vector_type)vec_splats(value);
  }

  static simdutf_really_inline simd8<T> zero() { return splat(0); }

  template <typename U>
  static simdutf_really_inline simd8<T> load(const U *values) {
    return vec_xl(0, reinterpret_cast<const T *>(values));
  }

  // Repeat 16 values as many times as necessary (usually for lookup tables)
  static simdutf_really_inline simd8<T> repeat_16(T v0, T v1, T v2, T v3, T v4,
                                                  T v5, T v6, T v7, T v8, T v9,
                                                  T v10, T v11, T v12, T v13,
                                                  T v14, T v15) {
    return simd8<T>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                    v14, v15);
  }

  simdutf_really_inline base8_numeric() : base8<T>() {}
  simdutf_really_inline base8_numeric(const vector_type _value)
      : base8<T>(_value) {}

  // Override to distinguish from bool version
  simdutf_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

  simdutf_really_inline simd8<T> &operator-=(const simd8<T> other) {
    this->value = vec_sub(this->value, other.value);
    return *static_cast<simd8<T> *>(this);
  }

  // Perform a lookup assuming the value is between 0 and 16 (undefined behavior
  // for out of range values)
  template <typename L>
  simdutf_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
    return (vector_type)vec_perm((vector_type)lookup_table,
                                 (vector_type)lookup_table, this->value);
  }

  template <typename L>
  simdutf_really_inline simd8<L>
  lookup_32(const simd8<L> lookup_table_lo,
            const simd8<L> lookup_table_hi) const {
    return (vector_type)vec_perm(lookup_table_lo.value, lookup_table_hi.value,
                                 this->value);
  }

  template <typename L>
  simdutf_really_inline simd8<L>
  lookup_16(L replace0, L replace1, L replace2, L replace3, L replace4,
            L replace5, L replace6, L replace7, L replace8, L replace9,
            L replace10, L replace11, L replace12, L replace13, L replace14,
            L replace15) const {
    return lookup_16(simd8<L>::repeat_16(
        replace0, replace1, replace2, replace3, replace4, replace5, replace6,
        replace7, replace8, replace9, replace10, replace11, replace12,
        replace13, replace14, replace15));
  }
};

// Unsigned bytes
template <> struct simd8<uint8_t> : base8_numeric<uint8_t> {
  using Self = simd8<uint8_t>;

  simdutf_really_inline simd8() : base8_numeric<uint8_t>() {}
  simdutf_really_inline simd8(const vector_type _value)
      : base8_numeric<uint8_t>(_value) {}
  // Splat constructor
  simdutf_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
  // Array constructor
  simdutf_really_inline simd8(const uint8_t *values) : simd8(load(values)) {}
  // Member-by-member initialization
  simdutf_really_inline
  simd8(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5,
        uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9, uint8_t v10,
        uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15)
      : simd8((vector_type){v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11,
                            v12, v13, v14, v15}) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  simdutf_really_inline static simd8<uint8_t>
  repeat_16(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4,
            uint8_t v5, uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9,
            uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14,
            uint8_t v15) {
    return simd8<uint8_t>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                          v13, v14, v15);
  }

  simdutf_really_inline bool is_ascii() const {
    return move_mask_u8(this->value) == 0;
  }

  template <typename T>
  simdutf_really_inline simd8(simd8<T> other)
      : simd8(vector_type(other.value)) {}

  template <int N>
  simdutf_really_inline Self prev(const Self prev_chunk) const {
    return prev_aux<N>(prev_chunk.value);
  }

  // Saturated math
  simdutf_really_inline simd8<uint8_t>
  saturating_sub(const simd8<uint8_t> other) const {
    return (vector_type)vec_subs(this->value, (vector_type)other);
  }

  // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
  simdutf_really_inline simd8<uint8_t>
  gt_bits(const simd8<uint8_t> other) const {
    return this->saturating_sub(other);
  }

  // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
  simdutf_really_inline simd8<uint8_t>
  lt_bits(const simd8<uint8_t> other) const {
    return other.saturating_sub(*this);
  }

  // Bit-specific operations
  simdutf_really_inline bool bits_not_set_anywhere() const {
    return vec_all_eq(this->value, (vector_type)vec_splats(0));
  }

  simdutf_really_inline bool any_bits_set_anywhere() const {
    return !bits_not_set_anywhere();
  }

  template <int N> simdutf_really_inline simd8<uint8_t> shr() const {
    return simd8<uint8_t>(
        (vector_type)vec_sr(this->value, (vector_type)vec_splat_u8(N)));
  }

  template <int N> simdutf_really_inline simd8<uint8_t> shl() const {
    return simd8<uint8_t>(
        (vector_type)vec_sl(this->value, (vector_type)vec_splat_u8(N)));
  }
  void dump() const {
#ifdef SIMDUTF_LOGGING
    uint8_t tmp[16];
    store(tmp);
    for (int i = 0; i < 16; i++) {
      if (i == 0) {
        printf("[%02x", tmp[i]);
      } else if (i == 15) {
        printf(" %02x]", tmp[i]);
      } else {
        printf(" %02x", tmp[i]);
      }
    }
    putchar('\n');
#endif // SIMDUTF_LOGGING
  }

  void dump_ascii() const {
#ifdef SIMDUTF_LOGGING
    uint8_t tmp[16];
    store(tmp);
    for (int i = 0; i < 16; i++) {
      if (i == 0) {
        printf("[%c", tmp[i]);
      } else if (i == 15) {
        printf("%c]", tmp[i]);
      } else {
        printf("%c", tmp[i]);
      }
    }
    putchar('\n');
#endif // SIMDUTF_LOGGING
  }
};

// Signed bytes
template <> struct simd8<int8_t> : base8_numeric<int8_t> {
  simdutf_really_inline simd8() : base8_numeric<int8_t>() {}
  simdutf_really_inline simd8(const vector_type _value)
      : base8_numeric<int8_t>(_value) {}

  template <typename T>
  simdutf_really_inline simd8(simd8<T> other)
      : simd8(vector_type(other.value)) {}

  // Splat constructor
  simdutf_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
  // Array constructor
  simdutf_really_inline simd8(const int8_t *values) : simd8(load(values)) {}

  simdutf_really_inline operator simd8<uint8_t>() const;

  // Saturated math
  simdutf_really_inline simd8<int8_t>
  saturating_add(const simd8<int8_t> other) const {
    return (vector_type)vec_adds(this->value, other.value);
  }

  void dump() const {
    int8_t tmp[16];
    store(tmp);
    for (int i = 0; i < 16; i++) {
      if (i == 0) {
        printf("[%02x", tmp[i]);
      } else if (i == 15) {
        printf("%02x]", tmp[i]);
      } else {
        printf("%02x", tmp[i]);
      }
    }
    putchar('\n');
  }
};

template <typename T>
simd8<bool> operator==(const simd8<T> a, const simd8<T> b) {
  return vec_cmpeq(a.value, b.value);
}

template <typename T>
simd8<bool> operator!=(const simd8<T> a, const simd8<T> b) {
  return vec_cmpne(a.value, b.value);
}

template <typename T> simd8<T> operator&(const simd8<T> a, const simd8<T> b) {
  return vec_and(a.value, b.value);
}

template <typename T, typename U> simd8<T> operator&(const simd8<T> a, U b) {
  return vec_and(a.value, vec_splats(T(b)));
}

template <typename T> simd8<T> operator|(const simd8<T> a, const simd8<T> b) {
  return vec_or(a.value, b.value);
}

template <typename T> simd8<T> operator^(const simd8<T> a, const simd8<T> b) {
  return vec_xor(a.value, b.value);
}

template <typename T, typename U> simd8<T> operator^(const simd8<T> a, U b) {
  return vec_xor(a.value, vec_splats(T(b)));
}

template <typename T> simd8<T> operator+(const simd8<T> a, const simd8<T> b) {
  return vec_add(a.value, b.value);
}

template <typename T, typename U> simd8<T> operator+(const simd8<T> a, U b) {
  return vec_add(a.value, vec_splats(T(b)));
}

simdutf_really_inline simd8<int8_t>::operator simd8<uint8_t>() const {
  return (simd8<uint8_t>::vector_type)value;
}

template <typename T>
simd8<bool> operator<(const simd8<T> a, const simd8<T> b) {
  return vec_cmplt(a.value, b.value);
}

template <typename T>
simd8<bool> operator>(const simd8<T> a, const simd8<T> b) {
  return vec_cmpgt(a.value, b.value);
}

template <typename T>
simd8<bool> operator>=(const simd8<T> a, const simd8<T> b) {
  return vec_cmpge(a.value, b.value);
}

template <typename T> struct simd8x64 {
  static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
  static constexpr size_t ELEMENTS = simd8<T>::ELEMENTS;

  static_assert(NUM_CHUNKS == 4,
                "PPC64 kernel should use four registers per 64-byte block.");
  simd8<T> chunks[NUM_CHUNKS];

  simd8x64(const simd8x64<T> &o) = delete; // no copy allowed
  simd8x64<T> &
  operator=(const simd8<T> other) = delete; // no assignment allowed
  simd8x64() = delete;                      // no default constructor allowed
  simd8x64(simd8x64<T> &&) = default;

  simdutf_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1,
                                 const simd8<T> chunk2, const simd8<T> chunk3)
      : chunks{chunk0, chunk1, chunk2, chunk3} {}
  simdutf_really_inline simd8x64(const T *ptr)
      : chunks{simd8<T>::load(ptr),
               simd8<T>::load(ptr + sizeof(simd8<T>) / sizeof(T)),
               simd8<T>::load(ptr + 2 * sizeof(simd8<T>) / sizeof(T)),
               simd8<T>::load(ptr + 3 * sizeof(simd8<T>) / sizeof(T))} {}

  simdutf_really_inline void store(T *ptr) const {
    this->chunks[0].store(ptr + ELEMENTS * 0);
    this->chunks[1].store(ptr + ELEMENTS * 1);
    this->chunks[2].store(ptr + ELEMENTS * 2);
    this->chunks[3].store(ptr + ELEMENTS * 3);
  }

  simdutf_really_inline simd8x64<T> &operator|=(const simd8x64<T> &other) {
    this->chunks[0] |= other.chunks[0];
    this->chunks[1] |= other.chunks[1];
    this->chunks[2] |= other.chunks[2];
    this->chunks[3] |= other.chunks[3];
    return *this;
  }

  simdutf_really_inline simd8<T> reduce_or() const {
    return (this->chunks[0] | this->chunks[1]) |
           (this->chunks[2] | this->chunks[3]);
  }

  simdutf_really_inline bool is_ascii() const {
    return this->reduce_or().is_ascii();
  }

  template <endianness endian>
  simdutf_really_inline void store_ascii_as_utf16(char16_t *ptr) const {
    this->chunks[0].template store_ascii_as_utf16<endian>(ptr +
                                                          sizeof(simd8<T>) * 0);
    this->chunks[1].template store_ascii_as_utf16<endian>(ptr +
                                                          sizeof(simd8<T>) * 1);
    this->chunks[2].template store_ascii_as_utf16<endian>(ptr +
                                                          sizeof(simd8<T>) * 2);
    this->chunks[3].template store_ascii_as_utf16<endian>(ptr +
                                                          sizeof(simd8<T>) * 3);
  }

  simdutf_really_inline void store_ascii_as_utf32(char32_t *ptr) const {
    this->chunks[0].store_ascii_as_utf32(ptr + sizeof(simd8<T>) * 0);
    this->chunks[1].store_ascii_as_utf32(ptr + sizeof(simd8<T>) * 1);
    this->chunks[2].store_ascii_as_utf32(ptr + sizeof(simd8<T>) * 2);
    this->chunks[3].store_ascii_as_utf32(ptr + sizeof(simd8<T>) * 3);
  }

  simdutf_really_inline uint64_t to_bitmask() const {
    uint64_t r0 = uint32_t(this->chunks[0].to_bitmask());
    uint64_t r1 = this->chunks[1].to_bitmask();
    uint64_t r2 = this->chunks[2].to_bitmask();
    uint64_t r3 = this->chunks[3].to_bitmask();
    return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
  }

  simdutf_really_inline uint64_t lt(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] < mask, this->chunks[1] < mask,
                          this->chunks[2] < mask, this->chunks[3] < mask)
        .to_bitmask();
  }

  simdutf_really_inline uint64_t gt(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] > mask, this->chunks[1] > mask,
                          this->chunks[2] > mask, this->chunks[3] > mask)
        .to_bitmask();
  }
  simdutf_really_inline uint64_t eq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] == mask, this->chunks[1] == mask,
                          this->chunks[2] == mask, this->chunks[3] == mask)
        .to_bitmask();
  }
  simdutf_really_inline uint64_t gteq_unsigned(const uint8_t m) const {
    const simd8<uint8_t> mask = simd8<uint8_t>::splat(m);
    return simd8x64<bool>(simd8<uint8_t>(this->chunks[0]) >= mask,
                          simd8<uint8_t>(this->chunks[1]) >= mask,
                          simd8<uint8_t>(this->chunks[2]) >= mask,
                          simd8<uint8_t>(this->chunks[3]) >= mask)
        .to_bitmask();
  }

  void dump() const {
    puts("");
    for (int i = 0; i < 4; i++) {
      printf("chunk[%d] = ", i);
      this->chunks[i].dump();
    }
  }
}; // struct simd8x64<T>

simdutf_really_inline simd8<uint8_t> avg(const simd8<uint8_t> a,
                                         const simd8<uint8_t> b) {
  return vec_avg(a.value, b.value);
}
