#ifndef SIMDUTF_ICELAKE_BITMANIPULATION_H
#define SIMDUTF_ICELAKE_BITMANIPULATION_H

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
simdutf_really_inline unsigned __int64 count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num);// Visual Studio wants two underscores
}
#else
simdutf_really_inline long long int count_ones(uint64_t input_num) {
  return _popcnt64(input_num);
}
#endif

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#endif // SIMDUTF_ICELAKE_BITMANIPULATION_H
