#ifndef SIMDUTF_H
#define SIMDUTF_H
#include <cstring>
#include <vector>


namespace temporary {

std::size_t validate_utf16le_with_errors(
    const char16_t * buf, std::size_t len) noexcept;

std::vector<char16_t> get_test_data();
}


#endif // SIMDUTF_H
