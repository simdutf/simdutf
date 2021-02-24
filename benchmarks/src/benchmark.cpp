#include "benchmark.h"
#include "simdutf.h"

#include <cassert>
#include <array>

namespace simdutf::benchmarks {

Benchmark::Benchmark(std::vector<input::Testcase>&& testcases)
    : BenchmarkBase(std::move(testcases)) {

    std::array<std::string, 2> implemented_functions{"validate_utf8", "convert_valid_utf8_to_utf16"};

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
    printf("%s, input size: %lu, iterations: %lu, \n",
           procedure_name.c_str(), input_data.size(), iterations);

    const size_t p = procedure_name.find('+');
    assert(p != std::string::npos);

    const std::string name{procedure_name.substr(0, p)};
    const std::string impl{procedure_name.substr(p + 1)};

    auto implementation = simdutf::available_implementations[impl];
    if (implementation == nullptr) {
        throw std::runtime_error("Wrong implementation " + impl);
    }

    if (name == "validate_utf8") {
        const char*  data = reinterpret_cast<const char*>(input_data.data());
        const size_t size = input_data.size();
        volatile bool sink{false};

        auto proc = [implementation, data, size, &sink]() {
            sink = implementation->validate_utf8(data, size);
        };

        const auto result = count_events(proc, iterations);
        print_summary(result, size);
    } else if(name == "convert_valid_utf8_to_utf16") {
        const char*  data = reinterpret_cast<const char*>(input_data.data());
        const size_t size = input_data.size();
        std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
        volatile size_t sink{0};

        auto proc = [implementation, data, size, &output_buffer, &sink]() {
            sink = implementation->convert_valid_utf8_to_utf16(data, size, output_buffer.get());
        };

        const auto result = count_events(proc, iterations);
        print_summary(result, size);
    } else {
        std::cerr << "Unsupported procedure: " << name << std::endl;
        std::cerr << "Report the issue." << std::endl;
        std::cerr << " Aborting ! " << std::endl;
        abort();
    }
}

const std::set<std::string>& Benchmark::all_procedures() const {
    return known_procedures;
}

} // namespace
