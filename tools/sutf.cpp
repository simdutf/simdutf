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
    run_procedure(stdout);
  } else {
    SIMDUTF_DISABLE_DEPRECATED_WARNING 
    std::FILE *fp = std::fopen(output_file.string().c_str(), "wb");
    SIMDUTF_POP_DISABLE_WARNINGS
    if (fp == NULL) {
      printf("Could not open %s\n",output_file.string().c_str());
      return;
    }
    run_procedure(fp);
    if(fclose(fp) != 0) {
      printf("Failed to close %s\n",output_file.string().c_str());
    }
  }
}

void CommandLine::run_procedure(std::FILE *fpout) {
  if (from_encoding == "UTF-8") {
    if (to_encoding == "UTF-16LE" || to_encoding == "UTF-16") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str()); continue; }
        const char* data = reinterpret_cast<const char*>(input_data.data());
        const size_t size = input_data.size();
        std::vector<char16_t> output_buffer(size);
        size_t len = simdutf::convert_utf8_to_utf16le(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char16_t));
      }
    } else if (to_encoding == "UTF-16BE") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char* data = reinterpret_cast<const char*>(input_data.data());
        size_t size = input_data.size();
        std::vector<char16_t> output_buffer(size);
        size_t len = simdutf::convert_utf8_to_utf16be(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char16_t));
      }
    } else if (to_encoding == "UTF-32LE") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char* data = reinterpret_cast<const char*>(input_data.data());
        size_t size = input_data.size();
        std::vector<char32_t> output_buffer(size);
        size_t len = simdutf::convert_utf8_to_utf32(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char32_t));
      }
    } else {
      iconv_fallback();
    }
  }
  else if (from_encoding == "UTF-16LE" || to_encoding == "UTF-16") {
    if (to_encoding == "UTF-8") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char> output_buffer(2*size);
        size_t len = simdutf::convert_utf16le_to_utf8(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char));
      }
    } else if (to_encoding == "UTF-16BE") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char16_t> output_buffer(size);
        simdutf::change_endianness_utf16(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), size);
      }
    } else if (to_encoding == "UTF-32LE") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char32_t> output_buffer(size);
        size_t len = simdutf::convert_utf16le_to_utf32(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char32_t));
      }
    } else {
      printf("UNSUPPORTED");
    }
  }
  else if (from_encoding == "UTF-16BE") {
    if (to_encoding == "UTF-8") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char> output_buffer(2*size);
        size_t len = simdutf::convert_utf16be_to_utf8(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char));
      }
    } else if (to_encoding == "UTF-16LE" || to_encoding == "UTF-16") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char16_t> output_buffer(size);
        simdutf::change_endianness_utf16(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), size);
      }
    } else if (to_encoding == "UTF-32LE") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char16_t* data = reinterpret_cast<const char16_t*>(input_data.data());
        const size_t size = input_data.size() / 2;
        std::vector<char32_t> output_buffer(size);
        size_t len = simdutf::convert_utf16be_to_utf32(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char32_t));
      }
    } else {
      iconv_fallback();
    }
  }
  else if (from_encoding == "UTF-32LE") {
    if (to_encoding == "UTF-8") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data());
        const size_t size = input_data.size() / 4;
        std::vector<char> output_buffer(4*size);
        size_t len = simdutf::convert_utf32_to_utf8(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char));
      }
    } else if (to_encoding == "UTF-16LE" || to_encoding == "UTF-16") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data());
        const size_t size = input_data.size() / 4;
        std::vector<char16_t> output_buffer(2*size);
        size_t len = simdutf::convert_utf32_to_utf16le(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char16_t));
      }
    } else if (to_encoding == "UTF-16BE") {
      for (auto file : input_files) {
        if(!load_file(file)) { printf("Could not load %s\n", file.string().c_str());  continue; }
        const char32_t* data = reinterpret_cast<const char32_t*>(input_data.data());
        const size_t size = input_data.size() / 4;
        std::vector<char16_t> output_buffer(2*size);
        size_t len = simdutf::convert_utf32_to_utf16be(data, size, output_buffer.data());
        write_to_file_descriptor(fpout, reinterpret_cast<char *>(output_buffer.data()), len * sizeof(char16_t));
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
  std::string command("iconv");
  if (!output_file.empty()) {
    command += " -o ";
    command += output_file.string();
  }
  command += " -f " + from_encoding + " -t " + to_encoding;
  for (auto file : input_files) {
    command += " " + file.string();
  }

  if (system(command.c_str())) {
    throw std::runtime_error("Error with iconv.");
  }
}


bool CommandLine::write_to_file_descriptor(std::FILE *fp, const char * data, size_t length) {
  if(fp == NULL) { return false; }
  size_t bytes_written = std::fwrite(data, 1, length, fp);
  if (bytes_written != length) { return false; }
  return true;
}

bool CommandLine::load_file(const std::filesystem::path& path) {

  SIMDUTF_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = std::fopen(path.string().c_str(), "rb");
  SIMDUTF_POP_DISABLE_WARNINGS

  if (fp == NULL) { return false; }

  // Get the file size
  if(std::fseek(fp, 0, SEEK_END) < 0) {
    std::fclose(fp);
    return false;
  }
#if defined(SIMDUTF_VISUAL_STUDIO) && !SIMDUTF_IS_32BITS
  __int64 file_size_in_bytes = _ftelli64(fp);
  if(file_size_in_bytes == -1L) {
    std::fclose(fp);
    return false;
  }
#else
  long file_size_in_bytes = std::ftell(fp);
  if((file_size_in_bytes < 0) || (file_size_in_bytes == LONG_MAX)) {
    std::fclose(fp);
    return false;
  }
#endif

  // Allocate the memory, we zero the buffer through resize
  // but that's inconsequential for newly allocated memory.
  size_t length = static_cast<size_t>(file_size_in_bytes);
  input_data.resize(static_cast<size_t>(length));

  std::rewind(fp);
  size_t bytes_read = std::fread(input_data.data(), 1, length, fp);
  if (std::fclose(fp) != 0 || bytes_read != length) { return false; }
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