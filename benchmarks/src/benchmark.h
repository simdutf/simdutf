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
        virtual std::set<simdutf::encoding_type> expected_encodings(const std::string& procedure) override;

    protected:
        virtual void run(const std::string& procedure_name, size_t iterations) override;

    private:
        std::set<std::string> known_procedures;
        std::map<std::string,std::set<simdutf::encoding_type>> expected_input_encoding;

    private:
        void run_validate_utf8(const simdutf::implementation& implementation, size_t iterations);
        void run_validate_utf16(const simdutf::implementation& implementation, size_t iterations);
        void run_count_utf8(const simdutf::implementation& implementation, size_t iterations);
        void run_count_utf16(const simdutf::implementation& implementation, size_t iterations);
        void run_convert_utf8_to_utf16(const simdutf::implementation& implementation, size_t iterations);
        void run_convert_valid_utf8_to_utf16(const simdutf::implementation& implementation, size_t iterations);
        void run_convert_utf16_to_utf8(const simdutf::implementation& implementation, size_t iterations);
        void run_convert_valid_utf16_to_utf8(const simdutf::implementation& implementation, size_t iterations);
    };

} // namespace simdutf::benchmarks
