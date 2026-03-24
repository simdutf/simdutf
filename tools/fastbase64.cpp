#include "simdutf.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <memory>
#include <vector>

class CommandLine {
public:
  std::FILE *current_file{NULL};
  std::string input_file{"-"};
  std::string output_file{"-"};
  int wrap_cols{0};
  int current_col{0};
  bool is_stdin{false};
  bool ignore_garbage{false};
  bool output_specified{false};

  CommandLine() = default;
  ~CommandLine() {
    if (current_file != NULL && !is_stdin) {
      fclose(current_file);
    }
  }
  static CommandLine parse_and_validate_arguments(int argc, char *argv[],
                                                  bool gnumode);
  bool run_procedure(std::FILE *fpout);
  bool encode_to(std::FILE *fpout);
  bool decode_to(std::FILE *fpout);
  bool run();
  std::pair<bool, size_t> load_chunk(char *input_data, size_t chunk_size,
                                     size_t offset);
  void write_to_file_descriptor(std::FILE *fp, const char *data, size_t length);
  void write_with_wrapping(std::FILE *fp, const char *data, size_t length,
                           int &current_col, int wrap_cols);
  static void show_help(const std::string &command_name, bool gnumode);
  bool decode = false; // default: encode for both fastbase64 and coreutils
};

CommandLine CommandLine::parse_and_validate_arguments(int argc, char *argv[],
                                                      bool gnumode) {
  CommandLine cmdline;
  std::vector<std::string> positional;
  std::string command_name = argv[0];
  cmdline.wrap_cols =
      gnumode ? 76 : 0; // default wrap: 76 for coreutils, 0 for fastbase64
  size_t i = 1;
  while (i < argc) {
    std::string arg = argv[i];
    if (arg == "-b" || arg == "--break") {
      if (i + 1 >= argc) {
        throw std::runtime_error("Missing value for " + arg);
      }
      cmdline.wrap_cols = std::stoi(argv[i + 1]);
      if (cmdline.wrap_cols < 0) {
        throw std::runtime_error("Break columns must be non-negative");
      }
      cmdline.decode = false; // -b implies encode
      i += 2;
    } else if (arg == "-w") {
      if (i + 1 >= argc) {
        throw std::runtime_error("Missing value for " + arg);
      }
      cmdline.wrap_cols = std::stoi(argv[i + 1]);
      if (cmdline.wrap_cols < 0) {
        throw std::runtime_error("Wrap columns must be non-negative");
      }
      cmdline.decode = false; // -w implies encode
      i += 2;
    } else if (arg.substr(0, 7) == "--wrap=") {
      cmdline.wrap_cols = std::stoi(arg.substr(7));
      if (cmdline.wrap_cols < 0) {
        throw std::runtime_error("Wrap columns must be non-negative");
      }
      cmdline.decode = false; // --wrap implies encode
      i++;
    } else if (arg.substr(0, 8) == "--break=") {
      cmdline.wrap_cols = std::stoi(arg.substr(8));
      if (cmdline.wrap_cols < 0) {
        throw std::runtime_error("Break columns must be non-negative");
      }
      cmdline.decode = false; // --break= implies encode
      i++;
    } else if (arg.substr(0, 8) == "--input=") {
      if (gnumode) {
        throw std::runtime_error("Unknown option: " + arg);
      }
      cmdline.input_file = arg.substr(8);
      i++;
    } else if (arg.substr(0, 9) == "--output=") {
      cmdline.output_file = arg.substr(9);
      cmdline.output_specified = true;
      i++;
    } else if (arg == "-d" || arg == "-D" || arg == "--decode") {
      cmdline.decode = true;
      i++;
    } else if (arg == "-e" || arg == "--encode") {
      cmdline.decode = false;
      i++;
    } else if (arg == "--ignore-garbage") {
      cmdline.ignore_garbage = true;
      i++;
    } else if (arg == "-n" || arg == "--noerrcheck") {
      if (!gnumode) {
        throw std::runtime_error("Unknown option: " + arg);
      }
      cmdline.ignore_garbage = true;
      i++;
    } else if (arg == "-h" || arg == "--help") {
      show_help(command_name, gnumode);
      exit(EXIT_SUCCESS);
    } else if (arg == "-i") {
      if (gnumode) {
        cmdline.ignore_garbage = true;
        i++;
      } else {
        if (i + 1 >= argc) {
          throw std::runtime_error("Missing value for " + arg);
        }
        cmdline.input_file = argv[i + 1];
        i += 2;
      }
    } else if (arg == "--input") {
      if (gnumode) {
        throw std::runtime_error("Unknown option: " + arg);
      }
      if (i + 1 >= argc) {
        throw std::runtime_error("Missing value for " + arg);
      }
      cmdline.input_file = argv[i + 1];
      i += 2;
    } else if (arg == "-o" || arg == "--output") {
      if (i + 1 >= argc) {
        throw std::runtime_error("Missing value for " + arg);
      }
      cmdline.output_file = argv[i + 1];
      cmdline.output_specified = true;
      i += 2;
    } else if (arg == "--version") {
      printf("fastbase64 version %s\n", SIMDUTF_VERSION);
      exit(EXIT_SUCCESS);
    } else if (arg[0] == '-' && arg != "-") {
      throw std::runtime_error("Unknown option: " + arg);
    } else {
      positional.push_back(arg);
      i++;
    }
  }

  // Handle positional arguments
  if (positional.size() > 0) {
    cmdline.input_file = positional[0];
  }
  if (positional.size() > 1 && !cmdline.output_specified) {
    cmdline.output_file = positional[1];
  }
  if (positional.size() > 2) {
    throw std::runtime_error("Too many positional arguments");
  }

  return cmdline;
}

