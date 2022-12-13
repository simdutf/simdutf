#define SIMDUTF_IMPLEMENTATION icelake
SIMDUTF_TARGET_ICELAKE

#if SIMDUTF_GCC11ORMORE // workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105593
SIMDUTF_DISABLE_GCC_WARNING(-Wmaybe-uninitialized)
#endif // end of workaround