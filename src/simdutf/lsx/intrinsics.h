#ifndef SIMDUTF_LSX_INTRINSICS_H
#define SIMDUTF_LSX_INTRINSICS_H

#include "simdutf.h"

// This should be the correct header whether
// you use visual studio or other compilers.
#include <lsxintrin.h>

/*
Encoding of argument for LoongArch64 xvldi instruction.  See:
https://jia.je/unofficial-loongarch-intrinsics-guide/lasx/misc/#__m256i-__lasx_xvldi-imm_n1024_1023-imm

1: imm[12:8]=0b10000: broadcast imm[7:0] as 32-bit elements to all lanes

2: imm[12:8]=0b10001: broadcast imm[7:0] << 8 as 32-bit elements to all lanes

3: imm[12:8]=0b10010: broadcast imm[7:0] << 16 as 32-bit elements to all lanes

4: imm[12:8]=0b10011: broadcast imm[7:0] << 24 as 32-bit elements to all lanes

5: imm[12:8]=0b10100: broadcast imm[7:0] as 16-bit elements to all lanes

6: imm[12:8]=0b10101: broadcast imm[7:0] << 8 as 16-bit elements to all lanes

7: imm[12:8]=0b10110: broadcast (imm[7:0] << 8) | 0xFF as 32-bit elements to all
lanes

8: imm[12:8]=0b10111: broadcast (imm[7:0] << 16) | 0xFFFF as 32-bit elements to
all lanes

9: imm[12:8]=0b11000: broadcast imm[7:0] as 8-bit elements to all lanes

10: imm[12:8]=0b11001: repeat each bit of imm[7:0] eight times, and broadcast
the result as 64-bit elements to all lanes
*/

namespace vldi {

template <uint16_t v> class const_u16 {
  constexpr static const uint8_t b0 = ((v >> 0 * 8) & 0xff);
  constexpr static const uint8_t b1 = ((v >> 1 * 8) & 0xff);

  constexpr static bool is_case5 = uint16_t(b0) == v;
  constexpr static bool is_case6 = (uint16_t(b1) << 8) == v;
  constexpr static bool is_case9 = (b0 == b1);
  constexpr static bool is_case10 =
      ((b0 == 0xff) || (b0 == 0x00)) && ((b1 == 0xff) || (b1 == 0x00));

public:
  constexpr static uint16_t operation = is_case5    ? 0b10100
                                        : is_case6  ? 0b10101
                                        : is_case9  ? 0b11000
                                        : is_case10 ? 0x11001
                                                    : 0xffff;

  constexpr static uint16_t byte =
      is_case5    ? b0
      : is_case6  ? b1
      : is_case9  ? b0
      : is_case10 ? ((b0 ? 0x55 : 0x00) | (b1 ? 0xaa : 0x00))
                  : 0xffff;

  constexpr static int value = int((operation << 8) | byte) - 8192;
  constexpr static bool valid = operation != 0xffff;
};

template <uint32_t v> class const_u32 {
  constexpr static const uint8_t b0 = (v & 0xff);
  constexpr static const uint8_t b1 = ((v >> 8) & 0xff);
  constexpr static const uint8_t b2 = ((v >> 16) & 0xff);
  constexpr static const uint8_t b3 = ((v >> 24) & 0xff);

  constexpr static bool is_case1 = (uint32_t(b0) == v);
  constexpr static bool is_case2 = ((uint32_t(b1) << 8) == v);
  constexpr static bool is_case3 = ((uint32_t(b2) << 16) == v);
  constexpr static bool is_case4 = ((uint32_t(b3) << 24) == v);
  constexpr static bool is_case5 = (b0 == b2) && (b1 == 0) && (b3 == 0);
  constexpr static bool is_case6 = (b1 == b3) && (b0 == 0) && (b2 == 0);
  constexpr static bool is_case7 = (b3 == 0) && (b2 == 0) && (b0 == 0xff);
  constexpr static bool is_case8 = (b3 == 0) && (b1 == 0xff) && (b0 == 0xff);
  constexpr static bool is_case9 = (b0 == b1) && (b0 == b2) && (b0 == b3);
  constexpr static bool is_case10 =
      ((b0 == 0xff) || (b0 == 0x00)) && ((b1 == 0xff) || (b1 == 0x00)) &&
      ((b2 == 0xff) || (b2 == 0x00)) && ((b3 == 0xff) || (b3 == 0x00));

public:
  constexpr static uint16_t operation = is_case1    ? 0b10000
                                        : is_case2  ? 0b10001
                                        : is_case3  ? 0b10010
                                        : is_case4  ? 0b10011
                                        : is_case5  ? 0b10100
                                        : is_case6  ? 0b10101
                                        : is_case7  ? 0b10110
                                        : is_case8  ? 0b10111
                                        : is_case9  ? 0b11000
                                        : is_case10 ? 0b11001
                                                    : 0xffff;

  constexpr static uint16_t byte =
      is_case1    ? b0
      : is_case2  ? b1
      : is_case3  ? b2
      : is_case4  ? b3
      : is_case5  ? b0
      : is_case6  ? b1
      : is_case7  ? b1
      : is_case8  ? b2
      : is_case9  ? b0
      : is_case10 ? ((b0 ? 0x11 : 0x00) | (b1 ? 0x22 : 0x00) |
                     (b2 ? 0x44 : 0x00) | (b3 ? 0x88 : 0x00))
                  : 0xffff;

  constexpr static int value = int((operation << 8) | byte) - 8192;
  constexpr static bool valid = operation != 0xffff;
};

template <uint64_t v> class const_u64 {
  constexpr static const uint8_t b0 = ((v >> 0 * 8) & 0xff);
  constexpr static const uint8_t b1 = ((v >> 1 * 8) & 0xff);
  constexpr static const uint8_t b2 = ((v >> 2 * 8) & 0xff);
  constexpr static const uint8_t b3 = ((v >> 3 * 8) & 0xff);
  constexpr static const uint8_t b4 = ((v >> 4 * 8) & 0xff);
  constexpr static const uint8_t b5 = ((v >> 5 * 8) & 0xff);
  constexpr static const uint8_t b6 = ((v >> 6 * 8) & 0xff);
  constexpr static const uint8_t b7 = ((v >> 7 * 8) & 0xff);

  constexpr static bool is_case10 =
      ((b0 == 0xff) || (b0 == 0x00)) && ((b1 == 0xff) || (b1 == 0x00)) &&
      ((b2 == 0xff) || (b2 == 0x00)) && ((b3 == 0xff) || (b3 == 0x00)) &&
      ((b4 == 0xff) || (b4 == 0x00)) && ((b5 == 0xff) || (b5 == 0x00)) &&
      ((b6 == 0xff) || (b6 == 0x00)) && ((b7 == 0xff) || (b7 == 0x00));

public:
  constexpr static bool is_32bit =
      ((v & 0xffffffff) == (v >> 32)) && const_u32<(v >> 32)>::value;
  constexpr static uint8_t op_32bit = const_u32<(v >> 32)>::operation;
  constexpr static uint8_t byte_32bit = const_u32<(v >> 32)>::byte;

  constexpr static uint16_t operation = is_32bit    ? op_32bit
                                        : is_case10 ? 0x11001
                                                    : 0xffff;

  constexpr static uint16_t byte =
      is_32bit ? byte_32bit
      : is_case10
          ? ((b0 ? 0x01 : 0x00) | (b1 ? 0x02 : 0x00) | (b2 ? 0x04 : 0x00) |
             (b3 ? 0x08 : 0x00) | (b4 ? 0x10 : 0x00) | (b5 ? 0x20 : 0x00) |
             (b6 ? 0x40 : 0x00) | (b7 ? 0x80 : 0x00))
          : 0xffff;

  constexpr static int value = int((operation << 8) | byte) - 8192;
  constexpr static bool valid = operation != 0xffff;
};
} // namespace vldi