bool CommandLine::run() {
  // Open input file
  if (input_file == "-") {
    current_file = stdin;
    is_stdin = true;
  } else {
    current_file = std::fopen(input_file.c_str(), "rb");
    if (current_file == NULL) {
      throw std::runtime_error("Could not open input file: " + input_file +
                               ": " + std::string(strerror(errno)));
    }
  }

  // Open output file
  if (output_file == "-") {
    return run_procedure(stdout);
  } else {
    struct FileDeleter {
      void operator()(FILE *fp) const {
        if (fp) {
          if (fclose(fp) != 0) {
            // Note: We do our best to close the file, but
            // if fclose fails, there's not much we can do. We ignore
            // the error.
          }
        }
      }
    };
    std::unique_ptr<FILE, FileDeleter> fp(fopen(output_file.c_str(), "wb"));
    if (!fp) {
      fprintf(stderr, "Could not open output file: %s: %s\n",
              output_file.c_str(), strerror(errno));
      return false;
    }
    bool result = run_procedure(fp.get());
    if (result) {
      FILE *raw = fp.release();
      if (fclose(raw) != 0) {
        throw std::runtime_error("Failed to close output file: " + output_file +
                                 ": " + std::string(strerror(errno)));
      }
    }
    return result;
  }
}

bool CommandLine::run_procedure(std::FILE *fpout) {
  if (decode) {
    return decode_to(fpout);
  } else {
    return encode_to(fpout);
  }
}

// load_chunk returns a pair of a boolean and a size_t, the boolean is true
// until we reach the end of the stream, the size_t is the number of bytes read.
std::pair<bool, size_t>
CommandLine::load_chunk(char *input_data, size_t chunk_size, size_t offset) {
  size_t bytes_read =
      std::fread(input_data + offset, 1, chunk_size - offset, current_file);
  if (std::ferror(current_file)) {
    throw std::runtime_error("Error while reading:" +
                             std::string(strerror(errno)));
  }
  if (std::feof(current_file)) { // Check if current_file is done

    return {false, bytes_read};
  }
  return {true, bytes_read};
}

void CommandLine::write_to_file_descriptor(std::FILE *fp, const char *data,
                                           size_t length) {
  if (fp == NULL) {
    throw std::runtime_error("File pointer is NULL");
  }
  size_t bytes_written = std::fwrite(data, 1, length, fp);
  if (bytes_written != length) {
    throw std::runtime_error("Failed to write:" + std::string(strerror(errno)));
  }
}

void CommandLine::write_with_wrapping(std::FILE *fp, const char *data,
                                      size_t length, int &current_col,
                                      int wrap_cols) {
  if (fp == NULL) {
    throw std::runtime_error("File pointer is NULL");
  }
  if (wrap_cols <= 0) { // This should never happen but we want to be safe.
    write_to_file_descriptor(fp, data, length);
    return;
  }
  size_t i = 0;
  while (i < length) {
    if (current_col >= wrap_cols) {
      if (std::fputc('\n', fp) == EOF) {
        throw std::runtime_error("Failed to write:" +
                                 std::string(strerror(errno)));
      }
      current_col = 0;
    }
    int remaining = wrap_cols - current_col;
    size_t to_write = std::min((size_t)remaining, length - i);
    if (std::fwrite(data + i, 1, to_write, fp) != to_write) {
      throw std::runtime_error("Failed to write:" +
                               std::string(strerror(errno)));
    }
    current_col += to_write;
    i += to_write;
  }
}

