#include "benchmark.h"
#include "simdutf.h"

#include <cassert>
#include <array>
#include <iostream>

namespace simdutf::benchmarks {

Benchmark::Benchmark(std::vector<input::Testcase>&& testcases)
    : BenchmarkBase(std::move(testcases)) {

    std::array<std::string, 8> implemented_functions{
        "validate_utf8",
        "validate_utf16",
        "count_utf8", 
        "count_utf16"
        "convert_utf8_to_utf16",
        "convert_valid_utf8_to_utf16",
        "convert_utf16_to_utf8",
        "convert_valid_utf16_to_utf8",
    };

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


// TODO: should be moved to some utility function, no idea where.
namespace BOM {
    enum Type { // purposely no C++ enum class, as we use the namespace to classify the constants
        UTF16_LE,   // 0xff 0xfe
        UTF16_BE,   // 0xfe 0xff
        UTF32_LE,   // 0xff 0xfe 0x00 0x00
        UTF32_BE,   // 0x00 0x00 0xfe 0xff
        UTF8,       // 0xef 0xbb 0xbf
        Unspecified
    };

    Type read_unsafe(const uint8_t* byte) {
        if (byte[0] == 0xff and byte[1] == 0xfe) {
            if (byte[2] == 0x00 and byte[3] == 0x0)
                return UTF32_LE;
            else
                return UTF16_LE;
        } else if (byte[0] == 0xfe and byte[1] == 0xff) {
            return UTF16_BE;
        } else if (byte[0] == 0x00 and byte[1] == 0x00 and byte[2] == 0xfe and byte[3] == 0xff) {
            return UTF32_BE;
        } else if (byte[0] == 0xef and byte[1] == 0xbb and byte[3] == 0xbf) {
            return UTF8;
        }

        return Unspecified;
    }

    size_t byte_size(Type bom) {
        switch (bom) {
            case UTF16_LE:     return 2;
            case UTF16_BE:     return 2;
            case UTF32_LE:     return 4;
            case UTF32_BE:     return 4;
            case UTF8:         return 3;
            case Unspecified:  return 0;
            default:           return 0;
        }
    }
} // namespace

void Benchmark::run(const std::string& procedure_name, size_t iterations) {
    const size_t p = procedure_name.find('+');
    assert(p != std::string::npos);

    const std::string name{procedure_name.substr(0, p)};
    const std::string impl{procedure_name.substr(p + 1)};

    auto implementation = simdutf::available_implementations[impl];
    if (implementation == nullptr) {
        throw std::runtime_error("Wrong implementation " + impl);
    }

    if (name == "validate_utf8") {
        run_validate_utf8(*implementation, iterations);
    } else if (name == "validate_utf16") {
        run_validate_utf16(*implementation, iterations);
    } else if(name == "count_utf8") {
        run_count_utf8(*implementation, iterations);
    } else if(name == "count_utf16") {
        run_count_utf16(*implementation, iterations);
    } else if(name == "convert_utf8_to_utf16") {
        run_convert_utf8_to_utf16(*implementation, iterations);
    } else if(name == "convert_valid_utf8_to_utf16") {
        run_convert_valid_utf8_to_utf16(*implementation, iterations);
    } else if(name == "convert_utf16_to_utf8") {
        run_convert_utf16_to_utf8(*implementation, iterations);
    } else if(name == "convert_valid_utf16_to_utf8") {
        run_convert_valid_utf16_to_utf8(*implementation, iterations);
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
    print_summary(result, size);
}

void Benchmark::run_validate_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const BOM::Type bom  = BOM::read_unsafe(input_data.data());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::byte_size(bom));
    size_t size = input_data.size() - BOM::byte_size(bom);
    if (size % 2 != 0) {
        printf("The input size is not divisible by two (it is %lu + %lu for BOM)",
               input_data.size(), BOM::byte_size(bom));

        return;
    }

    size /= 2;

    volatile bool sink{false};

    auto proc = [&implementation, data, size, &sink]() {
        sink = implementation.validate_utf16(data, size);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == false) && (iterations > 0)) { std::cerr << "The input was declared invalid.\n"; }
    print_summary(result, size);
}

void Benchmark::run_convert_utf8_to_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_utf8_to_utf16(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    print_summary(result, size);
}

void Benchmark::run_convert_valid_utf8_to_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const char*  data = reinterpret_cast<const char*>(input_data.data());
    const size_t size = input_data.size();
    std::unique_ptr<char16_t[]> output_buffer{new char16_t[size]};
    volatile size_t sink{0};
    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_valid_utf8_to_utf16(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate a misconfiguration.\n"; }
    print_summary(result, size);
}

void Benchmark::run_convert_utf16_to_utf8(const simdutf::implementation& implementation, size_t iterations) {
    const BOM::Type bom  = BOM::read_unsafe(input_data.data());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::byte_size(bom));
    size_t size = input_data.size() - BOM::byte_size(bom);
    if (size % 2 != 0) {
        printf("The input size is not divisible by two (it is %lu + %lu for BOM)",
               input_data.size(), BOM::byte_size(bom));

        return;
    }

    size /= 2;

    // Note: non-surroage words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_utf16_to_utf8(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    print_summary(result, size);
}

void Benchmark::run_convert_valid_utf16_to_utf8(const simdutf::implementation& implementation, size_t iterations) {
    const BOM::Type bom  = BOM::read_unsafe(input_data.data());
    const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data() + BOM::byte_size(bom));
    size_t size = input_data.size() - BOM::byte_size(bom);
    if (size % 2 != 0) {
        printf("The input size is not divisible by two (it is %lu + %lu for BOM)",
               input_data.size(), BOM::byte_size(bom));

        return;
    }

    size /= 2;

    // Note: non-surroage words can yield up to 3 bytes, a surrogate pair yields 4 bytes,
    //       thus we're making safe assumption that each 16-bit word will be expanded
    //       to four bytes.
    std::unique_ptr<char[]> output_buffer{new char[size * 4]};

    volatile size_t sink{0};

    auto proc = [&implementation, data, size, &output_buffer, &sink]() {
        sink = implementation.convert_valid_utf16_to_utf8(data, size, output_buffer.get());
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    print_summary(result, size);
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
    print_summary(result, size);
}

void Benchmark::run_count_utf16(const simdutf::implementation& implementation, size_t iterations) {
    const char*  utf8data = reinterpret_cast<const char*>(input_data.data());
    const size_t utf8size = input_data.size();
    std::unique_ptr<char16_t[]> tmp_buffer_utf16{new char16_t[utf8size]};
    const size_t size = implementation.convert_utf8_to_utf16(utf8data, utf8size, tmp_buffer_utf16.get());
    const char16_t*  data = tmp_buffer_utf16.get();
    volatile size_t sink{0};
    auto proc = [&implementation, data, size, &sink]() {
        sink = implementation.count_utf16(data, size);
    };
    count_events(proc, iterations); // warming up!
    const auto result = count_events(proc, iterations);
    if((sink == 0) && (size != 0) && (iterations > 0)) { std::cerr << "The output is zero which might indicate an error.\n"; }
    print_summary(result, size);
}

const std::set<std::string>& Benchmark::all_procedures() const {
    return known_procedures;
}

} // namespace