// Uncomment when running under QEMU affected
// by bug https://gitlab.com/qemu-project/qemu/-/issues/2865
// Versions <= 9.2.2 are affected, likely anything newer is correct.
#ifndef QEMU_VLDI_BUG
// #define QEMU_VLDI_BUG 1
#endif

#ifdef QEMU_VLDI_BUG
  #define lsx_splat_u16(v) __lsx_vreplgr2vr_h(v)
  #define lsx_splat_u32(v) __lsx_vreplgr2vr_w(v)
#else
template <uint16_t x> constexpr __m128i lsx_splat_u16_aux() {
  constexpr bool is_imm10 = (int16_t(x) < 512) && (int16_t(x) > -512);
  constexpr uint16_t imm10 = is_imm10 ? x : 0;
  constexpr bool is_vldi = vldi::const_u16<x>::valid;
  constexpr int vldi_imm = is_vldi ? vldi::const_u16<x>::value : 0;

  return is_imm10  ? __lsx_vrepli_h(int16_t(imm10))
         : is_vldi ? __lsx_vldi(vldi_imm)
                   : __lsx_vreplgr2vr_h(x);
}

template <uint32_t x> constexpr __m128i lsx_splat_u32_aux() {
  constexpr bool is_imm10 = (int32_t(x) < 512) && (int32_t(x) > -512);
  constexpr uint32_t imm10 = is_imm10 ? x : 0;
  constexpr bool is_vldi = vldi::const_u32<x>::valid;
  constexpr int vldi_imm = is_vldi ? vldi::const_u32<x>::value : 0;

  return is_imm10  ? __lsx_vrepli_w(int32_t(imm10))
         : is_vldi ? __lsx_vldi(vldi_imm)
                   : __lsx_vreplgr2vr_w(x);
}

  #define lsx_splat_u16(v) lsx_splat_u16_aux<(v)>()
  #define lsx_splat_u32(v) lsx_splat_u32_aux<(v)>()
#endif // QEMU_VLDI_BUG

#endif //  SIMDUTF_LSX_INTRINSICS_H
