namespace simdutf {

  simdutf_really_inline result::result() : error{error_code::SUCCESS}, count{0} {};

  simdutf_really_inline result::result(error_code _err, size_t _pos) : error{_err}, count{_pos} {};

}