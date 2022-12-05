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

#include <cconv/options.hpp>

std::optional<std::string> parse_options(options& opt, int argc, char* argv[]) {
	bool options_parsing       = true;
	std::size_t args_size      = static_cast<std::size_t>(argc);
	std::size_t args_max_index = static_cast<std::size_t>(args_size - 1);
	for (std::size_t arg_index = 1; arg_index < args_size; ++arg_index) {
		std::string_view arg = argv[arg_index];
		utf8string_view u8arg(
		     reinterpret_cast<const ztd_char8_t*>(arg.data()), arg.size());
		if (arg.size() < 1) {
			continue;
		}
		if (options_parsing) {
			if (arg == "-o" || arg == "--output-file") {
				// next argument is the target
				if (arg_index == args_max_index) {
					return "[error] Output file option argument is "
					       "missing an additional "
					       "argument specifying the encoding code.";
				}
				++arg_index;
				std::string_view arg_value = argv[arg_index];
				utf8string_view u8arg_value(
				     reinterpret_cast<const ztd_char8_t*>(arg_value.data()),
				     arg_value.size());
				opt.maybe_output_file_name.emplace(u8arg_value);
			}
			else if (arg.find("-o=") == 0) {
				u8arg.remove_prefix(3);
				opt.maybe_output_file_name.emplace(u8arg);
			}
			else if (arg.find("--output-file=") == 0) {
				u8arg.remove_prefix(14);
				opt.maybe_output_file_name.emplace(u8arg);
			}
			else if (arg == "-l" || arg == "--list") {
				opt.list_encodings = true;
			}
			else if (arg == "-s" || arg == "--silent") {
				opt.silent = true;
			}
			else if (arg == "-q" || arg == "--quiet") {
				opt.silent = true;
			}
			else if (arg == "-v" || arg == "--version") {
				opt.show_version = true;
			}
			else if (arg == "--verbose") {
				opt.verbose = true;
			}
			else if (arg == "-?" || arg == "--usage" || arg == "--help") {
				opt.show_help = true;
			}
			else if (arg == "-f" || arg == "--from-code") {
				// next argument is the target
				if (arg_index == args_max_index) {
					std::string error
					     = "[error] From encoding code option argument is "
					       "missing an "
					       "additional "
					       "argument specifying the encoding code.";
					return std::optional<std::string>(error);
				}
				++arg_index;
				std::string_view arg_value = argv[arg_index];
				utf8string_view u8arg_value(
				     reinterpret_cast<const ztd_char8_t*>(arg_value.data()),
				     arg_value.size());
				opt.from_code = u8arg_value;
			}
			else if (bool f_arg_shortform = arg.find("-f=") == 0;
			         f_arg_shortform || arg.find("--from-code=") == 0) {
				std::size_t remove_prefix = f_arg_shortform ? 3 : 12;
				u8arg.remove_prefix(remove_prefix);
				opt.from_code = u8arg;
			}
			else if (arg == "-t" || arg == "--to-code") {
				// next argument is the target
				if (arg_index == args_max_index) {
					return "[error] To encoding code option argument is "
					       "missing an additional "
					       "argument specifying the encoding code.";
				}
				++arg_index;
				std::string_view arg_value = argv[arg_index];
				utf8string_view u8arg_value(
				     reinterpret_cast<const ztd_char8_t*>(arg_value.data()),
				     arg_value.size());
				opt.to_code = u8arg_value;
			}
			else if (bool t_arg_shortform = arg.find("-t=") == 0;
			         t_arg_shortform || arg.find("--to-code=") == 0) {
				std::size_t remove_prefix = f_arg_shortform ? 3 : 10;
				u8arg.remove_prefix(remove_prefix);
				opt.to_code = u8arg;
			}
			else if (bool c_arg_shortform = arg.find("-c=") == 0;
			         c_arg_shortform || arg.find("--cache-size=") == 0) {
				std::size_t remove_prefix = c_arg_shortform ? 3 : 13;
				u8arg.remove_prefix(remove_prefix);
				std::optional<unsigned long long> maybe_value
				     = parse_unsigned_integer(u8arg);
				if (!maybe_value.has_value()) {
					std::string error
					     = "[error] Cache buffer size option argument \"";
					error.append(u8arg.data(), u8arg.data() + u8arg.size());
					error.append(
					     "\" is not a proper decimal positive integer "
					     "value.");
					return std::optional<std::string>(error);
				}
				opt.maybe_buffer_size.emplace(
				     (std::max)(static_cast<std::size_t>(minimum_buffer_size),
				          static_cast<std::size_t>(maybe_value.value())));
			}
			else if (arg.find("--cache-size") == 0 || arg.find("-c") == 0) {
				// next argument is the target
				if (arg_index == args_max_index) {
					std::string error
					     = "[error] Cache buffer size option argument is "
					       "missing an "
					       "additional "
					       "argument specifying the cache size.";
					return std::optional<std::string>(error);
				}
				++arg_index;
				std::string_view arg_value = argv[arg_index];
				utf8string_view u8arg_value(
				     reinterpret_cast<const ztd_char8_t*>(arg_value.data()),
				     arg_value.size());
				std::optional<unsigned long long> maybe_value
				     = parse_unsigned_integer(u8arg_value);
				if (!maybe_value.has_value()) {
					std::string error
					     = "[error] Cache buffer size option argument \"";
					error.append(u8arg_value.data(),
					     u8arg_value.data() + u8arg_value.size());
					error.append(
					     "\" is not a proper decimal positive integer "
					     "value.");
					return std::optional<std::string>(error);
				}
				opt.maybe_buffer_size.emplace(
				     (std::max)(static_cast<std::size_t>(minimum_buffer_size),
				          static_cast<std::size_t>(maybe_value.value())));
			}
			else if (arg == "-d" || arg == "--discard-on-failure") {
				opt.error_handler = discard_handler();
			}
			else if (arg == "-r" || arg == "--report-on-failure") {
				opt.error_handler = fail_handler();
			}
			else if (arg == "-b" || arg == "--byte-substitution") {
				byte_substitution_handler handler {};
				// next argument is the target
				if (arg_index == args_max_index) {
					std::string error
					     = "[error] Cache buffer size option argument is "
					       "missing an "
					       "additional "
					       "argument specifying the cache size.";
					return std::optional<std::string>(error);
				}
				++arg_index;
				std::string_view arg_value = argv[arg_index];
				handler.substitution.assign(
				     arg_value.data(), arg_value.data() + arg_value.size());
				opt.error_handler = std::move(handler);
			}
			else if (bool b_arg_shortform = arg.find("-b=") == 0;
			         b_arg_shortform || arg.find("--byte-substitution=") == 0) {
				std::size_t remove_prefix = b_arg_shortform ? 3 : 20;
				u8arg.remove_prefix(remove_prefix);
				byte_substitution_handler handler {};
				handler.substitution.assign(
				     u8arg.data(), u8arg.data() + u8arg.size());
				opt.error_handler = std::move(handler);
			}
			else {
				// rewind 1 argument and continue from there
				// with non-option parsing
				options_parsing = false;
				arg_index -= 1;
			}
		}
		else {
			if (arg == "-") {
				// interpret as stdin
				opt.input_files.push_back(stdin_read);
			}
			else {
				// interpret as a file
				opt.input_files.push_back(
				     utf8string(u8arg.data(), u8arg.size()));
			}
		}
	}
	return std::optional<std::string>(std::nullopt);
}
