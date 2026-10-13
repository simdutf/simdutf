#define SIMDUTF_IMPLEMENTATION icelake

#if SIMDUTF_CAN_ALWAYS_RUN_ICELAKE
// nothing needed.
#else
SIMDUTF_TARGET_ICELAKE
#endif

#if SIMDUTF_GCC11ORMORE // workaround for
                        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105593
// clang-format off
SIMDUTF_DISABLE_GCC_WARNING(-Wmaybe-uninitialized)
// clang-format on
#endif // end of workaround

// GCC (since GCC 10, with -fipa-ra) may omit vzeroupper before calling the
// scalar routines, which are compiled for the baseline target and use legacy
// SSE instructions. Calling them with dirty upper YMM/ZMM state is very slow
// on some Intel processors, so we clear the upper state explicitly.
#undef SIMDUTF_VZEROUPPER
#define SIMDUTF_VZEROUPPER() _mm256_zeroupper()
