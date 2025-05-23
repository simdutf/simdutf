template <LMUL> struct vector_u8_type;
template <> struct vector_u8_type<LMUL::M1> {
  using type = vuint8m1_t;
};
template <> struct vector_u8_type<LMUL::M2> {
  using type = vuint8m2_t;
};
template <> struct vector_u8_type<LMUL::M4> {
  using type = vuint8m4_t;
};
template <> struct vector_u8_type<LMUL::M8> {
  using type = vuint8m8_t;
};

template <LMUL> struct vector_u16_type;
template <> struct vector_u16_type<LMUL::M1> {
  using type = vuint16m1_t;
};
template <> struct vector_u16_type<LMUL::M2> {
  using type = vuint16m2_t;
};
template <> struct vector_u16_type<LMUL::M4> {
  using type = vuint16m4_t;
};
template <> struct vector_u16_type<LMUL::M8> {
  using type = vuint16m8_t;
};

template <LMUL> struct vector_u32_type;
template <> struct vector_u32_type<LMUL::M1> {
  using type = vuint32m1_t;
};
template <> struct vector_u32_type<LMUL::M2> {
  using type = vuint32m2_t;
};
template <> struct vector_u32_type<LMUL::M4> {
  using type = vuint32m4_t;
};
template <> struct vector_u32_type<LMUL::M8> {
  using type = vuint32m8_t;
};

template <typename> struct vector_element;
template <> struct vector_element<vuint32m1_t> {
  using type = uint32_t;
};
template <> struct vector_element<vuint32m2_t> {
  using type = uint32_t;
};
template <> struct vector_element<vuint32m4_t> {
  using type = uint32_t;
};
template <> struct vector_element<vuint32m8_t> {
  using type = uint32_t;
};

// ------------------------------------------------------------

template <typename T> T rvv_vle8_v(const uint8_t *, size_t);

template <> vuint8m8_t rvv_vle8_v<vuint8m8_t>(const uint8_t *ptr, size_t vl) {
  return __riscv_vle8_v_u8m8(ptr, vl);
}

template <> vuint8m4_t rvv_vle8_v<vuint8m4_t>(const uint8_t *ptr, size_t vl) {
  return __riscv_vle8_v_u8m4(ptr, vl);
}

template <> vuint8m2_t rvv_vle8_v<vuint8m2_t>(const uint8_t *ptr, size_t vl) {
  return __riscv_vle8_v_u8m2(ptr, vl);
}

template <> vuint8m1_t rvv_vle8_v<vuint8m1_t>(const uint8_t *ptr, size_t vl) {
  return __riscv_vle8_v_u8m1(ptr, vl);
}

// ------------------------------------------------------------

template <typename T> void rvv_vse8_v(uint8_t *, T, size_t);

template <>
void rvv_vse8_v<vuint8m1_t>(uint8_t *ptr, vuint8m1_t val, size_t vl) {
  return __riscv_vse8_v_u8m1(ptr, val, vl);
}

template <>
void rvv_vse8_v<vuint8m2_t>(uint8_t *ptr, vuint8m2_t val, size_t vl) {
  return __riscv_vse8_v_u8m2(ptr, val, vl);
}

template <>
void rvv_vse8_v<vuint8m4_t>(uint8_t *ptr, vuint8m4_t val, size_t vl) {
  return __riscv_vse8_v_u8m4(ptr, val, vl);
}

template <>
void rvv_vse8_v<vuint8m8_t>(uint8_t *ptr, vuint8m8_t val, size_t vl) {
  return __riscv_vse8_v_u8m8(ptr, val, vl);
}

// ------------------------------------------------------------

template <typename T> size_t rvv_vsetvl(size_t vl);

template <> size_t rvv_vsetvl<vuint8m1_t>(size_t vl) {
  return __riscv_vsetvl_e8m1(vl);
}

template <> size_t rvv_vsetvl<vuint8m2_t>(size_t vl) {
  return __riscv_vsetvl_e8m2(vl);
}

template <> size_t rvv_vsetvl<vuint8m4_t>(size_t vl) {
  return __riscv_vsetvl_e8m4(vl);
}

template <> size_t rvv_vsetvl<vuint8m8_t>(size_t vl) {
  return __riscv_vsetvl_e8m8(vl);
}

