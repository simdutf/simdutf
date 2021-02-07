#include "benchmark.h"
#include "simdutf.h"

#include <array>

namespace simdutf::benchmarks {

Benchmark::Benchmark(std::vector<input::Testcase>&& testcases)
    : BenchmarkBase(std::move(testcases)) {

    std::array<std::string, 1> implemented_functions{"validatate_utf8"};

    for (const auto& implementation: simdutf::available_implementations) {
        for (const auto& function: implemented_functions) {
            std::string name = function + "+" + implementation->name();
            known_procedures.insert(std::move(name));
        }
    }
}

//static
Benchmark Benchmark::create(const CommandLine& cmdline) {
    std::vector<input::Testcase> testcases;

    using input::Testcase;
    using input::File;
    using input::RandomUTF8;

    for (const size_t iterations: cmdline.iterations) {
        for (const auto& path: cmdline.files) {
            testcases.emplace_back(Testcase{cmdline.procedures, iterations, File{path}});
        }

        for (const size_t size: cmdline.random_size) {
            testcases.emplace_back(Testcase{cmdline.procedures, iterations, RandomUTF8{size}});
        }
    }

    return Benchmark{std::move(testcases)};
}

void Benchmark::run(const std::string& procedure_name, size_t iterations) {
    printf("will run: %s, %lu iteration(s)\n", procedure_name.c_str(), iterations);
}

const std::set<std::string>& Benchmark::all_procedures() const {
    return known_procedures;
}

} // namespace
