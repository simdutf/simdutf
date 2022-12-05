// ============================================================================
//
// ztd.cuneicode
// Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.cuneicode licenses may use this file
// in accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// 		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ========================================================================= //

#include <ztd/cuneicode.h>

#include <cconv/handler.hpp>
#include <cconv/def.hpp>
#include <cconv/usage.hpp>
#include <cconv/io.hpp>
#include <cconv/options.hpp>

#include <ztd/idk/utf8_startup_hook.hpp>
#include <ztd/idk/size.h>

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <set>
#include <clocale>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

ztd::utf8_startup_hook utf8_startup {};

struct cnc_conversion_deleter {
	void operator()(cnc_conversion* handle) const {
		cnc_conv_delete(handle);
	}
};

struct cnc_registry_deleter {
	void operator()(cnc_conversion_registry* handle) const {
		cnc_registry_delete(handle);
	}
};

int main(int argc, char* argv[]) {

	std::size_t total_input_read     = 0;
	std::size_t total_output_written = 0;
	options opt;
#if ZTD_IS_OFF(ZTD_DEBUG)
	try {
#endif // top-level try
		std::optional<std::string> maybe_error_message
		     = parse_options(opt, argc, argv);
		if (maybe_error_message.has_value()) {
			if (!opt.silent) {
				const std::string& err = *maybe_error_message;
				std::cerr << err << std::endl;
			}
			return exit_option_failure;
		}

		if (opt.input_files.empty()) {
			opt.input_files.push_back(stdin_read);
		}

		if (opt.show_version) {
			print_version();
			return exit_success;
		}

		if (opt.show_help) {
			print_help();
			return exit_success;
		}

		if (opt.verbose) {
			std::cout << "[info] cconv v0.0.0 invoked with " << argc
			          << " arguments." << std::endl;
			std::cout << "[info] Current working directory is \""
			          << std::filesystem::current_path().string() << "\"."
			          << std::endl;
		}
		if (opt.verbose && argc > 0) {
			std::cout << "[info] 1st command line argument is \"" << argv[0]
			          << "\"." << std::endl;
		}

		// open the registry
		std::unique_ptr<cnc_conversion_registry, cnc_registry_deleter> registry
		     = nullptr;
		{
			cnc_conversion_registry* raw_registry = registry.get();
			cnc_registry_options registry_options
			     = CNC_REGISTRY_OPTIONS_DEFAULT;
			cnc_open_error err
			     = cnc_registry_new(&raw_registry, registry_options);
			if (err != CNC_OPEN_ERROR_OK) {
				if (!opt.silent) {
					std::cerr << "[error] Could not open a conversion "
					             "registry (error code: "
					          << err << ")." << std::endl;
				}
				return exit_registry_open_failure;
			}
			registry.reset(raw_registry);
		}

		// if we are just listing encodings, do so and then stop here.
		if (opt.list_encodings) {
			print_encoding_list(registry.get());
			return exit_success;
		}

		// open up the conversion specifier
		std::unique_ptr<cnc_conversion, cnc_conversion_deleter> conversion
		     = nullptr;
		cnc_conversion_info info = {};
		{
			cnc_conversion* raw_conversion      = conversion.get();
			std::size_t from_size               = opt.from_code.size();
			const ztd_char8_t* from_data        = opt.from_code.data();
			std::size_t to_size                 = opt.to_code.size();
			const ztd_char8_t* to_data          = opt.to_code.data();
			cnc_conversion_registry* __registry = registry.get();
			cnc_open_error err = cnc_conv_new_c8n(__registry, from_size,
			     from_data, to_size, to_data, &raw_conversion, &info);
			if (err != CNC_OPEN_ERROR_OK) {
				if (!opt.silent) {
					std::cerr << "[error] Could not open a conversion from \""
					          << opt.from_code << "\" to \"" << opt.to_code
					          << "\"." << std::endl;
				}
				return exit_open_failure;
			}
			if (opt.verbose) {
				std::cout
				     << "[info] Opened a conversion from \""
				     << utf8string_view(
				             info.from_code_data, info.from_code_size)
				     << "\" to \""
				     << utf8string_view(info.to_code_data, info.to_code_size)
				     << "\"";
				if (info.is_indirect) {
					std::cout << " (converting indirectly from \""
					          << utf8string_view(info.from_code_data,
					                  info.from_code_size)
					          << "\" to \""
					          << utf8string_view(info.indirect_code_data,
					                  info.indirect_code_size)
					          << "\" to \""
					          << utf8string_view(
					                  info.to_code_data, info.to_code_size)
					          << "\").";
				}
				else {
					std::cout << " (converting directly from \""
					          << utf8string_view(info.from_code_data,
					                  info.from_code_size)
					          << "\" to \""
					          << utf8string_view(
					                  info.to_code_data, info.to_code_size)
					          << "\").";
				}
				std::cout << std::endl;
			}
			conversion.reset(raw_conversion);
		}

		// make sure we have an output to write to
		std::optional<std::ofstream> maybe_file_output;
		utf8string output_file_name;
		if (opt.maybe_output_file_name) {
			output_file_name      = *opt.maybe_output_file_name;
			std::string file_name = std::string(
			     reinterpret_cast<const char*>(output_file_name.data()),
			     output_file_name.size());
			if (file_name.find('\0') != std::string::npos) {
				if (!opt.silent) {
					std::cerr << "[error] File name contains embedded "
					             "null: cannot open "
					             "output file \""
					          << output_file_name << "\" properly."
					          << std::endl;
				}
				return exit_file_open_failure;
			}
			std::ofstream target_output(
			     file_name.c_str(), std::ios::binary | std::ios::app);
			if (!target_output.is_open() || !target_output.good()) {
				if (!opt.silent) {
					std::cerr << "[error] Could not open output file "
					             "(binary, append) \""
					          << output_file_name << "\"." << std::endl;
				}
				return exit_file_open_failure;
			}
			maybe_file_output.emplace(std::move(target_output));
		}
		else {
			output_file_name.assign(
			     reinterpret_cast<const ztd_char8_t*>(+"<stdout>"), 8);
		}
		std::ostream& output_stream
		     = maybe_file_output.has_value() ? *maybe_file_output : std::cout;
		if (opt.verbose) {
			std::cout << "[info] Writing to \"" << output_file_name << "\"."
			          << std::endl;
			std::cout << "[info] Attempting to convert from \"" << opt.from_code
			          << "\" to \"" << opt.to_code << "\" for "
			          << opt.input_files.size() << " inputs." << std::endl;
		}

		std::size_t buffer_size = opt.maybe_buffer_size.value_or(
		     ZTD_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_);
		std::vector<unsigned char> input {};
		std::vector<unsigned char> output(buffer_size);
		unsigned char
		     error_output[ZTD_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_] {};
		unsigned char* const initial_error_output_data = error_output;
		const size_t initial_error_output_size = ztd_c_array_size(error_output);
		{
			cnc_conversion* raw_conversion = conversion.get();
			for (std::size_t i = 0; i < opt.input_files.size(); ++i) {
				input_type& input_file_or_stdin = opt.input_files[i];
				auto success                    = read_input_into(
				                        input, input_file_or_stdin, opt.verbose, opt.silent);
				if (success.maybe_return_code) {
					return *success.maybe_return_code;
				}
				const utf8string& input_file_name = success.input_file_name;
				if (opt.verbose) {
					std::cout << "[info] Read " << input.size()
					          << " bytes from \"" << input_file_name << "\"."
					          << std::endl;
				}
				const unsigned char* const initial_input_data = input.data();
				const size_t initial_input_size               = input.size();
				total_input_read += initial_input_size;
				const unsigned char* input_data          = initial_input_data;
				size_t input_size                        = initial_input_size;
				unsigned char* const initial_output_data = output.data();
				const std::size_t initial_output_size    = output.size();
				for (; input_size > 0;) {
					unsigned char* output_data       = initial_output_data;
					std::size_t output_size          = initial_output_size;
					std::size_t error_output_written = 0;
					const cnc_mcerror convert_err
					     = cnc_conv(raw_conversion, &output_size,
					          &output_data, &input_size, &input_data);
					const std::size_t output_written
					     = initial_output_size - output_size;
					switch (convert_err) {
					case CNC_MCERROR_INCOMPLETE_INPUT:
						if (!opt.silent) {
							std::cerr << "[error] Could not convert "
							             "input data from \""
							          << opt.from_code << "\" to \""
							          << opt.to_code
							          << "\" because the input data is "
							             "incomplete."
							          << std::endl;
						}
						return exit_conversion_failure;
					case CNC_MCERROR_INVALID_SEQUENCE: {
						unsigned char* error_output_data
						     = initial_error_output_data;
						size_t error_output_size = initial_error_output_size;
						auto handler_visitor     = [&](auto&& arg) {
                                   return arg(info, raw_conversion,
							         &error_output_size, &error_output_data,
							         &input_size, &input_data);
						};
						// okay, handle it with the handler
						bool can_continue_processing = std::visit(
						     handler_visitor, opt.error_handler);
						if (!can_continue_processing) {
							if (!opt.silent) {
								std::cerr << "[error] Could not convert "
								             "input data from \""
								          << opt.from_code << "\" to \""
								          << opt.to_code << "\"."
								          << std::endl;
							}
							return exit_conversion_failure;
						}
						error_output_written
						     = initial_error_output_size - error_output_size;
					} break;
					case CNC_MCERROR_OK:
					case CNC_MCERROR_INSUFFICIENT_OUTPUT:
					default:
						break;
					}
					// write bytes exactly by using `.write` instead of
					// formatted I/O
					const std::size_t combined_written
					     = (output_written + error_output_written);
					const bool must_write_output = output_written != 0;
					const bool must_write_error_output
					     = error_output_written != 0;
					if (must_write_output) {
						output_stream.write(reinterpret_cast<const char*>(
						                         initial_output_data),
						     static_cast<std::streamsize>(output_written));
						if (!output_stream) {
							if (!opt.silent) {
								std::cerr << "[error] Could not write "
								             "data to output file (\""
								          << output_file_name << "\")."
								          << std::endl;
							}
							return exit_file_write_failure;
						}
						output_stream.flush();
						if (!output_stream) {
							if (!opt.silent) {
								std::cerr << "[error] Could not flush "
								             "data to output file (\""
								          << output_file_name << "\")."
								          << std::endl;
							}
							return exit_file_write_failure;
						}
						if (opt.verbose) {
							std::cout << "[info] Wrote " << output_written
							          << " bytes of output to \""
							          << output_file_name << "\" with "
							          << input_size
							          << " bytes left to read."
							          << std::endl;
						}
					}
					if (must_write_error_output) {
						output_stream.write(reinterpret_cast<const char*>(
						                         initial_error_output_data),
						     error_output_written);
						if (!output_stream) {
							if (!opt.silent) {
								std::cerr << "[error] Could not write "
								             "data produced by the "
								             "error handler to "
								             "output file (\""
								          << output_file_name << "\")."
								          << std::endl;
							}
							return exit_file_write_failure;
						}
						output_stream.flush();
						if (!output_stream) {
							if (!opt.silent) {
								std::cerr << "[error] Could not flush "
								             "data to output file (\""
								          << output_file_name << "\")."
								          << std::endl;
							}
							return exit_file_write_failure;
						}
						if (opt.verbose) {
							std::cout
							     << "[info] Wrote " << error_output_written
							     << " bytes of error to \""
							     << output_file_name << "\" with "
							     << input_size << " bytes left to read."
							     << std::endl;
						}
					}
					total_output_written += combined_written;
				}
			}
		}
#if ZTD_IS_OFF(ZTD_DEBUG)
	}
	catch (...) {
		if (!opt.silent)
			std::cerr << "[error] An explosive, unexpected error "
			             "(exception) occurred and the "
			             "program must terminate."
			          << std::endl;
		return 5;
	}
#endif // top-level try
	if (opt.verbose) {
		std::cout << "[info] cconv completed successfully. Read a total of "
		          << total_input_read << " bytes and wrote a total of "
		          << total_output_written << " bytes" << std::endl;
	}
	return exit_success;
}