// ------------------------------------------------------------

template <typename T> T rvv_splat(typename vector_element<T>::type, size_t);

template <> vuint32m1_t rvv_splat(uint32_t v, size_t vl) {
  return __riscv_vmv_v_x_u32m1(v, vl);
}

template <> vuint32m2_t rvv_splat(uint32_t v, size_t vl) {
  return __riscv_vmv_v_x_u32m2(v, vl);
}

template <> vuint32m4_t rvv_splat(uint32_t v, size_t vl) {
  return __riscv_vmv_v_x_u32m4(v, vl);
}

template <> vuint32m8_t rvv_splat(uint32_t v, size_t vl) {
  return __riscv_vmv_v_x_u32m8(v, vl);
}

// ------------------------------------------------------------

template <typename Source, typename Destination>
Destination rvv_reinterpret(Source);

template <> vuint32m1_t rvv_reinterpret<vuint8m1_t, vuint32m1_t>(vuint8m1_t x) {
  return __riscv_vreinterpret_v_u8m1_u32m1(x);
}

template <> vuint32m2_t rvv_reinterpret<vuint8m2_t, vuint32m2_t>(vuint8m2_t x) {
  return __riscv_vreinterpret_v_u8m2_u32m2(x);
}

template <> vuint32m4_t rvv_reinterpret<vuint8m4_t, vuint32m4_t>(vuint8m4_t x) {
  return __riscv_vreinterpret_v_u8m4_u32m4(x);
}

template <> vuint32m8_t rvv_reinterpret<vuint8m8_t, vuint32m8_t>(vuint8m8_t x) {
  return __riscv_vreinterpret_v_u8m8_u32m8(x);
}

template <>
vuint32m1_t rvv_reinterpret<vuint16m1_t, vuint32m1_t>(vuint16m1_t x) {
  return __riscv_vreinterpret_v_u16m1_u32m1(x);
}

template <>
vuint32m2_t rvv_reinterpret<vuint16m2_t, vuint32m2_t>(vuint16m2_t x) {
  return __riscv_vreinterpret_v_u16m2_u32m2(x);
}

template <>
vuint32m4_t rvv_reinterpret<vuint16m4_t, vuint32m4_t>(vuint16m4_t x) {
  return __riscv_vreinterpret_v_u16m4_u32m4(x);
}

template <>
vuint32m8_t rvv_reinterpret<vuint16m8_t, vuint32m8_t>(vuint16m8_t x) {
  return __riscv_vreinterpret_v_u16m8_u32m8(x);
}

template <> vuint8m1_t rvv_reinterpret<vuint16m1_t, vuint8m1_t>(vuint16m1_t x) {
  return __riscv_vreinterpret_v_u16m1_u8m1(x);
}

template <> vuint8m2_t rvv_reinterpret<vuint16m2_t, vuint8m2_t>(vuint16m2_t x) {
  return __riscv_vreinterpret_v_u16m2_u8m2(x);
}

template <> vuint8m4_t rvv_reinterpret<vuint16m4_t, vuint8m4_t>(vuint16m4_t x) {
  return __riscv_vreinterpret_v_u16m4_u8m4(x);
}

template <> vuint8m8_t rvv_reinterpret<vuint16m8_t, vuint8m8_t>(vuint16m8_t x) {
  return __riscv_vreinterpret_v_u16m8_u8m8(x);
}

template <>
vuint16m1_t rvv_reinterpret<vuint32m1_t, vuint16m1_t>(vuint32m1_t x) {
  return __riscv_vreinterpret_v_u32m1_u16m1(x);
}

template <>
vuint16m2_t rvv_reinterpret<vuint32m2_t, vuint16m2_t>(vuint32m2_t x) {
  return __riscv_vreinterpret_v_u32m2_u16m2(x);
}

template <>
vuint16m4_t rvv_reinterpret<vuint32m4_t, vuint16m4_t>(vuint32m4_t x) {
  return __riscv_vreinterpret_v_u32m4_u16m4(x);
}

template <>
vuint16m8_t rvv_reinterpret<vuint32m8_t, vuint16m8_t>(vuint32m8_t x) {
  return __riscv_vreinterpret_v_u32m8_u16m8(x);
}
