#ifndef SIMDUTF_INTERNAL_STL_COMPAT_H
#define SIMDUTF_INTERNAL_STL_COMPAT_H

#include "simdutf/common_defs.h"

#ifndef SIMDUTF_NO_LIBCXX
  #include <algorithm>
  #include <array>
  #include <cstddef>
  #include <cstring>
  #include <iterator>
  #if SIMDUTF_CPLUSPLUS20
    #include <span>
  #endif
  #include <string>
  #include <tuple>
  #include <type_traits>
  #include <utility>
#else
  #include <stddef.h>
  #include <string.h>
#endif

namespace simdutf {
namespace internal {

//--------------------------------------------------------------------
// Pair
//--------------------------------------------------------------------
#ifndef SIMDUTF_NO_LIBCXX
template <typename First, typename Second>
using pair = std::pair<First, Second>;

using std::make_pair;

template <size_t Index, typename First, typename Second>
using pair_get_impl = std::tuple_element<Index, pair<First, Second>>;
#else
template <typename First, typename Second> struct pair {
  First first;
  Second second;
};

template <size_t Index, typename First, typename Second> struct pair_get_impl {
  static_assert(Index < 2, "pair index out of bounds");
  using type = First;
};

template <typename First, typename Second>
struct pair_get_impl<0, First, Second> {
  using type = First;

  simdutf_really_inline static constexpr type &
  get(pair<First, Second> &value) noexcept {
    return value.first;
  }

  simdutf_really_inline static constexpr const type &
  get(const pair<First, Second> &value) noexcept {
    return value.first;
  }
};

template <typename First, typename Second>
struct pair_get_impl<1, First, Second> {
  using type = Second;

  simdutf_really_inline static constexpr type &
  get(pair<First, Second> &value) noexcept {
    return value.second;
  }

  simdutf_really_inline static constexpr const type &
  get(const pair<First, Second> &value) noexcept {
    return value.second;
  }
};

template <typename First, typename Second>
simdutf_really_inline constexpr pair<First, Second>
make_pair(First first, Second second) noexcept {
  return {first, second};
}

template <size_t Index, typename First, typename Second>
simdutf_really_inline constexpr
    typename pair_get_impl<Index, First, Second>::type &
    get(pair<First, Second> &value) noexcept {
  return pair_get_impl<Index, First, Second>::get(value);
}

template <size_t Index, typename First, typename Second>
simdutf_really_inline constexpr const typename pair_get_impl<Index, First,
                                                             Second>::type &
get(const pair<First, Second> &value) noexcept {
  return pair_get_impl<Index, First, Second>::get(value);
}
#endif

//--------------------------------------------------------------------
// Tuple
//--------------------------------------------------------------------
#ifndef SIMDUTF_NO_LIBCXX
using std::get;

template <typename First, typename Second, typename Third>
using tuple = std::tuple<First, Second, Third>;

using std::make_tuple;

template <size_t Index, typename First, typename Second, typename Third>
using tuple_get_impl = std::tuple_element<Index, tuple<First, Second, Third>>;
#else
template <typename First, typename Second, typename Third> struct tuple {
  First first;
  Second second;
  Third third;
};

template <size_t Index, typename First, typename Second, typename Third>
struct tuple_get_impl {
  static_assert(Index < 3, "tuple index out of bounds");
  using type = First;
};

template <typename First, typename Second, typename Third>
struct tuple_get_impl<0, First, Second, Third> {
  using type = First;

  simdutf_really_inline static constexpr type &
  get(tuple<First, Second, Third> &value) noexcept {
    return value.first;
  }

  simdutf_really_inline static constexpr const type &
  get(const tuple<First, Second, Third> &value) noexcept {
    return value.first;
  }
};

template <typename First, typename Second, typename Third>
struct tuple_get_impl<1, First, Second, Third> {
  using type = Second;

  simdutf_really_inline static constexpr type &
  get(tuple<First, Second, Third> &value) noexcept {
    return value.second;
  }

