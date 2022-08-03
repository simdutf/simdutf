#include "../singleheader/simdutf.h"
#include "../singleheader/simdutf.cpp"

#include <filesystem>
#include <vector>
#include <set>
#include <ostream>


class CommandLine
{
public:
  std::string from_encoding;
  std::string to_encoding;
  std::set<std::filesystem::path> input_files;
  std::filesystem::path output_file;
  std::vector<uint8_t> input_data;

  CommandLine() = default;
  static CommandLine parse_and_validate_arguments(int argc, char* argv[]);
  static void show_help();
  static void show_formats();

  void run();
  void run_procedure(std::ostream*);
  void iconv_fallback();
  void load_file(const std::filesystem::path&);
};