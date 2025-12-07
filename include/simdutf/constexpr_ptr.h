#ifndef SIMDUTF_CONSTEXPR_PTR_H
#define SIMDUTF_CONSTEXPR_PTR_H

#include <cstddef>
#include <bit>

namespace simdutf {
namespace detail {
/**
 * The constexpr_ptr class is a workaround for reinterpret_cast not being
 * allowed during constant evaluation.
 */
template <typename to, typename from>
  requires(sizeof(to) == sizeof(from))
struct constexpr_ptr {
  const from *p;

  constexpr constexpr_ptr() noexcept = default;

  constexpr explicit constexpr_ptr(const from *ptr) noexcept : p(ptr) {}

  constexpr to operator*() const noexcept { return std::bit_cast<to>(*p); }

  constexpr constexpr_ptr &operator++() noexcept {
    ++p;
    return *this;
  }

  constexpr constexpr_ptr operator++(int) noexcept {
    auto old = *this;
    ++p;
    return old;
  }

  constexpr constexpr_ptr &operator--() noexcept {
    --p;
    return *this;
  }

  constexpr constexpr_ptr operator--(int) noexcept {
    auto old = *this;
    --p;
    return old;
  }

  constexpr constexpr_ptr &operator+=(std::ptrdiff_t n) noexcept {
    p += n;
    return *this;
  }

  constexpr constexpr_ptr &operator-=(std::ptrdiff_t n) noexcept {
    p -= n;
    return *this;
  }

  constexpr constexpr_ptr operator+(std::ptrdiff_t n) const noexcept {
    return constexpr_ptr{p + n};
  }

  constexpr constexpr_ptr operator-(std::ptrdiff_t n) const noexcept {
    return constexpr_ptr{p - n};
  }

  constexpr std::ptrdiff_t operator-(const constexpr_ptr &o) const noexcept {
    return p - o.p;
  }

  constexpr to operator[](std::ptrdiff_t n) const noexcept {
    return std::bit_cast<to>(*(p + n));
  }

  //  constexpr auto operator<=>(const constexpr_ptr &) const noexcept =
  //  default;

  // for memcpy to work
  constexpr operator const void *() const noexcept { return p; }
};

template <typename to, typename from>
constexpr constexpr_ptr<to, from> constexpr_cast_ptr(from *p) noexcept {
  return constexpr_ptr<to, from>{p};
}

} // namespace detail
} // namespace simdutf
#endif
