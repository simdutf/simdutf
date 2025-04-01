#ifndef SIMDUTF_LASX_INTRINSICS_H
#define SIMDUTF_LASX_INTRINSICS_H

#include "simdutf.h"

// This should be the correct header whether
// you use visual studio or other compilers.
#include <lsxintrin.h>
#include <lasxintrin.h>

#if defined(__loongarch_asx)
  #ifdef __clang__
    #define VREGS_PREFIX "$vr"
    #define XREGS_PREFIX "$xr"
  #else // GCC
    #define VREGS_PREFIX "$f"
    #define XREGS_PREFIX "$f"
  #endif
  #define __ALL_REGS                                                           \
    "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,"  \
    "27,28,29,30,31"
// Convert __m128i to __m256i
static inline __m256i ____m256i(__m128i in) {
  __m256i out = __lasx_xvldi(0);
  __asm__ volatile(".irp i," __ALL_REGS "\n\t"
                   " .ifc %[out], " XREGS_PREFIX "\\i    \n\t"
                   "  .irp j," __ALL_REGS "\n\t"
                   "   .ifc %[in], " VREGS_PREFIX "\\j  \n\t"
                   "    xvpermi.q $xr\\i, $xr\\j, 0x0  \n\t"
                   "   .endif                           \n\t"
                   "  .endr                             \n\t"
                   " .endif                             \n\t"
                   ".endr                               \n\t"
                   : [out] "+f"(out)
                   : [in] "f"(in));
  return out;
}
// Convert two __m128i to __m256i
static inline __m256i lasx_set_q(__m128i inhi, __m128i inlo) {
  __m256i out;
  __asm__ volatile(".irp i," __ALL_REGS "\n\t"
                   " .ifc %[hi], " VREGS_PREFIX "\\i    \n\t"
                   "  .irp j," __ALL_REGS "\n\t"
                   "   .ifc %[lo], " VREGS_PREFIX "\\j  \n\t"
                   "    xvpermi.q $xr\\i, $xr\\j, 0x20  \n\t"
                   "   .endif                           \n\t"
                   "  .endr                             \n\t"
                   " .endif                             \n\t"
                   ".endr                               \n\t"
                   ".ifnc %[out], %[hi]                 \n\t"
                   ".irp i," __ALL_REGS "\n\t"
                   " .ifc %[out], " XREGS_PREFIX "\\i   \n\t"
                   "  .irp j," __ALL_REGS "\n\t"
                   "   .ifc %[hi], " VREGS_PREFIX "\\j  \n\t"
                   "    xvori.b $xr\\i, $xr\\j, 0       \n\t"
                   "   .endif                           \n\t"
                   "  .endr                             \n\t"
                   " .endif                             \n\t"
                   ".endr                               \n\t"
                   ".endif                              \n\t"
                   : [out] "=f"(out), [hi] "+f"(inhi)
                   : [lo] "f"(inlo));
  return out;
}
// Convert __m256i low part to __m128i
static inline __m128i lasx_extracti128_lo(__m256i in) {
  __m128i out;
  __asm__ volatile(".ifnc %[out], %[in]                 \n\t"
                   ".irp i," __ALL_REGS "\n\t"
                   " .ifc %[out], " VREGS_PREFIX "\\i   \n\t"
                   "  .irp j," __ALL_REGS "\n\t"
                   "   .ifc %[in], " XREGS_PREFIX "\\j  \n\t"
                   "    vori.b $vr\\i, $vr\\j, 0        \n\t"
                   "   .endif                           \n\t"
                   "  .endr                             \n\t"
                   " .endif                             \n\t"
                   ".endr                               \n\t"
                   ".endif                              \n\t"
                   : [out] "=f"(out)
                   : [in] "f"(in));
  return out;
}
// Convert __m256i high part to __m128i
static inline __m128i lasx_extracti128_hi(__m256i in) {
  __m128i out;
  __asm__ volatile(".irp i," __ALL_REGS "\n\t"
                   " .ifc %[out], " VREGS_PREFIX "\\i   \n\t"
                   "  .irp j," __ALL_REGS "\n\t"
                   "   .ifc %[in], " XREGS_PREFIX "\\j  \n\t"
                   "    xvpermi.q $xr\\i, $xr\\j, 0x11  \n\t"
                   "   .endif                           \n\t"
                   "  .endr                             \n\t"
                   " .endif                             \n\t"
                   ".endr                               \n\t"
                   : [out] "=f"(out)
                   : [in] "f"(in));
  return out;
}
#endif

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

namespace lasx_vldi {

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

} // namespace lasx_vldi

// Uncomment when running under QEMU affected
// by bug https://gitlab.com/qemu-project/qemu/-/issues/2865
// Versions <= 9.2.2 are affected, likely anything newer is correct.
#ifndef QEMU_VLDI_BUG
// #define QEMU_VLDI_BUG 1
#endif

#ifdef QEMU_VLDI_BUG
  #define lasx_splat_u16(v) __lasx_xvreplgr2vr_h(v)
  #define lasx_splat_u32(v) __lasx_xvreplgr2vr_w(v)
#else
template <uint16_t x> constexpr __m256i lasx_splat_u16_aux() {
  constexpr bool is_imm10 = (int16_t(x) < 512) && (int16_t(x) > -512);
  constexpr uint16_t imm10 = is_imm10 ? x : 0;
  constexpr bool is_vldi = lasx_vldi::const_u16<x>::valid;
  constexpr int vldi_imm = is_vldi ? lasx_vldi::const_u16<x>::value : 0;

  return is_imm10  ? __lasx_xvrepli_h(int16_t(imm10))
         : is_vldi ? __lasx_xvldi(vldi_imm)
                   : __lasx_xvreplgr2vr_h(x);
}

template <uint32_t x> constexpr __m256i lasx_splat_u32_aux() {
  constexpr bool is_imm10 = (int32_t(x) < 512) && (int32_t(x) > -512);
  constexpr uint32_t imm10 = is_imm10 ? x : 0;
  constexpr bool is_vldi = lasx_vldi::const_u32<x>::valid;
  constexpr int vldi_imm = is_vldi ? lasx_vldi::const_u32<x>::value : 0;

  return is_imm10  ? __lasx_xvrepli_w(int32_t(imm10))
         : is_vldi ? __lasx_xvldi(vldi_imm)
                   : __lasx_xvreplgr2vr_w(x);
}

  #define lasx_splat_u16(v) lasx_splat_u16_aux<(v)>()
  #define lasx_splat_u32(v) lasx_splat_u32_aux<(v)>()
#endif // QEMU_VLDI_BUG

#endif //  SIMDUTF_LASX_INTRINSICS_H
