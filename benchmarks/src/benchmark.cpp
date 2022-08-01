#include "benchmark.h"
#include "simdutf.h"

#include <cassert>
#include <array>
#include <iostream>
#ifdef __x86_64__
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
SIMDUTF_TARGET_WESTMERE
namespace {
#include "benchmarks/competition/utf8lut/src/utf8lut.h"
}
SIMDUTF_UNTARGET_REGION


/**
 * Bob Steagall, CppCon2018
 * https://github.com/BobSteagall/CppCon2018/
 *
 * Fast Conversion From UTF-8 with C++, DFAs, and SSE Intrinsics
 * https://www.youtube.com/watch?v=5FQ87-Ecb-A
 */
#include "benchmarks/competition/CppCon2018/utf_utils.cpp"
#endif

/**
 * Bjoern Hoehrmann
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */
#include "benchmarks/competition/hoehrmann/hoehrmann.h"
/**
 * LLVM relies on code from the Unicode Consortium
 * https://en.wikipedia.org/wiki/Unicode_Consortium
 */
#include "benchmarks/competition/llvm/ConvertUTF.cpp"
#ifdef __x86_64__
/**
 * Olivier Goffart, UTF-8 processing using SIMD (SSE4), 2012.
 * https://woboq.com/blog/utf-8-processing-using-simd.html
 */
#include "benchmarks/competition/utf8sse4/fromutf8-sse.cpp"
#endif

#ifdef __x86_64__
/**
 * benchmarks/competition/u8u16 contains an open source version of u8u16, referenced in
 * Cameron, Robert D, A case study in SIMD text processing with parallel bit streams: UTF-8 to UTF-16 transcoding,
 * Proceedings of the 13th ACM SIGPLAN Symposium on Principles and practice of parallel programming, 91--98.
 */
// It seems that u8u16 is not good at scoping macros.
#undef LITTLE_ENDIAN
#undef BYTE_ORDER
#undef BIG_ENDIAN
#include "benchmarks/competition/u8u16/config/p4_config.h"
#include "benchmarks/competition/u8u16/src/libu8u16.c"
#endif

namespace simdutf::benchmarks {

Benchmark::Benchmark(std::vector<input::Testcase>&& testcases)
    : BenchmarkBase(std::move(testcases)) {

    // the std::set<simdutf::encoding_type> value represents the *expected* encoding.
    std::vector<std::pair<std::string,std::set<simdutf::encoding_type>>> implemented_functions{
        {"validate_utf8", {simdutf::encoding_type::UTF8}},
        {"validate_utf8_with_errors", {simdutf::encoding_type::UTF8}},
        {"validate_utf16", {simdutf::encoding_type::UTF16_LE}},
        {"validate_utf16le_with_errors", {simdutf::encoding_type::UTF16_LE}},
        {"validate_utf32", {simdutf::encoding_type::UTF32_LE}},
        {"validate_utf32_with_errors", {simdutf::encoding_type::UTF32_LE}},
        
        {"count_utf8", {simdutf::encoding_type::UTF8}},
        {"count_utf16", {simdutf::encoding_type::UTF16_LE}},
        
        {"convert_utf8_to_utf16", {simdutf::encoding_type::UTF8}},
        {"convert_utf8_to_utf16_with_errors", {simdutf::encoding_type::UTF8}},
        {"convert_utf8_to_utf16_with_dynamic_allocation", {simdutf::encoding_type::UTF8}},
        {"convert_valid_utf8_to_utf16", {simdutf::encoding_type::UTF8}},
        
        {"convert_utf8_to_utf32", {simdutf::encoding_type::UTF8}},
        {"convert_utf8_to_utf32_with_errors", {simdutf::encoding_type::UTF8}},
        {"convert_utf8_to_utf32_with_dynamic_allocation", {simdutf::encoding_type::UTF8}},
        {"convert_valid_utf8_to_utf32", {simdutf::encoding_type::UTF8}},
        
        {"convert_utf16_to_utf8", {simdutf::encoding_type::UTF16_LE}},
        {"convert_utf16_to_utf8_with_errors", {simdutf::encoding_type::UTF16_LE}},
        {"convert_utf16_to_utf8_with_dynamic_allocation", {simdutf::encoding_type::UTF16_LE}},
        {"convert_valid_utf16_to_utf8", {simdutf::encoding_type::UTF16_LE}},

        
        {"convert_utf16_to_utf32", {simdutf::encoding_type::UTF16_LE}},
        {"convert_utf16_to_utf32_with_errors", {simdutf::encoding_type::UTF16_LE}},
        {"convert_utf16_to_utf32_with_dynamic_allocation", {simdutf::encoding_type::UTF16_LE}},
        {"convert_valid_utf16_to_utf32", {simdutf::encoding_type::UTF16_LE}},

        {"convert_utf32_to_utf8", {simdutf::encoding_type::UTF32_LE}},
        {"convert_utf32_to_utf8_with_errors", {simdutf::encoding_type::UTF32_LE}},
        {"convert_valid_utf32_to_utf8", {simdutf::encoding_type::UTF32_LE}},

        {"convert_utf32_to_utf16", {simdutf::encoding_type::UTF32_LE}},
        {"convert_utf32_to_utf16_with_errors", {simdutf::encoding_type::UTF32_LE}},
        {"convert_valid_utf32_to_utf16", {simdutf::encoding_type::UTF32_LE}},

    };

    for (const auto& implementation: simdutf::available_implementations) {
        for (const auto& function: implemented_functions) {
            std::string name = function.first + "+" + implementation->name();
            known_procedures.insert(name);
            expected_input_encoding.insert(make_pair(name,function.second));
        }
    }
#ifdef ICU_AVAILABLE
    std::cout << "Using ICU version " << U_ICU_VERSION << std::endl;
    {
        std::string name = "convert_utf8_to_utf16+icu";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf16_to_utf8+icu";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF16_LE})));
    }
#endif
#ifdef INOUE2008
    {
        std::string name = "convert_valid_utf8_to_utf16+inoue2008";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
#endif
#ifdef __x86_64__
    {
        std::string name = "convert_utf8_to_utf16+u8u16";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf16_to_utf8+utf8lut";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF16_LE})));
    }
    {
        std::string name = "convert_valid_utf16_to_utf8+utf8lut";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF16_LE})));
    }
    {
        std::string name = "convert_utf8_to_utf16+utf8lut";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf8_to_utf32+utf8lut";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_valid_utf8_to_utf16+utf8lut";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
		{
        std::string name = "convert_utf32_to_utf8+utf8lut";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF32_LE})));
    }
    {
        std::string name = "convert_valid_utf32_to_utf8+utf8lut";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF32_BE})));
    }
    {
        std::string name = "convert_valid_utf8_to_utf32+utf8lut";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf8_to_utf16+utf8sse4";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf8_to_utf16+cppcon2018";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf8_to_utf32+cppcon2018";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
#endif
    {
        std::string name = "convert_utf8_to_utf16+hoehrmann";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf8_to_utf32+hoehrmann";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf8_to_utf16+llvm";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf8_to_utf32+llvm";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF8})));
    }
    {
        std::string name = "convert_utf16_to_utf8+llvm";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF16_LE})));
    }
    {
        std::string name = "convert_utf32_to_utf8+llvm";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF32_LE})));
    }
    {
        std::string name = "convert_utf32_to_utf16+llvm";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF32_LE})));
    }
    {
        std::string name = "convert_utf16_to_utf32+llvm";
        known_procedures.insert(name);
        expected_input_encoding.insert(std::make_pair(name,std::set<simdutf::encoding_type>({simdutf::encoding_type::UTF16_LE})));
    }
}

