#include "simdutf.h"

#include <filesystem>
#include <vector>
#include <set>


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
  void run_procedure(std::FILE *fp);
  void iconv_fallback();
  bool load_file(const std::filesystem::path&);
  bool write_to_file_descriptor(std::FILE *fp, const char * data, size_t length);
};