  simdutf_really_inline static constexpr const type &
  get(const tuple<First, Second, Third> &value) noexcept {
    return value.second;
  }
};

template <typename First, typename Second, typename Third>
struct tuple_get_impl<2, First, Second, Third> {
  using type = Third;

  simdutf_really_inline static constexpr type &
  get(tuple<First, Second, Third> &value) noexcept {
    return value.third;
  }

  simdutf_really_inline static constexpr const type &
  get(const tuple<First, Second, Third> &value) noexcept {
    return value.third;
  }
};

template <typename First, typename Second, typename Third>
simdutf_really_inline constexpr tuple<First, Second, Third>
make_tuple(First first, Second second, Third third) noexcept {
  return {first, second, third};
}

template <size_t Index, typename First, typename Second, typename Third>
simdutf_really_inline constexpr
    typename tuple_get_impl<Index, First, Second, Third>::type &
    get(tuple<First, Second, Third> &value) noexcept {
  return tuple_get_impl<Index, First, Second, Third>::get(value);
}

template <size_t Index, typename First, typename Second, typename Third>
simdutf_really_inline constexpr const typename tuple_get_impl<
    Index, First, Second, Third>::type &
get(const tuple<First, Second, Third> &value) noexcept {
  return tuple_get_impl<Index, First, Second, Third>::get(value);
}
#endif

//--------------------------------------------------------------------
// Array
//--------------------------------------------------------------------
#ifndef SIMDUTF_NO_LIBCXX
template <typename T, size_t N> using array = std::array<T, N>;
#else
template <typename T, size_t N> struct array {
  // Keep zero-sized instantiations well-formed without changing size().
  T storage[N == 0 ? 1 : N];

  simdutf_really_inline T *data() noexcept { return storage; }
  simdutf_really_inline constexpr const T *data() const noexcept {
    return storage;
  }
  simdutf_really_inline constexpr size_t size() const noexcept { return N; }
  simdutf_really_inline T *begin() noexcept { return data(); }
  simdutf_really_inline constexpr const T *begin() const noexcept {
    return data();
  }
  simdutf_really_inline T *end() noexcept { return data() + N; }
  simdutf_really_inline constexpr const T *end() const noexcept {
    return data() + N;
  }
  simdutf_really_inline T &operator[](size_t index) noexcept {
    return storage[index];
  }
  simdutf_really_inline constexpr const T &
  operator[](size_t index) const noexcept {
    return storage[index];
  }
};
#endif

//--------------------------------------------------------------------
// Types and Traits
//--------------------------------------------------------------------
#ifndef SIMDUTF_NO_LIBCXX
using ptrdiff_t = std::ptrdiff_t;
template <typename T, typename U> using is_same = std::is_same<T, U>;
template <typename T> using remove_reference = std::remove_reference<T>;
template <typename T> using remove_const = std::remove_const<T>;
template <typename T> using remove_volatile = std::remove_volatile<T>;
template <typename T> using remove_cv = std::remove_cv<T>;
template <typename T> using decay = std::decay<T>;
#else
using ptrdiff_t = ::ptrdiff_t;

template <typename T, typename U> struct is_same {
  static constexpr bool value = false;
};

template <typename T> struct is_same<T, T> {
  static constexpr bool value = true;
};

template <typename T> struct remove_reference {
  using type = T;
};

template <typename T> struct remove_reference<T &> {
  using type = T;
};

template <typename T> struct remove_reference<T &&> {
  using type = T;
};

template <typename T> struct remove_const {
  using type = T;
};

template <typename T> struct remove_const<const T> {
  using type = T;
};

template <typename T> struct remove_volatile {
  using type = T;
};

template <typename T> struct remove_volatile<volatile T> {
  using type = T;
};

template <typename T> struct remove_cv {
  using type = typename remove_const<typename remove_volatile<T>::type>::type;
};

template <typename T> struct decay {
  using type = typename remove_cv<typename remove_reference<T>::type>::type;
};
#endif

template <typename T> using decay_t = typename decay<T>::type;

//--------------------------------------------------------------------
// String Helpers
//--------------------------------------------------------------------

simdutf_really_inline constexpr const char *c_str(const char *value) noexcept {
  return value;
}

#ifndef SIMDUTF_NO_LIBCXX
simdutf_really_inline const char *c_str(const std::string &value) noexcept {
  return value.c_str();
}
#endif

//--------------------------------------------------------------------
// Span
//--------------------------------------------------------------------
#if !defined(SIMDUTF_NO_LIBCXX) && SIMDUTF_CPLUSPLUS20
template <typename T> using span = std::span<T>;
#else
template <typename T> class span {
public:
  using element_type = T;
  using value_type = typename remove_cv<T>::type;
  using pointer = T *;
  using reference = T &;
  using iterator = pointer;

  simdutf_really_inline constexpr span() noexcept : data_(nullptr), size_(0) {}
  simdutf_really_inline constexpr span(pointer data, size_t size) noexcept
      : data_(data), size_(size) {}

  simdutf_really_inline constexpr pointer data() const noexcept {
    return data_;
  }
  simdutf_really_inline constexpr size_t size() const noexcept { return size_; }
  simdutf_really_inline constexpr iterator begin() const noexcept {
    return data_;
  }
  simdutf_really_inline constexpr iterator end() const noexcept {
    return data_ + size_;
  }
  simdutf_really_inline constexpr reference
  operator[](size_t index) const noexcept {
    return data_[index];
  }

private:
  pointer data_;
  size_t size_;
};
#endif

//--------------------------------------------------------------------
// Iterator Helpers
//--------------------------------------------------------------------
template <typename Iterator>
simdutf_really_inline constexpr ptrdiff_t distance(Iterator first,
                                                   Iterator last) noexcept {
#ifndef SIMDUTF_NO_LIBCXX
  return std::distance(first, last);
#else
  return last - first;
#endif
}

//--------------------------------------------------------------------
// Algorithm Helpers
//--------------------------------------------------------------------
template <typename T>
simdutf_really_inline simdutf_constexpr T min_value(T a, T b) noexcept {
#if !defined(SIMDUTF_NO_LIBCXX) && SIMDUTF_CPLUSPLUS14
  return (std::min)(a, b);
#else
  return b < a ? b : a;
#endif
}

template <typename T>
simdutf_really_inline T *find(T *first, T *last, const T &value) noexcept {
#ifndef SIMDUTF_NO_LIBCXX
  return std::find(first, last, value);
#else
  while (first != last) {
    if (*first == value) {
      return first;
    }
    ++first;
  }
  return last;
#endif
}

template <typename T>
simdutf_really_inline const T *find(const T *first, const T *last,
                                    const T &value) noexcept {
#ifndef SIMDUTF_NO_LIBCXX
  return std::find(first, last, value);
#else
  while (first != last) {
    if (*first == value) {
      return first;
    }
    ++first;
  }
  return last;
#endif
}

//--------------------------------------------------------------------
// Memory Helpers
//--------------------------------------------------------------------
simdutf_really_inline void *memcpy(void *destination, const void *source,
                                   size_t count) noexcept {
#ifndef SIMDUTF_NO_LIBCXX
  return std::memcpy(destination, source, count);
#else
  return ::memcpy(destination, source, count);
#endif
}

simdutf_really_inline void *memmove(void *destination, const void *source,
                                    size_t count) noexcept {
#ifndef SIMDUTF_NO_LIBCXX
  return std::memmove(destination, source, count);
#else
  return ::memmove(destination, source, count);
#endif
}

simdutf_really_inline void *memset(void *destination, int ch,
                                   size_t count) noexcept {
#ifndef SIMDUTF_NO_LIBCXX
  return std::memset(destination, ch, count);
#else
  return ::memset(destination, ch, count);
#endif
}

} // namespace internal
} // namespace simdutf

#endif // SIMDUTF_INTERNAL_STL_COMPAT_H
