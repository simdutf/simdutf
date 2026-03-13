#include "simdutf.h"

#include <array>
#include <cerrno>
#include <filesystem>
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

  CommandLine() = default;
  static CommandLine parse_and_validate_arguments(int argc, char *argv[]);
  bool run_procedure(std::FILE *fpout);
  bool encode_to(std::FILE *fpout);
  bool decode_to(std::FILE *fpout);
  bool run();
  std::pair<bool, size_t> load_chunk(char *input_data, size_t chunk_size,
                                     size_t offset);
  bool write_to_file_descriptor(std::FILE *fp, const char *data, size_t length);
  bool write_with_wrapping(std::FILE *fp, const char *data, size_t length,
                           int &current_col, int wrap_cols);
  static void show_help();
  bool decode;
};

CommandLine CommandLine::parse_and_validate_arguments(int argc, char *argv[]) {
  CommandLine cmdline;
  cmdline.decode = true;  // decode is default
  cmdline.input_file = "-";
  cmdline.output_file = "-";
  cmdline.wrap_cols = 0;
  cmdline.ignore_garbage = false;

  std::vector<std::string> positional;

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
      cmdline.decode = false;  // -b implies encode
      i += 2;
    } else if (arg == "-w") {
      if (i + 1 >= argc) {
        throw std::runtime_error("Missing value for " + arg);
      }
      cmdline.wrap_cols = std::stoi(argv[i + 1]);
      if (cmdline.wrap_cols < 0) {
        throw std::runtime_error("Wrap columns must be non-negative");
      }
      cmdline.decode = false;  // -w implies encode
      i += 2;
    } else if (arg.substr(0, 7) == "--wrap=") {
      cmdline.wrap_cols = std::stoi(arg.substr(7));
      if (cmdline.wrap_cols < 0) {
        throw std::runtime_error("Wrap columns must be non-negative");
      }
      cmdline.decode = false;  // --wrap implies encode
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
    } else if (arg == "-h" || arg == "--help") {
      show_help();
      exit(0);
    } else if (arg == "-i" || arg == "--input") {
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
      i += 2;
    } else if (arg == "--version") {
      printf("fastbase64 version %s\n", SIMDUTF_VERSION);
      exit(0);
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
  if (positional.size() > 1) {
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
      throw std::runtime_error("Could not open input file: " + input_file + ": " +
                               std::string(strerror(errno)));
    }
  }

  // Open output file
  if (output_file == "-") {
    return run_procedure(stdout);
  } else {
    SIMDUTF_PUSH_DISABLE_WARNINGS
    SIMDUTF_DISABLE_DEPRECATED_WARNING
    std::FILE *fp = std::fopen(output_file.c_str(), "wb");
    SIMDUTF_POP_DISABLE_WARNINGS
    if (fp == NULL) {
      fprintf(stderr, "Could not open output file: %s: %s\n", output_file.c_str(),
              strerror(errno));
      return false;
    }
    bool success = run_procedure(fp);
    // Let us first try to close the file.
    if (fclose(fp) != 0) {
      fprintf(stderr, "Failed to close %s: %s\n", output_file.c_str(),
              strerror(errno));
      return false;
    }
    return success;
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
    if (!is_stdin) std::fclose(current_file);
    throw std::runtime_error("Error while reading:" +
                             std::string(strerror(errno)));
  }
  if (std::feof(current_file)) { // Check if current_file is done
    if (!is_stdin) std::fclose(current_file);   // best effort
    current_file = NULL;
    return {false, bytes_read};
  }
  return {true, bytes_read};
}

bool CommandLine::write_to_file_descriptor(std::FILE *fp, const char *data,
                                           size_t length) {
  if (fp == NULL) {
    return false;
  }
  size_t bytes_written = std::fwrite(data, 1, length, fp);
  if (bytes_written != length) {
    throw std::runtime_error("Failed to write:" + std::string(strerror(errno)));
  }
  return true;
}

bool CommandLine::write_with_wrapping(std::FILE *fp, const char *data,
                                     size_t length, int &current_col,
                                     int wrap_cols) {
  if (fp == NULL) {
    return false;
  }
  size_t i = 0;
  while (i < length) {
    if (current_col >= wrap_cols) {
      if (std::fwrite("\n", 1, 1, fp) != 1) {
        throw std::runtime_error("Failed to write:" + std::string(strerror(errno)));
      }
      current_col = 0;
    }
    int remaining = wrap_cols - current_col;
    size_t to_write = std::min((size_t)remaining, length - i);
    if (std::fwrite(data + i, 1, to_write, fp) != to_write) {
      throw std::runtime_error("Failed to write:" + std::string(strerror(errno)));
    }
    current_col += to_write;
    i += to_write;
  }
  return true;
}

