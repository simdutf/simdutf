#include "sutf.h"

#include <vector>
#include <string>
#include <set>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <iterator>
#include <fstream>

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
      cmdline.input_files.insert(arg);
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
    run_procedure(&std::cout);
  } else {
    std::ofstream output(output_file, std::ios::binary | std::ios::trunc);
    run_procedure(&output);
    output.close();
  }
}

void CommandLine::run_procedure(std::ostream* output) {
  if (from_encoding == "UTF-8") {
    if (to_encoding == "UTF-16LE") {
      for (auto file : input_files) {
        load_file(file);
        const char* data = reinterpret_cast<const char*>(input_data.data());
        const size_t size = input_data.size();
        std::vector<char16_t> output_buffer(size);
        size_t len = simdutf::convert_utf8_to_utf16le(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char16_t));
      }
    } else if (to_encoding == "UTF-16BE") {
      for (auto file : input_files) {
        load_file(file);
        const char* data = reinterpret_cast<const char*>(input_data.data());
        size_t size = input_data.size();
        std::vector<char16_t> output_buffer(size);
        size_t len = simdutf::convert_utf8_to_utf16be(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char16_t));
      }
    } else if (to_encoding == "UTF-32LE") {
      for (auto file : input_files) {
        load_file(file);
        const char* data = reinterpret_cast<const char*>(input_data.data());
        size_t size = input_data.size();
        std::vector<char32_t> output_buffer(size);
        size_t len = simdutf::convert_utf8_to_utf32(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char32_t));
      }
    } else {
      iconv_fallback();
    }
  }
  else if (from_encoding == "UTF-16LE") {
    if (to_encoding == "UTF-8") {
      for (auto file : input_files) {
        load_file(file);
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char> output_buffer(2*size);
        size_t len = simdutf::convert_utf16le_to_utf8(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char));
      }
    } else if (to_encoding == "UTF-16BE") {
      for (auto file : input_files) {
        load_file(file);
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char16_t> output_buffer(size);
        simdutf::change_endianness_utf16(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), size);
      }
    } else if (to_encoding == "UTF-32LE") {
      for (auto file : input_files) {
        load_file(file);
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char32_t> output_buffer(size);
        size_t len = simdutf::convert_utf16le_to_utf32(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char32_t));
      }
    } else {
      std::cout << "UNSUPPORTED" << std::endl;
    }
  }
  else if (from_encoding == "UTF-16BE") {
    if (to_encoding == "UTF-8") {
      for (auto file : input_files) {
        load_file(file);
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char> output_buffer(2*size);
        size_t len = simdutf::convert_utf16be_to_utf8(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char));
      }
    } else if (to_encoding == "UTF-16BE") {
      for (auto file : input_files) {
        load_file(file);
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char16_t> output_buffer(size);
        simdutf::change_endianness_utf16(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), size);
      }
    } else if (to_encoding == "UTF-32LE") {
      for (auto file : input_files) {
        load_file(file);
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char32_t> output_buffer(size);
        size_t len = simdutf::convert_utf16be_to_utf32(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char32_t));
      }
    } else {
      iconv_fallback();
    }
  }
  else if (from_encoding == "UTF-32LE") {
    if (to_encoding == "UTF-8") {
      for (auto file : input_files) {
        load_file(file);
        const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data());
        const size_t size = input_data.size() / 4;
        std::vector<char> output_buffer(4*size);
        size_t len = simdutf::convert_utf32_to_utf8(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char));
      }
    } else if (to_encoding == "UTF-16LE") {
      for (auto file : input_files) {
        load_file(file);
        const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data());
        const size_t size = input_data.size() / 4;
        std::vector<char16_t> output_buffer(2*size);
        size_t len = simdutf::convert_utf32_to_utf16le(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char16_t));
      }
    } else if (to_encoding == "UTF-16BE") {
      for (auto file : input_files) {
        load_file(file);
        const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data());
        const size_t size = input_data.size() / 4;
        std::vector<char16_t> output_buffer(2*size);
        size_t len = simdutf::convert_utf32_to_utf16be(data, size, output_buffer.data());
        output->write(reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char16_t));
      }
    } else {
      iconv_fallback();
    }
  }
  else {
    iconv_fallback();
  }
}

void CommandLine::iconv_fallback() {

}

void CommandLine::load_file(const std::filesystem::path& path) {
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  file.open(path);

  input_data.assign(std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>());
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