#include "sutf.h"

#include <vector>
#include <string>
#include <set>
#include <filesystem>
#include <iterator>
#include <climits>

CommandLine parse_and_validate_arguments(int argc, char* argv[]) {
  CommandLine cmdline;
  std::vector<std::string> arguments;

  for (int i=1; i < argc; i++) {
    std::string arg{argv[i]};
    if ((arg == "--help") || (arg == "-h")) {
      CommandLine::show_help();
      return cmdline;
    }
    else if ((arg == "--list") || (arg == "-l")) {
      CommandLine::show_formats();
      return cmdline;
    }
    else {
      arguments.push_back(std::move(arg));
    }
  }

  bool has_from_arg = false;
  bool has_to_arg = false;

  for (size_t i=0; i < arguments.size();) {
    const std::string& arg = arguments[i];

    if (arg == "-f") {
      const std::string& value = arguments.at(i + 1);
      cmdline.from_encoding = value;
      has_from_arg = true;
      i += 2;
    }
    else if (arg == "-t") {
      const std::string& value = arguments.at(i + 1);
      cmdline.to_encoding = value;
      has_to_arg = true;
      i += 2;
    }
    else if (arg == "-o") {
      const std::string& value = arguments.at(i + 1);
      cmdline.output_file = value;
      i += 2;
    }
    else {
      if (! std::filesystem::exists(arg)) {
        throw std::runtime_error("File " + arg + " does not exist.");
      }
      cmdline.input_files.push(arg);
      i++;
    }
  }

  if (!has_from_arg || !has_to_arg) {
    throw std::invalid_argument("Missing -f or -t argument(s).");
  }

  return cmdline;
}

void CommandLine::run() {
  if (output_file.empty()) {
    run_procedure(stdout);
  } else {
    SIMDUTF_PUSH_DISABLE_WARNINGS
    SIMDUTF_DISABLE_DEPRECATED_WARNING
    std::FILE *fp = std::fopen(output_file.string().c_str(), "wb");
    SIMDUTF_POP_DISABLE_WARNINGS
    if (fp == NULL) {
      printf("Could not open %s\n", output_file.string().c_str());
      return;
    }
    run_procedure(fp);
    if(fclose(fp) != 0) {
      printf("Failed to close %s\n", output_file.string().c_str());
    }
  }
}

void CommandLine::run_procedure(std::FILE *fpout) {
  if (from_encoding == "UTF-8") {
    if (to_encoding == "UTF-16LE" || to_encoding == "UTF-16") {
      auto proc = [this, &fpout](size_t size) {
        std::unique_ptr<char16_t[]> output_buffer(new char16_t[size]);
        size_t len = simdutf::convert_utf8_to_utf16le(reinterpret_cast<const char*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char16_t));
      };
      run_simdutf_procedure(proc);
    } else if (to_encoding == "UTF-16BE") {
      auto proc = [this, &fpout](size_t size) {
        std::unique_ptr<char16_t[]> output_buffer(new char16_t[size]);
        size_t len = simdutf::convert_utf8_to_utf16be(reinterpret_cast<const char*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char16_t));
      };
      run_simdutf_procedure(proc);
    } else if (to_encoding == "UTF-32LE" || to_encoding == "UTF-32") {
      auto proc = [this, &fpout](size_t size) {
        std::unique_ptr<char32_t[]> output_buffer(new char32_t[size]);
        size_t len = simdutf::convert_utf8_to_utf32(reinterpret_cast<const char*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char32_t));
      };
      run_simdutf_procedure(proc);
    } else {
      iconv_fallback(fpout);
    }
  }
  else if (from_encoding == "UTF-16LE" || to_encoding == "UTF-16") {
    if (to_encoding == "UTF-8") {
      auto proc = [this, &fpout](size_t size_bytes) {
        const size_t size = size_bytes / 2;
        std::unique_ptr<char[]> output_buffer(new char[4*size]);
        size_t len = simdutf::convert_utf16le_to_utf8(reinterpret_cast<const char16_t*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char));
      };
      run_simdutf_procedure(proc);
    } else if (to_encoding == "UTF-16BE") {
      auto proc = [this, &fpout](size_t size_bytes) {
        const size_t size = size_bytes / 2;
        std::unique_ptr<char16_t[]> output_buffer(new char16_t[size]);
        simdutf::change_endianness_utf16(reinterpret_cast<const char16_t*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), size);
      };
      run_simdutf_procedure(proc);
    } else if (to_encoding == "UTF-32LE" || to_encoding == "UTF-32") {
      auto proc = [this, &fpout](size_t size_bytes) {
        const size_t size = size_bytes / 2;
        std::unique_ptr<char32_t[]> output_buffer(new char32_t[size]);
        size_t len = simdutf::convert_utf16le_to_utf32(reinterpret_cast<const char16_t*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char32_t));
      };
      run_simdutf_procedure(proc);
    } else {
      iconv_fallback(fpout);
    }
  }
  else if (from_encoding == "UTF-16BE") {
    if (to_encoding == "UTF-8") {
      auto proc = [this, &fpout](size_t size_bytes) {
        const size_t size = size_bytes / 2;
        std::unique_ptr<char[]> output_buffer(new char[3*size]);
        size_t len = simdutf::convert_utf16be_to_utf8(reinterpret_cast<const char16_t*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char));
      };
      run_simdutf_procedure(proc);
    } else if (to_encoding == "UTF-16LE" || to_encoding == "UTF-16") {
      auto proc = [this, &fpout](size_t size_bytes) {
        const size_t size = size_bytes / 2;
        std::unique_ptr<char16_t[]> output_buffer(new char16_t[size]);
        simdutf::change_endianness_utf16(reinterpret_cast<const char16_t*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), size);
      };
      run_simdutf_procedure(proc);
    } else if (to_encoding == "UTF-32LE" || to_encoding == "UTF-32") {
      auto proc = [this, &fpout](size_t size_bytes) {
        const size_t size = size_bytes / 2;
        std::unique_ptr<char32_t[]> output_buffer(new char32_t[size]);
        size_t len = simdutf::convert_utf16be_to_utf32(reinterpret_cast<const char16_t*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char32_t));
      };
      run_simdutf_procedure(proc);
    } else {
      iconv_fallback(fpout);
    }
  }
  else if (from_encoding == "UTF-32LE" || to_encoding == "UTF-32") {
    if (to_encoding == "UTF-8") {
      auto proc = [this, &fpout](size_t size_bytes) {
          const size_t size = size_bytes / 4;
          std::unique_ptr<char[]> output_buffer(new char[4*size]);
          size_t len = simdutf::convert_utf32_to_utf8(reinterpret_cast<const char32_t*>(input_data.data()), size, output_buffer.get());
          write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char));
      };
      run_simdutf_procedure(proc);
    } else if (to_encoding == "UTF-16LE" || to_encoding == "UTF-16") {
      auto proc = [this, &fpout](size_t size_bytes) {
        const size_t size = size_bytes / 4;
        std::unique_ptr<char16_t[]> output_buffer(new char16_t[2*size]);
        size_t len = simdutf::convert_utf32_to_utf16le(reinterpret_cast<const char32_t*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char16_t));
      };
      run_simdutf_procedure(proc);
    } else if (to_encoding == "UTF-16BE") {
      auto proc = [this, &fpout](size_t size_bytes) {
        const size_t size = size_bytes / 4;
        std::unique_ptr<char16_t[]> output_buffer(new char16_t[2*size]);
        size_t len = simdutf::convert_utf32_to_utf16be(reinterpret_cast<const char32_t*>(input_data.data()), size, output_buffer.get());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), len * sizeof(char16_t));
      };
      run_simdutf_procedure(proc);
    } else {
      iconv_fallback(fpout);
    }
  }
  else {
    iconv_fallback(fpout);
  }
}

