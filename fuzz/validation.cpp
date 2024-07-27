#include "simdutf.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

constexpr bool allow_implementations_to_differ = false;

std::vector<const simdutf::implementation *> get_implementations()
{
    std::vector<const simdutf::implementation *> ret;
    for (auto e : simdutf::get_available_implementations()) {
        std::cerr << "implementation " << e->name() << " is available? "
                  << e->supported_by_runtime_system() << '\n';
#if 0
        if (e->name() == "icelake")
            continue;
#endif
        if (e->supported_by_runtime_system()) {
            ret.push_back(e);
        }
    }
    return ret;
}

template<typename T, bool bigendian = false>
int testit(const uint8_t *databytes, size_t size_in_bytes)
{
    const auto *data = reinterpret_cast<const T *>(databytes);
    const auto size = size_in_bytes / sizeof(T);

    static const auto implementations = get_implementations();
    std::vector<simdutf::result> results;
    results.reserve(implementations.size());

    for (auto e : implementations) {
        simdutf::result result{};
        if constexpr (std::is_same_v<T, char8_t>) {
            result = e->validate_utf8_with_errors(reinterpret_cast<const char *>(data), size);
        }
        if constexpr (std::is_same_v<T, char16_t> && bigendian) {
            result = e->validate_utf16be_with_errors(data, size);
        }
        if constexpr (std::is_same_v<T, char16_t> && !bigendian) {
            result = e->validate_utf16le_with_errors(data, size);
        }
        if constexpr (std::is_same_v<T, char32_t>) {
            result = e->validate_utf32_with_errors(data, size);
        }
        results.push_back(result);
    }
    auto ne = [](const simdutf::result &a, const simdutf::result &b) -> bool {
        return !(a.count == b.count && a.error == b.error);
    };
    if (std::ranges::adjacent_find(results, ne) != results.end()) {
        std::cerr << "fuzz case " << __PRETTY_FUNCTION__ << ":\n";
        for (std::size_t i = 0; i < results.size(); ++i) {
            std::cerr << "got error " << results.at(i).error << " and count " << results.at(i).count
                      << " from implementation " << implementations.at(i)->name() << '\n';
        }
        if (!allow_implementations_to_differ) {
            std::abort();
        }
    }
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // the first byte is which action to take. step forward
    // several bytes so the input is aligned.
    if (size < 4) {
        return 0;
    }
    const auto action = data[0];
    data += 4;
    size -= 4;

    switch (action & 0b11) {
    case 0:
        // utf8
        return testit<char8_t>(data, size);
    case 1:
        // utf16 big endian
        return testit<char16_t, true>(data, size);
    case 2:
        // utf16 little endian
        return testit<char16_t, false>(data, size);
    case 3:
        // utf32
        return testit<char32_t>(data, size);
    default:
        return 0;
    }
}
//int main(){};
