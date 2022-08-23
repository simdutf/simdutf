#include "simdutf.h"

#if !defined(ICONV_AVAILABLE) && __has_include (<iconv.h>)
#define ICONV_AVAILABLE 1
#endif //__has_include (<iconv.h>)
#if ICONV_AVAILABLE
#include <iconv.h>
#endif

#include <filesystem>
#include <array>
#include <queue>

constexpr size_t CHUNK_SIZE = 1048576;

class CommandLine
{
public:
  std::string from_encoding;
  std::string to_encoding;
  std::queue<std::filesystem::path> input_files;
  std::FILE* current_file = NULL;
  std::filesystem::path output_file;
  std::array<uint8_t, CHUNK_SIZE> input_data;

  CommandLine() = default;
  static CommandLine parse_and_validate_arguments(int argc, char* argv[]);
  static void show_help();
  static void show_formats();

  void run();
  void run_procedure(std::FILE *fp);
  void iconv_fallback(std::FILE *fp);
  bool load_data(size_t count);
  bool write_to_file_descriptor(std::FILE *fp, const char * data, size_t length);
};