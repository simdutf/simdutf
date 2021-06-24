#include "random_int.h"

#include <stdexcept>

namespace simdutf::tests::helpers {

    RandomInt::RandomInt(uint64_t lo, uint64_t hi, uint64_t seed) noexcept
      : distr{lo, hi}
      , gen(std::mt19937::result_type(seed)) {}

    uint32_t RandomInt::operator()() noexcept {
      return uint32_t(distr(gen));
    }

    RandomIntRanges::RandomIntRanges(std::initializer_list<std::pair<uint64_t, uint64_t>> ranges_, uint64_t seed) noexcept
      : gen(std::mt19937::result_type(seed)) {

      for (const auto [lo, hi]: ranges_)
        ranges.emplace_back(lo, hi);

      range_index = Distribution(0, ranges.size() - 1);
    }

    uint32_t RandomIntRanges::operator()() noexcept {
      const size_t index = size_t(range_index(gen));
      return uint32_t(ranges[index](gen));
    }
}