bool CommandLine::decode_to(std::FILE *fpout) {
  const size_t chunk_size = 65536;
  std::array<char, chunk_size> input_data;
  std::array<char, (chunk_size + 3) / 4 * 3> output_buffer;
  size_t pos =
      0; // the pos variable keeps track of the position in the input file.
  // Its purpose is to provide a position for error messages.
  size_t offset = 0;
  simdutf::base64_options options = simdutf::base64_options::base64_default_accept_garbage;
  // load_chunk returns a pair of a boolean and a size_t, the boolean is true
  // until we reach the end of the stream, the size_t is the number of bytes
  // read.
  for (auto p = load_chunk(input_data.data(), chunk_size, offset); p.second > 0;
       p = load_chunk(input_data.data(), chunk_size, offset)) {
    // We convertto base64 the data we have read so far
    simdutf::result r = simdutf::base64_to_binary(
        input_data.data(), p.second + offset, output_buffer.data(), options);
    // If we have encountered an invalid character, we print an error message
    // and return false.
    if (r.error == simdutf::error_code::INVALID_BASE64_CHARACTER) {
      fprintf(stderr, "Invalid base64 character at position %zu\n.",
              pos + r.count);
      return false;
    }
    // if p.first is false, we have reached the end of the file.
    if (!p.first) {
      // At the end of the file, if we are left with one base64 character
      // leftover, it is a fatal error.
      if (r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
        fprintf(stderr, "The base64 input contained an invalid number of "
                        "characters or could not be read.");
        return false;
      }
      // Otherwise, we write the output and return true.
      write_to_file_descriptor(fpout, output_buffer.data(), r.count);
      return true;
    }
    // We want to write the data in chunks of 3 bytes and read blocks of
    // 4 bytes. We keep the last 0, 1, 3 or 4 base64 bytes in the input buffer.
    // And we write the output in chunks of 3 bytes.
    offset = 0;
    if (r.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
      offset = 1;
    } else {
      offset = (r.count % 3) == 0 ? 0 : (r.count % 3) + 1;
    }
    // Copy 0, 1, 2 or 3 non-space bytes to the input buffer.
    size_t copied = 0;
    for (size_t z = p.second - 1; copied < offset; z--) {
      if (input_data[z] == ' ' || input_data[z] == '\n' ||
          input_data[z] == '\r' || input_data[z] == '\t') {
        continue;
      }
      copied++;
      input_data[offset - copied] = input_data[z];
    }
    // We write a multiple of 3 bytes to the output buffer, discarding
    // the last 0, 1 or 2 bytes.
    r.count -= (r.count % 3);
    write_to_file_descriptor(fpout, output_buffer.data(), r.count);
    pos += p.second;
  }
  return true;
}

bool CommandLine::encode_to(std::FILE *fpout) {
  const size_t chunk_size = 49152;
  std::array<char, chunk_size> input_data;
  std::array<char, (chunk_size + 2) / 3 * 4> output_buffer;
  size_t pos = 0;
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
      write_to_file_descriptor(fpout, "\n", 1);
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

void CommandLine::show_help() {
  printf("Usage: fastbase64 [OPTIONS...] [INPUTFILE] [OUTPUTFILE]\n\n");
  printf("  -b, --break NUM   break encoded output up into lines of length NUM\n");
  printf("  -w COLS            same as -b\n");
  printf("  --wrap=COLS        same as -b\n");
  printf("  -d, -D, --decode   decode input (default)\n");
  printf("  -e, --encode       encode input\n");
  printf("  --ignore-garbage   when decoding, ignore non-alphabet characters\n");
  printf("  -h, --help         display this message\n");
  printf("  -i, --input FILE   input file (default: \"-\" for stdin)\n");
  printf("  -o, --output FILE  output file (default: \"-\" for stdout)\n");
  printf("  --version          output version information and exit\n\n");
  printf("With no INPUTFILE, or when INPUTFILE is -, read standard input.\n");
  printf("If OUTPUTFILE is not specified, the output is redirected to standard output.\n");
}

int main(int argc, char *argv[]) {
  try {
    CommandLine cmdline = CommandLine::parse_and_validate_arguments(argc, argv);
    return cmdline.run() ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (const std::exception &e) {
    fprintf(stderr, "%s\n", e.what());
    CommandLine::show_help();
    return EXIT_FAILURE;
  }
}
