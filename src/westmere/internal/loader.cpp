namespace internal {

using simdutf::internal::find;
using simdutf::internal::make_pair;
using simdutf::internal::memcpy;
using simdutf::internal::memmove;
using simdutf::internal::memset;
using simdutf::internal::min_value;
using simdutf::internal::pair;
using simdutf::internal::ptrdiff_t;

namespace westmere {

#include "westmere/internal/write_v_u16_11bits_to_utf8.cpp"

} // namespace westmere
} // namespace internal