//static
Benchmark Benchmark::create(const CommandLine& cmdline) {
    std::vector<input::Testcase> testcases;

    using input::Testcase;
    using input::File;
    using input::random_utf8;

    for (const size_t iterations: cmdline.iterations) {
        for (const auto& path: cmdline.files) {
            testcases.emplace_back(Testcase{cmdline.procedures, iterations, File{path}});
        }

        for (const size_t size: cmdline.random_size) {
            testcases.emplace_back(Testcase{cmdline.procedures, iterations, random_utf8{size}});
        }
    }

    return Benchmark{std::move(testcases)};
}

void Benchmark::run(const std::string& procedure_name, size_t iterations) {
    const size_t p = procedure_name.find('+');
    assert(p != std::string::npos);

    const std::string name{procedure_name.substr(0, p)};
    const std::string impl{procedure_name.substr(p + 1)};
#ifdef INOUE2008
    if(impl == "inoue2008") {
        // this is a special case
        run_convert_valid_utf8_to_utf16_inoue2008(iterations);
        return;
    }
#endif
#ifdef ICU_AVAILABLE
    if(impl == "icu") {
        if(name == "convert_utf8_to_utf16") {
          run_convert_utf8_to_utf16_icu(iterations);
        } else if(name == "convert_utf16_to_utf8") {
          run_convert_utf16_to_utf8_icu(iterations);
        }
        return;
    }
#endif
#ifdef __x86_64__
    if(impl == "cppcon2018") {
        // this is a special case
        if(name == "convert_utf8_to_utf16") {
          run_convert_utf8_to_utf16_cppcon2018(iterations);
        } else if(name == "convert_utf8_to_utf32") {
          run_convert_utf8_to_utf16_cppcon2018(iterations);
        } else {
          std::cerr << "unrecognized:" << procedure_name << "\n";
        }
        return;
    }
    if(impl == "u8u16") {
        // this is a special case
        if(name == "convert_utf8_to_utf16") {
          run_convert_utf8_to_utf16_u8u16(iterations);
        } else {
          std::cerr << "unrecognized:" << procedure_name << "\n";
        }
        return;
    }
    if(impl == "utf8sse4") {
        // this is a special case
        if(name == "convert_utf8_to_utf16") {
          run_convert_utf8_to_utf16_utf8sse4(iterations);
        } else {
          std::cerr << "unrecognized:" << procedure_name << "\n";
        }
        return;
    }
    if(impl == "utf8lut") {
        // this is a special case
        if(name == "convert_valid_utf8_to_utf16") {
          run_convert_valid_utf8_to_utf16_utf8lut(iterations);
        } else if(name == "convert_valid_utf8_to_utf32") {
          run_convert_valid_utf8_to_utf32_utf8lut(iterations);
        } else if(name == "convert_utf8_to_utf16") {
          run_convert_utf8_to_utf16_utf8lut(iterations);
        } else if(name == "convert_utf8_to_utf32") {
          run_convert_utf8_to_utf32_utf8lut(iterations);
        } else if(name == "convert_utf16_to_utf8") {
          run_convert_utf16_to_utf8_utf8lut(iterations);
        } else if(name == "convert_valid_utf16_to_utf8") {
          run_convert_valid_utf16_to_utf8_utf8lut(iterations);
		} else if(name == "convert_utf32_to_utf8") {
          run_convert_utf32_to_utf8_utf8lut(iterations);
        } else if(name == "convert_valid_utf32_to_utf8") {
          run_convert_valid_utf32_to_utf8_utf8lut(iterations);
        } else {
          std::cerr << "unrecognized:" << procedure_name << "\n";
        }
        return;
    }
#endif
    if(impl == "hoehrmann") {
        // this is a special case
        if(name == "convert_utf8_to_utf16") {
          run_convert_utf8_to_utf16_hoehrmann(iterations);
        }
        if(name == "convert_utf8_to_utf32") {
          run_convert_utf8_to_utf32_hoehrmann(iterations);
        } else {
          std::cerr << "unrecognized:" << procedure_name << "\n";
        }
        return;
    }
    if(impl == "llvm") {
        // this is a special case
        if(name == "convert_utf8_to_utf16") {
          run_convert_utf8_to_utf16_llvm(iterations);
        } else if(name == "convert_utf8_to_utf32") {
          run_convert_utf8_to_utf32_llvm(iterations);
        } else if(name == "convert_utf16_to_utf8") {
          run_convert_utf16_to_utf8_llvm(iterations);
        } else if(name == "convert_utf32_to_utf8") {
        	run_convert_utf32_to_utf8_llvm(iterations);
        } else if(name == "convert_utf32_to_utf16") {
          run_convert_utf32_to_utf16_llvm(iterations);
        } else if(name == "convert_utf16_to_utf32") {
          run_convert_utf16_to_utf32_llvm(iterations);
        }  else {
          std::cerr << "unrecognized:" << procedure_name << "\n";
        }
        return;
    }
    auto implementation = simdutf::available_implementations[impl];
    if (implementation == nullptr) {
        throw std::runtime_error("Wrong implementation " + impl);
    }

    if (name == "validate_utf8") {
        run_validate_utf8(*implementation, iterations);
    } else if (name == "validate_utf8_with_errors") {
        run_validate_utf8_with_errors(*implementation, iterations);
    } else if (name == "validate_utf16") {
        run_validate_utf16(*implementation, iterations);
    } else if (name == "validate_utf16le_with_errors") {
        run_validate_utf16le_with_errors(*implementation, iterations);
    } else if(name == "validate_utf32") {
        run_validate_utf32(*implementation, iterations);
    } else if(name == "validate_utf32_with_errors") {
        run_validate_utf32_with_errors(*implementation, iterations);
    } else if(name == "count_utf8") {
        run_count_utf8(*implementation, iterations);
    } else if(name == "count_utf16") {
        run_count_utf16(*implementation, iterations);
    } else if(name == "convert_utf8_to_utf16") {
        run_convert_utf8_to_utf16(*implementation, iterations);
    } else if(name == "convert_utf8_to_utf16_with_errors") {
        run_convert_utf8_to_utf16_with_errors(*implementation, iterations);
    } else if(name == "convert_utf8_to_utf32") {
        run_convert_utf8_to_utf32(*implementation, iterations);
    } else if(name == "convert_utf8_to_utf32_with_errors") {
        run_convert_utf8_to_utf32_with_errors(*implementation, iterations);
    } else if(name == "convert_utf8_to_utf16_with_dynamic_allocation") {
        run_convert_utf8_to_utf16_with_dynamic_allocation(*implementation, iterations);
    } else if(name == "convert_utf8_to_utf32_with_dynamic_allocation") {
        run_convert_utf8_to_utf32_with_dynamic_allocation(*implementation, iterations);
    } else if(name == "convert_valid_utf8_to_utf16") {
        run_convert_valid_utf8_to_utf16(*implementation, iterations);
    } else if(name == "convert_valid_utf8_to_utf32") {
        run_convert_valid_utf8_to_utf32(*implementation, iterations);
    } else if(name == "convert_utf16_to_utf8") {
        run_convert_utf16_to_utf8(*implementation, iterations);
    } else if(name == "convert_utf16_to_utf8_with_errors") {
        run_convert_utf16_to_utf8_with_errors(*implementation, iterations);
    } else if(name == "convert_utf16_to_utf32") {
        run_convert_utf16_to_utf32(*implementation, iterations);
    } else if(name == "convert_utf16_to_utf32_with_errors") {
        run_convert_utf16_to_utf32_with_errors(*implementation, iterations);
    } else if(name == "convert_utf16_to_utf8_with_dynamic_allocation") {
        run_convert_utf16_to_utf8_with_dynamic_allocation(*implementation, iterations);
    } else if(name == "convert_utf16_to_utf32_with_dynamic_allocation") {
        run_convert_utf16_to_utf32_with_dynamic_allocation(*implementation, iterations);
    } else if(name == "convert_valid_utf16_to_utf8") {
        run_convert_valid_utf16_to_utf8(*implementation, iterations);
    } else if(name == "convert_utf32_to_utf8") {
        run_convert_utf32_to_utf8(*implementation, iterations);
    } else if(name == "convert_utf32_to_utf8_with_errors") {
        run_convert_utf32_to_utf8_with_errors(*implementation, iterations);
    } else if(name == "convert_valid_utf32_to_utf8") {
        run_convert_valid_utf32_to_utf8(*implementation, iterations);
    } else if(name == "convert_utf32_to_utf16") {
        run_convert_utf32_to_utf16(*implementation, iterations);
    } else if(name == "convert_utf32_to_utf16_with_errors") {
        run_convert_utf32_to_utf16_with_errors(*implementation, iterations);
    } else if(name == "convert_valid_utf32_to_utf16") {
        run_convert_valid_utf32_to_utf16(*implementation, iterations);
    } else if(name == "convert_valid_utf16_to_utf32") {
        run_convert_valid_utf16_to_utf32(*implementation, iterations);
    } else {
        std::cerr << "Unsupported procedure: " << name << '\n';
        std::cerr << "Report the issue.\n";
        std::cerr << " Aborting ! " << std::endl;
        abort();
    }
}

