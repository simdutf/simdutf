#pragma once

#include "benchmark_base.h"
#include "cmdline.h"

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
    };

} // namespace simdutf::benchmarks
