#pragma once
#include "benchmark_base.h"
#include "cmdline.h"
#include "simdutf.h"

/**
 * inoue2008 is:
 * Hiroshi Inoue and Hideaki Komatsu and Toshio Nakatani,
 * Accelerating UTF-8 Decoding Using SIMD Instructions (in Japanese),
 * Information Processing Society of Japan Transactions on Programming 1 (2), 2008.
 */
#include "benchmarks/competition/inoue2008/inoue_utf8_to_utf16.h"

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
#ifdef INOUE2008
        /**
         * Hiroshi Inoue and Hideaki Komatsu and Toshio Nakatani,
         * Accelerating UTF-8 Decoding Using SIMD Instructions (in Japanese),
         * Information Processing Society of Japan Transactions on Programming 1 (2), 2008.
         */
        void run_convert_valid_utf8_to_utf16_inoue2008(size_t iterations);
#endif
#ifdef __x86_64__
        /**
         * benchmarks/competition/u8u16 contains an open source version of u8u16, referenced in
         * Cameron, Robert D, A case study in SIMD text processing with parallel bit streams: UTF-8 to UTF-16 transcoding,
         * Proceedings of the 13th ACM SIGPLAN Symposium on Principles and practice of parallel programming, 91--98.
         */
        void run_convert_utf8_to_utf16_u8u16(size_t iterations);
        /**
         * Olivier Goffart, UTF-8 processing using SIMD (SSE4), 2012.
         * https://woboq.com/blog/utf-8-processing-using-simd.html
         */
        void run_convert_utf8_to_utf16_utf8sse4(size_t iterations);
#endif
        void run_convert_utf8_to_utf16_hoehrmann(size_t iterations);
        /**
         * LLVM relies on code from the Unicode Consortium
         * https://en.wikipedia.org/wiki/Unicode_Consortium
         */
        void run_convert_utf8_to_utf16_llvm(size_t iterations);
        void run_convert_utf16_to_utf8_llvm(size_t iterations);

    };

} // namespace simdutf::benchmarks