void Benchmark::run_validate_utf8(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    volatile bool sink{false};

    auto proc = [&implementation, data, size, &sink]() {
        sink = implementation.validate_utf8(data, size);
    };

    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_validate_utf8_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    volatile bool sink{false};

    auto proc = [&implementation, data, size, &sink]() {
        result res = implementation.validate_utf8_with_errors(data, size);
        sink = !(res.error);
    };

    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}


void Benchmark::run_validate_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
        printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &sink]() {
        sink = implementation.validate_utf16le(data, size);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_validate_utf16le_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
        printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &sink]() {
        result res = implementation.validate_utf16le_with_errors(data, size);
        sink = !(res.error);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_validate_utf32(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
        printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &sink]() {
        sink = implementation.validate_utf32(data, size);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = size;
    print_summary(result, size, char_count);
}

void Benchmark::run_validate_utf32_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
        printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &sink]() {
        result res = implementation.validate_utf32_with_errors(data, size);
        sink = !(res.error);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = size;
    print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_utf8_to_utf16le(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf16_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile bool sink{false};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        result res = implementation.convert_utf8_to_utf16le_with_errors(data, size, output_buffer.get());
        sink = !(res.error);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf32(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_utf8_to_utf32(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf32_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
    volatile bool sink{false};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        result res = implementation.convert_utf8_to_utf32_with_errors(data, size, output_buffer.get());
        sink = !(res.error);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf8_to_utf16_with_dynamic_allocation(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    volatile size_t sink{0};
    auto proc = [&implementation, data, size, &sink]() {
        auto dyn_size = implementation.utf16_length_from_utf8(data, size);
        std::unique_ptr<char16_t[]> output_buffer{new char16_t[dyn_size]};
        sink = implementation.convert_utf8_to_utf16le(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}


void Benchmark::run_convert_utf8_to_utf32_with_dynamic_allocation(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    volatile size_t sink{0};
    auto proc = [&implementation, data, size, &sink]() {
        auto dyn_size = implementation.utf32_length_from_utf8(data, size);
        std::unique_ptr<char32_t[]> output_buffer{new char32_t[dyn_size]};
        sink = implementation.convert_utf8_to_utf32(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

#ifdef ICU_AVAILABLE
void Benchmark::run_convert_utf8_to_utf16_icu(size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    volatile size_t sink{0};
    auto proc = [data, size,  &sink]() {
        auto str = U_ICU_NAMESPACE::UnicodeString::fromUTF8(std::string_view(data, size));
        sink = str.length();
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    // checking
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    size_t expected = convert_utf8_to_utf16le(data, size, output_buffer.get());
    if(expected != sink) { std::cerr << "The number of UTF-16 words does not match.\n"; }
    print_summary(result, size, char_count);
}
void Benchmark::run_convert_utf16_to_utf8_icu(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }
    size /= 2;
    volatile size_t sink{0};

    auto proc = [data, size, &sink]() {
        U_ICU_NAMESPACE::UnicodeString str(data, size);
        std::string out;
        out = str.toUTF8String(out);
        sink = out.size();
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}
#endif

#ifdef INOUE2008
void Benchmark::run_convert_valid_utf8_to_utf16_inoue2008(size_t iterations) {
    // Inoue2008 is only up to 3-byte UTF8 sequence.
    for(uint8_t c : input_data) {
        if(c>=0b11110000) {
            std::cerr << "Warning: Inoue 2008 does not support 4-byte inputs!" << std::endl;
            break;
        }
    }
    // This is currently minimally tested. Itt is possible that the transcoding could be wrong.
    // It is also unsafe: it could fail in disastrous ways if the input is adversarial.
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
        sink = inoue2008::convert_valid(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}
#endif
/**
 * Bjoern Hoehrmann
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */
void Benchmark::run_convert_utf8_to_utf16_hoehrmann(size_t iterations) {
    uint8_t const *  data = input_data.data();
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
        sink = hoehrmann::toUtf16(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf8(reinterpret_cast<const char*>(data), size);
    print_summary(result, size, char_count);
}
/**
 * Bjoern Hoehrmann
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */
void Benchmark::run_convert_utf8_to_utf32_hoehrmann(size_t iterations) {
    uint8_t const *  data = input_data.data();
    const size_t size = input_data.size();
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
        sink = hoehrmann::toUtf32(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf8(reinterpret_cast<const char*>(data), size);
    print_summary(result, size, char_count);
}

#ifdef __x86_64__
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_utf16_to_utf8_utf8lut(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: non-surrogate words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    // utf8lut requires an extra 16 bytes of padding.
    std::unique_ptr<char[]> output_buffer{new char[size * 4 + 16]};

    volatile size_t sink{0};

    auto proc = [data, size, &output_buffer, &sink]() {
      std::unique_ptr<BaseBufferProcessor> processor(ProcessorSelector<dfUtf16,dfUtf8>::WithOptions<cmValidate>::Create());
      ConversionResult result = ConvertInMemory(*processor, reinterpret_cast<const char*>(data), 2*size, reinterpret_cast<char*>(output_buffer.get()), size * 4 + 16);
      if(result.status != 0) {
          sink = 0;
      } else {
          sink = result.outputSize;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_valid_utf16_to_utf8_utf8lut(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: non-surrogate words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    // utf8lut requires an extra 16 bytes of padding.
    std::unique_ptr<char[]> output_buffer{new char[size * 4 + 16]};

    volatile size_t sink{0};

    auto proc = [data, size, &output_buffer, &sink]() {
      std::unique_ptr<BaseBufferProcessor> processor(ProcessorSelector<dfUtf16,dfUtf8>::WithOptions<cmFull>::Create());
      ConversionResult result = ConvertInMemory(*processor, reinterpret_cast<const char*>(data), 2*size, reinterpret_cast<char*>(output_buffer.get()), size * 4 + 16);
      if(result.status != 0) {
          sink = 0;
      } else {
          sink = result.outputSize;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_utf8_to_utf16_utf8lut(size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    // utf8lut requires an extra 8 bytes of padding.
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size*2+8]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
      std::unique_ptr<BaseBufferProcessor> processor(ProcessorSelector<dfUtf8, dfUtf16>::WithOptions<cmValidate>::Create());
      ConversionResult result = ConvertInMemory(*processor, data, size, reinterpret_cast<char*>(output_buffer.get()), size*2+16);
      if(result.status != 0) {
          sink = 0;
      } else {
          sink = result.outputSize / 2;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_utf8_to_utf32_utf8lut(size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();

    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size+4]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
      std::unique_ptr<BaseBufferProcessor> processor(ProcessorSelector<dfUtf8, dfUtf32>::WithOptions<cmValidate>::Create());
      ConversionResult result = ConvertInMemory(*processor, data, size, reinterpret_cast<char*>(output_buffer.get()), size*4+16);
      if(result.status != 0) {
          sink = 0;
      } else {
          sink = result.outputSize / 2;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_valid_utf8_to_utf16_utf8lut(size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    // utf8lut requires an extra 8 bytes of padding.
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size*2+8]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
      std::unique_ptr<BaseBufferProcessor> processor(ProcessorSelector<dfUtf8, dfUtf16>::WithOptions<cmFull>::Create());
      ConversionResult result = ConvertInMemory(*processor, data, size, reinterpret_cast<char*>(output_buffer.get()), size*2+16);
      if(result.status != 0) {
          sink = 0;
      } else {
          sink = result.outputSize / 2;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_utf32_to_utf8_utf8lut(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by four (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: a single 32-bit word can yield up to four UTF-8 bytes. We are
    //       making a safe assumption that each 32-bit word will yield four
    //       UTF-8 bytes.
    // utf8lut requires an extra 16 bytes of padding.
    std::unique_ptr<char[]> output_buffer{new char[size * 4 + 16]};

    volatile size_t sink{0};

    auto proc = [data, size, &output_buffer, &sink]() {
      std::unique_ptr<BaseBufferProcessor> processor(ProcessorSelector<dfUtf32,dfUtf8>::WithOptions<cmValidate>::Create());
      ConversionResult result = ConvertInMemory(*processor, reinterpret_cast<const char*>(data), 4*size, reinterpret_cast<char*>(output_buffer.get()), size * 4 + 16);
      if(result.status != 0) {
          sink = 0;
      } else {
          sink = result.outputSize;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}

/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_valid_utf8_to_utf32_utf8lut(size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();

    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size+4]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
      std::unique_ptr<BaseBufferProcessor> processor(ProcessorSelector<dfUtf8, dfUtf32>::WithOptions<cmFull>::Create());
      ConversionResult result = ConvertInMemory(*processor, data, size, reinterpret_cast<char*>(output_buffer.get()), size*4+16);
      if(result.status != 0) {
          sink = 0;
      } else {
          sink = result.outputSize / 2;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
/**
 * utf8lut: Vectorized UTF-8 converter.
 * by stgatilov (2019)
 *  https://dirtyhandscoding.github.io/posts/utf8lut-vectorized-utf-8-converter-introduction.html
 */
void Benchmark::run_convert_valid_utf32_to_utf8_utf8lut(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by four (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: a single 32-bit word can yield up to four UTF-8 bytes. We are
    //       making a safe assumption that each 32-bit word will yield four
    //       UTF-8 bytes.
    // utf8lut requires an extra 16 bytes of padding.
    std::unique_ptr<char[]> output_buffer{new char[size * 4 + 16]};

    volatile size_t sink{0};

    auto proc = [data, size, &output_buffer, &sink]() {
      std::unique_ptr<BaseBufferProcessor> processor(ProcessorSelector<dfUtf32,dfUtf8>::WithOptions<cmFull>::Create());
      ConversionResult result = ConvertInMemory(*processor, reinterpret_cast<const char*>(data), 4*size, reinterpret_cast<char*>(output_buffer.get()), size * 4 + 16);
      if(result.status != 0) {
          sink = 0;
      } else {
          sink = result.outputSize;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}
/**
 * Bob Steagall, CppCon2018
 * https://github.com/BobSteagall/CppCon2018/
 *
 * Fast Conversion From UTF-8 with C++, DFAs, and SSE Intrinsics
 * https://www.youtube.com/watch?v=5FQ87-Ecb-A
 */
void Benchmark::run_convert_utf8_to_utf16_cppcon2018(size_t iterations) {
    using char8_t   = unsigned char;
    const char8_t*  data = reinterpret_cast<const char8_t*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
      sink = uu::UtfUtils::SseConvert(data, data + size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(reinterpret_cast<const char*>(data), size);
    print_summary(result, size, char_count);
}
/**
 * Bob Steagall, CppCon2018
 * https://github.com/BobSteagall/CppCon2018/
 *
 * Fast Conversion From UTF-8 with C++, DFAs, and SSE Intrinsics
 * https://www.youtube.com/watch?v=5FQ87-Ecb-A
 */
void Benchmark::run_convert_utf8_to_utf32_cppcon2018(size_t iterations) {
    using char8_t   = unsigned char;
    const char8_t*  data = reinterpret_cast<const char8_t*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
      sink = uu::UtfUtils::SseConvert(data, data + size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(reinterpret_cast<const char*>(data), size);
    print_summary(result, size, char_count);
}
/**
 * Cameron, Robert D, A case study in SIMD text processing with parallel bit streams: UTF-8 to UTF-16 transcoding,
 * Proceedings of the 13th ACM SIGPLAN Symposium on Principles and practice of parallel programming, 91--98.
 */
void Benchmark::run_convert_utf8_to_utf16_u8u16(size_t iterations) {
    // u8u16 wants to take mutable chars, let us hope it does not actually mutate anything!
    //
    // This is currently untested. At a glance it looks fine, but
    // it is possible that the transcoding could be wrong.
    char*  data = reinterpret_cast<char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
        char * srcbuf_ptr = data;
        size_t inbytes_left = size;
        char * trgtbuf_ptr = reinterpret_cast<char *>(output_buffer.get());
        size_t outbytes_left = size * sizeof(char16_t);
        size_t result_code = u8u16(&srcbuf_ptr, &inbytes_left, &trgtbuf_ptr, &outbytes_left);
        bool is_ok = (result_code != size_t(-1));
        if(is_ok) {
          sink = (reinterpret_cast<char16_t *>(trgtbuf_ptr) - output_buffer.get());
        } else {
          sink = 0;
        }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

/**
 * Olivier Goffart, UTF-8 processing using SIMD (SSE4), 2012.
 * https://woboq.com/blog/utf-8-processing-using-simd.html
 */
void Benchmark::run_convert_utf8_to_utf16_utf8sse4(size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
        const char * srcbuf_ptr = data;
        size_t inbytes_left = size;
        char * trgtbuf_ptr = reinterpret_cast<char *>(output_buffer.get());
        size_t outbytes_left = size * sizeof(char16_t);
        size_t result_code = utf8sse4::fromUtf8(&srcbuf_ptr, &inbytes_left, &trgtbuf_ptr, &outbytes_left);
        bool is_ok = (result_code != size_t(-1));
        if(is_ok) {
          sink = (reinterpret_cast<char16_t *>(trgtbuf_ptr) - output_buffer.get());
        } else {
          sink = 0;
        }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}
#endif

void Benchmark::run_convert_valid_utf8_to_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};
    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_valid_utf8_to_utf16le(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_convert_valid_utf8_to_utf32(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
    volatile size_t sink{0};
    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_valid_utf8_to_utf32(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_convert_utf16_to_utf8(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: non-surrogate words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_utf16le_to_utf8(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf8_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: non-surrogate words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        result res = implementation.convert_utf16le_to_utf8_with_errors(data, size, output_buffer.get());
        sink = !(res.error);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf32(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: all words yield 4 bytes. We are making a safe assumption that all words
    // will be non-surrogate words so the size would get doubled (16 bits -> 32 bits).
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size * 2]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_utf16le_to_utf32(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf32_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: all words yield 4 bytes. We are making a safe assumption that all words
    // will be non-surrogate words so the size would get doubled (16 bits -> 32 bits).
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size * 2]};

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        result res = implementation.convert_utf16le_to_utf32_with_errors(data, size, output_buffer.get());
        sink = !(res.error);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf8_with_dynamic_allocation(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: non-surrogate words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.


    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &sink]() {
        auto dyn_size = implementation.utf8_length_from_utf16le(data, size);
        std::unique_ptr<char[]> output_buffer{new char[dyn_size]};
        sink = implementation.convert_utf16le_to_utf8(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf16_to_utf32_with_dynamic_allocation(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: all words yield 4 bytes. We are making a safe assumption that all words
    // will be non-surrogate words so the size would get doubled (16 bits -> 32 bits).

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &sink]() {
        auto dyn_size = implementation.utf32_length_from_utf16le(data, size);
        std::unique_ptr<char32_t[]> output_buffer{new char32_t[dyn_size]};
        sink = implementation.convert_utf16le_to_utf32(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_valid_utf16_to_utf8(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: non-surrogate words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_valid_utf16le_to_utf8(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf8(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by four (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: In the "worst" case, a 32-bit word will yield 4 UTF-8 bytes. So, we are making a safe
    // assumption that each word will produce 4 bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_utf32_to_utf8(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf8_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by four (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: In the "worst" case, a 32-bit word will yield 4 UTF-8 bytes. So, we are making a safe
    // assumption that each word will produce 4 bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        result res = implementation.convert_utf32_to_utf8_with_errors(data, size, output_buffer.get());
        sink = !(res.error);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_valid_utf32_to_utf8(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: In the "worst" case, a 32-bit word will yield 4 UTF-8 bytes. So, we are making a safe
    // assumption that each word will produce 4 bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_valid_utf32_to_utf8(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}


void Benchmark::run_convert_valid_utf16_to_utf32(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: non-surrogate words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size * 4]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_valid_utf16le_to_utf32(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: In the "worst" case, a 32-bit word will yield two 16-bit words. So, we are making a safe
    // assumption that each word will produce 2 bytes.
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_utf32_to_utf16le(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf16_with_errors(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: In the "worst" case, a 32-bit word will yield two 16-bit words. So, we are making a safe
    // assumption that each word will produce 2 bytes.
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2]};

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        result res = implementation.convert_utf32_to_utf16le_with_errors(data, size, output_buffer.get());
        sink = !(res.error);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) &&  (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_valid_utf32_to_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: In the "worst" case, a 32-bit word will yield two 16-bit words. So, we are making a safe
    // assumption that each word will produce 2 bytes.
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size * 2]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_valid_utf32_to_utf16le(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_count_utf8(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &sink]() {
        sink = implementation.count_utf8(data, size);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}

void Benchmark::run_count_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t*  data = reinterpret_cast<const char16_t*>(input_data.data());
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }
    size /= 2;
    volatile size_t sink{0};
    auto proc = [&implementation, data, size, &sink]() {
        sink = implementation.count_utf16le(data, size);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

const std::set<std::string>& Benchmark::all_procedures() const {
    return known_procedures;
}

std::set<simdutf::encoding_type> Benchmark::expected_encodings(const std::string& procedure) {
    return expected_input_encoding[procedure];
}


/**
 * LLVM relies on code from the Unicode Consortium
 * https://en.wikipedia.org/wiki/Unicode_Consortium
 */
void Benchmark::run_convert_utf8_to_utf16_llvm(size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
      const unsigned char * sourceStart = reinterpret_cast<const unsigned char *>(data);
      const unsigned char * sourceEnd = sourceStart + size;
      short unsigned int * targetStart =  reinterpret_cast<short unsigned int *>(output_buffer.get());
      short unsigned int * targetEnd = targetStart + size;
      bool  is_ok = (llvm::conversionOK == llvm::ConvertUTF8toUTF16 (&sourceStart, sourceEnd, &targetStart, targetEnd, llvm::ConversionFlags::strictConversion));
      if(is_ok) {
          sink = (targetStart - reinterpret_cast<short unsigned int *>(output_buffer.get()));
      } else {
          sink = 0;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}


void Benchmark::run_convert_utf8_to_utf32_llvm(size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size]};
    volatile size_t sink{0};
    auto proc = [data, size, &output_buffer, &sink]() {
      const unsigned char * sourceStart = reinterpret_cast<const unsigned char *>(data);
      const unsigned char * sourceEnd = sourceStart + size;
      unsigned int * targetStart =  reinterpret_cast<unsigned int *>(output_buffer.get());
      unsigned int * targetEnd = targetStart + size;
      bool  is_ok = (llvm::conversionOK == llvm::ConvertUTF8toUTF32 (&sourceStart, sourceEnd, &targetStart, targetEnd, llvm::ConversionFlags::strictConversion));
      if(is_ok) {
          sink = (targetStart - reinterpret_cast<unsigned int *>(output_buffer.get()));
      } else {
          sink = 0;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    size_t char_count = active_implementation->count_utf8(data, size);
    print_summary(result, size, char_count);
}


void Benchmark::run_convert_utf16_to_utf8_llvm(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: non-surrogate words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile size_t sink{0};

    auto proc = [data, size, &output_buffer, &sink]() {
      const short unsigned int * sourceStart = reinterpret_cast<const short unsigned int *>(data);
      const short unsigned int * sourceEnd = sourceStart + size;
      unsigned char * targetStart =  reinterpret_cast<unsigned char *>(output_buffer.get());
      unsigned char * targetEnd = targetStart + size * 4;
      bool  is_ok = (llvm::conversionOK == llvm::ConvertUTF16toUTF8 (&sourceStart, sourceEnd, &targetStart, targetEnd, llvm::ConversionFlags::strictConversion));
      if(is_ok) {
          sink = (targetStart - reinterpret_cast<unsigned char *>(output_buffer.get()));
      } else {
          sink = 0;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}

void Benchmark::run_convert_utf32_to_utf8_llvm(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: a single 32-bit word can yield up to four UTF-8 bytes. We are
    //       making a safe assumption that each 32-bit word will yield four
    //       UTF-8 bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile size_t sink{0};

    auto proc = [data, size, &output_buffer, &sink]() {
      const unsigned int * sourceStart = reinterpret_cast<const unsigned int *>(data);
      const unsigned int * sourceEnd = sourceStart + size;
      unsigned char * targetStart =  reinterpret_cast<unsigned char *>(output_buffer.get());
      unsigned char * targetEnd = targetStart + size*4;
      bool  is_ok = (llvm::conversionOK == llvm::ConvertUTF32toUTF8 (&sourceStart, sourceEnd, &targetStart, targetEnd, llvm::ConversionFlags::strictConversion));
      if(is_ok) {
          sink = (targetStart - reinterpret_cast<unsigned char *>(output_buffer.get()));
      } else {
          sink = 0;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}


void Benchmark::run_convert_utf16_to_utf32_llvm(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 2 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 2;

    // Note: all words yield four bytes. We make the safe assumption that all words 
    // will be non surrogate words so the size will double (16 bits -> 32 bits).
    std::unique_ptr<char32_t[]> output_buffer{new char32_t[size * 2]};

    volatile size_t sink{0};

    auto proc = [data, size, &output_buffer, &sink]() {
      const short unsigned int * sourceStart = reinterpret_cast<const short unsigned int *>(data);
      const short unsigned int * sourceEnd = sourceStart + size;
      unsigned int * targetStart =  reinterpret_cast<unsigned int *>(output_buffer.get());
      unsigned int * targetEnd = targetStart + 2*size;
      bool  is_ok = (llvm::conversionOK == llvm::ConvertUTF16toUTF32 (&sourceStart, sourceEnd, &targetStart, targetEnd, llvm::ConversionFlags::strictConversion));
      if(is_ok) {
          sink = (targetStart - reinterpret_cast<unsigned int *>(output_buffer.get()));
      } else {
          sink = 0;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = active_implementation->count_utf16le(data, size);
    print_summary(result, input_data.size(), char_count);
}



void Benchmark::run_convert_utf32_to_utf16_llvm(size_t iterations) {
    const simdutf::encoding_type bom  = BOM::check_bom(input_data.data(), input_data.size());
    const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data() + BOM::bom_byte_size(bom));
    size_t size = input_data.size() - BOM::bom_byte_size(bom);
    if (size % 4 != 0) {
       printf("# The input size is not divisible by two (it is %zu + %zu for BOM)",
               size_t(input_data.size()), size_t(BOM::bom_byte_size(bom)));
        printf(" Running function on truncated input.\n");
    }

    size /= 4;

    // Note: a single 32-bit word can produce a surrogate pair, i.e. two
    //       16-bit words. We are making a safe assumption that each 32-
    //       bit word will yield two 16-bit words.
    std::unique_ptr<char[]> output_buffer{new char[size * 2]};

    volatile size_t sink{0};

    auto proc = [data, size, &output_buffer, &sink]() {
      const unsigned int * sourceStart = reinterpret_cast<const unsigned int *>(data);
      const unsigned int * sourceEnd = sourceStart + size;
      short unsigned int * targetStart =  reinterpret_cast<short unsigned int *>(output_buffer.get());
      short unsigned int * targetEnd = targetStart + size*2;
      bool  is_ok = (llvm::conversionOK == llvm::ConvertUTF32toUTF16 (&sourceStart, sourceEnd, &targetStart, targetEnd, llvm::ConversionFlags::strictConversion));
      if(is_ok) {
          sink = (targetStart - reinterpret_cast<short unsigned int *>(output_buffer.get()));
      } else {
          sink = 0;
      }
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    size_t char_count = size;
    print_summary(result, input_data.size(), char_count);
}

} // namespace
