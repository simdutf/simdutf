#ifndef RESULT_H
#define RESULT_H
namespace simdutf {

struct result {
  bool is_valid;
  size_t length;

  simdutf_really_inline result();

  simdutf_really_inline result(bool _is_valid, size_t _length);

  simdutf_really_inline void invalidate();

  simdutf_really_inline result& operator++();
  simdutf_really_inline result operator+(const result other) const;
  simdutf_really_inline result& operator+=(const result other);
};

}
#endif