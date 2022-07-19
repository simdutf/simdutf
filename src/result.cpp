namespace simdutf {

  simdutf_really_inline result::result() : is_valid{true}, length{0} {};

  simdutf_really_inline result::result(bool _is_valid, size_t _length) : is_valid{_is_valid}, length{_length} {};

  simdutf_really_inline void result::invalidate() {
    is_valid = false;
    length = 0;
  }

  simdutf_really_inline result& result::operator++() {
    (this->length)++;
    return *this;
  }

  simdutf_really_inline result result::operator+(const result other) const {
    if (this->is_valid && other.is_valid) {
      return result(true, this->length + other.length);
    } else {
      return result(false, 0);
    }
  }

  simdutf_really_inline result& result::operator+=(const result other) {
    auto this_cast = static_cast<result*>(this);
    *this_cast = *this_cast + other;
    return *this_cast;
  }

}