bool CommandLine::decode_to(std::FILE *fpout) {
  const size_t chunk_size = 65536;
  std::array<char, chunk_size> input_data;
  std::array<char, (chunk_size + 3) / 4 * 3> output_buffer;
  size_t pos =
      0; // the pos variable keeps track of the position in the input file.
  // Its purpose is to provide a position for error messages.
  size_t offset = 0;
  simdutf::base64_options options =
      ignore_garbage ? simdutf::base64_options::base64_default_accept_garbage
                     : simdutf::base64_options::base64_default;
  // load_chunk returns a pair of a boolean and a size_t, the boolean is true
  // until we reach the end of the stream, the size_t is the number of bytes
  // read.
  for (auto p = load_chunk(input_data.data(), chunk_size, offset);
       p.second + offset > 0;
       p = load_chunk(input_data.data(), chunk_size, offset)) {
    size_t total_input = p.second + offset;
    // if p.first is false, we have reached the end of the file.
    if (!p.first) {
      // Final chunk: use loose mode.
      simdutf::full_result r =
          simdutf::get_active_implementation()->base64_to_binary_details(
              input_data.data(), total_input, output_buffer.data(), options,
              simdutf::last_chunk_handling_options::loose);
      if (r.error == simdutf::error_code::INVALID_BASE64_CHARACTER) {
        fprintf(stderr, "Invalid base64 character at position %zu.\n",
                pos + r.input_count);
        return false;
      }
      if (r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
        fprintf(stderr, "The base64 input contained an invalid number of "
                        "characters or could not be read.\n");
        return false;
      }
      if (r.error != simdutf::error_code::SUCCESS) {
        fprintf(stderr, "Unexpected error during base64 decoding.\n");
        return false;
      }
      write_to_file_descriptor(fpout, output_buffer.data(), r.output_count);
      return true;
    }
    // Non-final chunk: use stop_before_partial so incomplete 4-char groups
    // at the end are not decoded and can be carried over to the next chunk.
    // base64_to_binary_details returns both input_count and output_count,
    // so we know exactly how many input bytes were consumed.
    simdutf::full_result r =
        simdutf::get_active_implementation()->base64_to_binary_details(
            input_data.data(), total_input, output_buffer.data(), options,
            simdutf::last_chunk_handling_options::stop_before_partial);
    if (r.error == simdutf::error_code::INVALID_BASE64_CHARACTER) {
      fprintf(stderr, "Invalid base64 character at position %zu.\n",
              pos + r.input_count);
      return false;
    }
    if (r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
      fprintf(stderr, "The base64 input contained an invalid number of "
                      "characters: remainder of one base64 character.\n");
      return false;
    }
    if (r.error != simdutf::error_code::SUCCESS) {
      fprintf(stderr,
              "There was an unexpected error during base64 decoding.\n");
      return false;
    }
    if (r.input_count == 0) {
      // No progress made, compact the buffer by removing all ignorable bytes
      size_t write_index = 0;
      for (size_t i = 0; i < total_input; ++i) {
        char c = input_data[i];
        if (!simdutf::base64_ignorable(c, options)) {
          input_data[write_index++] = c;
        }
      }
      size_t removed = total_input - write_index;
      // Update offset to the compacted length and advance position
      offset = write_index;
      pos += removed;
    } else {
      write_to_file_descriptor(fpout, output_buffer.data(), r.output_count);
      // Carry over unconsumed input bytes for the next iteration.
      offset = total_input - r.input_count;
      if (offset > 0) {
        std::memmove(input_data.data(), input_data.data() + r.input_count,
                     offset);
      }
      pos += r.input_count;
    }
  }
  return true;
}

