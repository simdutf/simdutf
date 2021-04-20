#include "src/cmdline.h"
#include "src/benchmark.h"

int main(int argc, char* argv[]) {
#ifdef INOUE2008
    inoue2008::inoue_test(); // minimal testing
#endif
    using simdutf::benchmarks::CommandLine;
    CommandLine cmdline;
    try {
        cmdline = CommandLine::parse_arguments(argc, argv);
        if (cmdline.show_help) {
            CommandLine::print_help();
            return EXIT_SUCCESS;
        }
        else if (cmdline.empty()) {
            CommandLine::print_help();
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }

    using simdutf::benchmarks::Benchmark;

    Benchmark benchmark{Benchmark::create(cmdline)};
    if (cmdline.show_procedures) {
        const auto& known_procedures = benchmark.all_procedures();
        printf("Available procedures (%lu)\n", known_procedures.size());
        for (const auto& name: known_procedures) {
            printf("- %s\n", name.c_str());
        }
        return EXIT_SUCCESS;
    } else {
        return benchmark.run() ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}
