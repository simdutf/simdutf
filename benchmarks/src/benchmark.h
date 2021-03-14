#pragma once

#include "benchmark_base.h"
#include "cmdline.h"
#include "simdutf.h"

namespace simdutf::benchmarks {

    class Benchmark : public BenchmarkBase {
    public:
        using BenchmarkBase::run;

        Benchmark(std::vector<input::Testcase>&& testcases);

        static Benchmark create(const CommandLine& cmdline);
        virtual const std::set<std::string>& all_procedures() const override;

    protected:
        virtual void run(const std::string& procedure_name, size_t iterations) override;

    private:
        std::set<std::string> known_procedures;

    private:
        void run_validate_utf8(const simdutf::implementation& implementation, size_t iterations);
        void run_convert_utf8_to_utf16(const simdutf::implementation& implementation, size_t iterations);
        void run_convert_valid_utf8_to_utf16(const simdutf::implementation& implementation, size_t iterations);
        void run_convert_utf16_to_utf8(const simdutf::implementation& implementation, size_t iterations);
        void run_convert_valid_utf16_to_utf8(const simdutf::implementation& implementation, size_t iterations);
    };

} // namespace simdutf::benchmarks