bool CommandLine::encode_to(std::FILE *fpout) {
  const size_t chunk_size = 49152;
  std::array<char, chunk_size> input_data;
  std::array<char, (chunk_size + 2) / 3 * 4> output_buffer;
  size_t offset = 0;
  this->current_col = 0;
  // load_chunk returns a pair of a boolean and a size_t, the boolean is true
  // until we reach the end of the stream, the size_t is the number of bytes
  // read.
  for (auto p = load_chunk(input_data.data(), chunk_size, offset); p.second > 0;
       p = load_chunk(input_data.data(), chunk_size, offset)) {
    // We have read p.second bytes, and we have offset bytes left from the
    // previous chunk.
    size_t total_bytes = p.second + offset;
    // If we have reached the end of the file, we write the output and return
    // true.
    if (!p.first) {
      // We finish the file
      size_t output_size = simdutf::binary_to_base64(
          input_data.data(), total_bytes, output_buffer.data());
      if (this->wrap_cols == 0) {
        write_to_file_descriptor(fpout, output_buffer.data(), output_size);
      } else {
        write_with_wrapping(fpout, output_buffer.data(), output_size,
                            this->current_col, this->wrap_cols);
      }
      if (std::fputc('\n', fpout) == EOF) {
        throw std::runtime_error("Failed to write:" +
                                 std::string(strerror(errno)));
      }
      return true;
    }
    // We want to write the data in chunks of 4 bytes and read blocks of
    // 3 bytes. We keep the last 0, 1 or 2 bytes in the input buffer.
    offset = total_bytes % 3;
    total_bytes -= offset;
    size_t output_size = simdutf::binary_to_base64(
        input_data.data(), total_bytes, output_buffer.data());
    if (this->wrap_cols == 0) {
      write_to_file_descriptor(fpout, output_buffer.data(), output_size);
    } else {
      write_with_wrapping(fpout, output_buffer.data(), output_size,
                          this->current_col, this->wrap_cols);
    }
    // Copy 0, 1 or 2 bytes to the start of the input buffer.
    memcpy(input_data.data(), input_data.data() + total_bytes, offset);
  }
  write_to_file_descriptor(fpout, "\n", 1);
  return true;
}

void CommandLine::show_help(const std::string &command_name, bool gnumode) {
  // The usage line omits [OUTPUTFILE] even though the tool supports a second
  // positional output filename. It is intentional (for simplicity).
  printf("Usage: %s [OPTIONS] [INPUTFILE]\n\n", command_name.c_str());
  if (gnumode) {
    printf(
        "Encodes or decodes base64 data with GNU coreutils compatibility.\n\n");
  } else {
    printf("Encodes or decodes base64 data using the high-performance simdutf "
           "library.\n\n");
  }
  printf("  -b, --break=NUM   break encoded output up into lines of length NUM "
         "(default %s)\n",
         gnumode ? "76" : "0, meaning no breaks");
  printf("  -w, --wrap=NUM    same as -b\n");
  printf("  -d, -D, --decode   decode input\n");
  printf("  -e, --encode       encode input (default)\n");
  printf(
      "  --ignore-garbage   when decoding, ignore non-alphabet characters\n");
  printf("  -h, --help         display this message\n");
  if (gnumode) {
    printf("  -i, -n, --noerrcheck  same as --ignore-garbage\n");
  } else {
    printf("  -i, --input=FILE   input file (default: \"-\" for stdin)\n");
  }
  printf("  -o, --output=FILE  output file (default: \"-\" for stdout)\n");
  printf("  --version          output version information and exit\n\n");
  printf("With no INPUTFILE, or when INPUTFILE is -, read standard input.\n");
  printf("If a second filename is specified, it is used as the output file.\n");
  printf("Otherwise, output is redirected to standard output.\n");
}

int main(int argc, char *argv[]) {
  std::string progname = argv[0];
  // Detect if we are running in GNU mode by checking if the program name
  // contains "coreutils". Technically, this would allow a program called
  // potatocoreutilsmywife to trigger GNU mode, but it is a simple heuristic
  // that should work in practice.
  std::filesystem::path exe_path(progname);
  std::string exe_name = exe_path.filename().string();
  bool gnumode = (exe_name.find("coreutils") != std::string::npos);
  try {
    CommandLine cmdline =
        CommandLine::parse_and_validate_arguments(argc, argv, gnumode);
    return cmdline.run() ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (const std::exception &e) {
    fprintf(stderr, "%s\n", e.what());
    // main() prints the full help text for any thrown exception (including
    // runtime I/O errors like open/write/close failures). This is deliberate,
    // as it provides users with immediate guidance on how to use the tool
    // correctly after an error.
    CommandLine::show_help(argv[0], gnumode);
    return EXIT_FAILURE;
  }
}
