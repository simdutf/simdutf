namespace simdutf {

  simdutf_really_inline result::result() : error{error_code::SUCCESS}, length{0} {};

  simdutf_really_inline result::result(error_code _error, size_t _length) : error{_error}, length{_length} {};

  simdutf_really_inline void result::set(error_code _error, size_t _length) {
    this->error = _error;
    this->length = _length;
  }

}