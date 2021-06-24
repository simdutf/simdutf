#include "random_int.h"

#include <stdexcept>

namespace simdutf::tests::helpers {

    RandomInt::RandomInt(uint64_t lo, uint64_t hi, uint64_t seed)
      : distr{lo, hi}
      , gen(std::mt19937::result_type(seed)) {}

    uint64_t RandomInt::operator()() {
      return distr(gen);
    }

    RandomIntRanges::RandomIntRanges(std::initializer_list<std::pair<uint64_t, uint64_t>> ranges_, uint64_t seed)
      : gen(std::mt19937::result_type(seed)) {

      if (ranges_.size() == 0)
        throw std::invalid_argument("Ranges must not be empty");

      for (const auto [lo, hi]: ranges_)
        ranges.emplace_back(lo, hi);

      range_index = Distribution(0, ranges.size() - 1);
    }

    uint32_t RandomIntRanges::operator()() {
      const auto index = range_index(gen);
      return ranges[index](gen);
    }
}
