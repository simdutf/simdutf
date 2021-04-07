#include "benchmark_base.h"
#include "tests/helpers/random_utf8.h"
#include "simdutf.h"
#include <fstream>

namespace simdutf::benchmarks {

    BenchmarkBase::BenchmarkBase(std::vector<input::Testcase>&& testcases)
        : testcases{std::move(testcases)} {}

    bool BenchmarkBase::run() {
        printf("testcases: %lu\n", testcases.size());
        for (const auto& testcase: testcases) {
            run(testcase);
        }

        return true;
    }

    void BenchmarkBase::run(const input::Testcase& testcase) {
        prepare_input(testcase);
        auto detected_encoding = simdutf::autodetect_encoding(input_data.data(), input_data.size());
        printf("input detected as %s\n", simdutf::to_string(detected_encoding).c_str());
        printf("current system detected as %s\n", active_implementation->name().c_str());
        printf("===========================\n");

        const auto& known_procedures = all_procedures();

        if (testcase.tested_procedures.empty()) {
            for (const std::string& procedure: known_procedures) {
                // We will first identify the input.
                auto expected_input_encoding = expected_encodings(procedure);
                const bool is_in = expected_input_encoding.find(detected_encoding) != expected_input_encoding.end();
                if(is_in) { run(procedure, testcase); }
            }
        } else {
            std::set<std::string> to_be_tested;
            for (const auto& procedure: testcase.tested_procedures) {
                for(const auto& candidate: known_procedures) {
                    if(candidate.find(procedure) != std::string::npos) {
                        to_be_tested.insert(candidate);
                    }
                }
            }

            for (const auto& procedure: to_be_tested) {
                run(procedure, testcase);
            }
        }
    }

    void BenchmarkBase::run(const std::string& procedure_name, const input::Testcase& testcase) {
        printf("%s, input size: %lu, iterations: %lu",
           procedure_name.c_str(), input_data.size(), testcase.iterations);

        if (std::holds_alternative<input::File>(testcase.input)) {
            const input::File& file{std::get<input::File>(testcase.input)};
            printf(", dataset: %s\n", file.path.c_str());
        } else
            putchar('\n');

        run(procedure_name, testcase.iterations);
    }

    void BenchmarkBase::prepare_input(const input::Testcase& testcase) {
        if (std::holds_alternative<input::File>(testcase.input)) {
            const input::File& file{std::get<input::File>(testcase.input)};
            load_file(file.path);
        } else {
            const input::random_utf8& random{std::get<input::random_utf8>(testcase.input)};

            simdutf::tests::helpers::random_utf8 gen_1_2_3_4(rd,
                random.utf_1byte_prob,
                random.utf_2bytes_prob,
                random.utf_3bytes_prob,
                random.utf_4bytes_prob);

            input_data = gen_1_2_3_4.generate(random.size);
        }
    }

    void BenchmarkBase::load_file(const std::filesystem::path& path) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(path);

        input_data.assign(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());
    }

    void BenchmarkBase::print_summary(const event_aggregate& all, double data_size) const {
        const double gbs = data_size / all.best.elapsed_ns();
        const double gbs_avs = data_size / (all.total.elapsed_ns()/all.iterations);
        const double error_margin = (gbs-gbs_avs)/gbs_avs * 100;

        if (all.has_events) {
            const double _1GHz = 1'000'000'000.0;
            const double freq = (all.best.cycles() / all.best.elapsed_sec()) / _1GHz;
            const double insperunit = all.best.instructions() / data_size;
            const double inspercycle = all.best.instructions() / all.best.cycles();
            const double cmisperunit = all.best.cache_misses() / data_size;
            const double bmisperunit = all.best.branch_misses() / data_size;

            printf("%8.3f ins/byte, %8.3f GHz, %8.3f GB/s (%.1f %%), %8.3f ins/cycle, %g b.misses/byte, %g c.mis/byte \n", insperunit, freq, gbs, error_margin, inspercycle, bmisperunit, cmisperunit);
        } else {
            printf("%8.3f GB/s (%.1f %%)\n", gbs, error_margin);
        }
        if(error_margin>10) { printf("WARNING: Measurements are noisy, try increasing iteration count (-I).\n"); }
    }
}