template <typename PROCEDURE>
void CommandLine::run_simdutf_procedure(PROCEDURE proc) {
  while(!(input_files.empty())) {
    size_t input_size{0};
    if(!load_data(CHUNK_SIZE, &input_size)) { printf("Could not load %s\n", input_files.front().string().c_str()); input_files.pop();  continue; }
    proc(input_size);
  }
}

void CommandLine::iconv_fallback(std::FILE *fpout) {
  #if ICONV_AVAILABLE
  iconv_t cv = iconv_open(to_encoding.c_str(), from_encoding.c_str());
  if (cv == (iconv_t)(-1)) {
    fprintf( stderr,"[iconv] cannot initialize %s to %s converter\n", from_encoding.c_str(), to_encoding.c_str());
    return;
  }
  while (!(input_files.empty())) {
    size_t inbytes{0};
    if(!load_data(CHUNK_SIZE, &inbytes)) { printf("Could not load %s\n", input_files.front().string().c_str()); input_files.pop();  continue; }
    size_t outbytes = sizeof(uint32_t) * inbytes;
    size_t start_outbytes = outbytes;
    std::unique_ptr<char[]> output_buffer(new char[outbytes]);
    char * inptr = reinterpret_cast<char *>(input_data.data());
    char * outptr = reinterpret_cast<char *>(output_buffer.get());
    size_t result = iconv(cv, &inptr, &inbytes, &outptr, &outbytes);
    if (result == static_cast<size_t>(-1)) {
      fprintf( stderr,"[iconv] Error iconv.\n");
      return;
    }

    write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.get()), start_outbytes - outbytes);
  }
  iconv_close(cv);
  #else
  fprintf( stderr, "Conversion from %s to %s is not supported by the simdutf library and iconv is not available.\n", from_encoding.c_str(), to_encoding.c_str());
  show_formats();
  #endif
}


bool CommandLine::write_to_file_descriptor(std::FILE *fp, const char * data, size_t length) {
  if(fp == NULL) { return false; }
  size_t bytes_written = std::fwrite(data, 1, length, fp);
  if (bytes_written != length) { return false; }
  return true;
}

bool CommandLine::load_data(size_t count, size_t *input_size) {
  while (count > 0) {
    // Open a file if no file is opened
    if (current_file == NULL) {
      SIMDUTF_PUSH_DISABLE_WARNINGS
      SIMDUTF_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
      current_file = std::fopen(input_files.front().string().c_str(), "rb");
      SIMDUTF_POP_DISABLE_WARNINGS

      if (current_file == NULL) { return false; }
    }

    // Try to read 'count' bytes
    size_t bytes_read = std::fread(input_data.data() + *input_size, 1, count, current_file);
    if (std::ferror(current_file)) { return false; }

    count -= bytes_read;  // 'count' should never be negative since we read at most 'count' bytes
    *input_size += bytes_read;  // input_size should never exceed count initial value
    if (std::feof(current_file)) {  // Check if current_file is done
      if (std::fclose(current_file) != 0) { return false; }
      input_files.pop();
      current_file = NULL;
      if (input_files.empty()) { break; }
    }
  }
  return true;
}

void CommandLine::show_help() {
  printf("Usage: sutf [OPTION...] [-f encoding] [-t encoding] [inputfile ...]\n");
}

void CommandLine::show_formats() {
  printf("UTF-8 UTF-16LE UTF-16BE UTF-32LE\n");
}

int main(int argc, char* argv[]) {
  try {
    CommandLine cmdline = parse_and_validate_arguments(argc, argv);
    cmdline.run();
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
      printf("%s\n", e.what());
      return EXIT_FAILURE;
  }
}