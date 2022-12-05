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

#include <cconv/io.hpp>

#include <ztd/idk/type_traits.hpp>

#include <iostream>
#include <fstream>
#include <optional>
#include <utility>

input_read read_input_into(std::vector<unsigned char>& input,
     const input_type& any_input, bool verbose, bool silent) {
	utf8string input_file_name;
	auto input_visitor = [&](auto&& arg) -> std::optional<int> {
		using arg_type = ztd::remove_cvref_t<decltype(arg)>;
		std::optional<std::ifstream> maybe_input_stream = std::nullopt;
		if constexpr (std::is_same_v<utf8string, arg_type>) {
			input_file_name       = arg;
			std::string file_name = std::string(
			     reinterpret_cast<const char*>(arg.data()), arg.size());
			if (file_name.find('\0') != std::string::npos) {
				if (!silent) {
					std::cerr << "[error] File name contains embedded "
					             "null: cannot open input "
					             "file \""
					          << input_file_name << "\" properly."
					          << std::endl;
				}
				return { exit_file_open_failure };
			}
			maybe_input_stream.emplace(file_name.c_str(),
			     static_cast<std::ios_base::openmode>(std::ios_base::beg)
			          | static_cast<std::ios_base::openmode>(
			               std::ios_base::binary));
			std::ifstream& input_stream = *maybe_input_stream;
			if (!input_stream.is_open() || !input_stream.good()) {
				if (!silent) {
					std::cerr << "[error] Failure to open input file \""
					          << input_file_name << "\"." << std::endl;
				}
				return { exit_file_open_failure };
			}
		}
		else if constexpr (std::is_same_v<stdin_read_tag, arg_type>) {
			input_file_name.assign(
			     reinterpret_cast<const ztd_char8_t*>(+"<stdin>"), 7);
			if (verbose) {
				std::cout << "[info] Attempting to read bytes from \""
				          << input_file_name << "\"." << std::endl;
			}
		}
		else {
			static_assert(std::is_same_v<utf8string, arg_type>,
			     "this type is not processed in the visitor");
			(void)arg;
		}
		std::istream& input_stream
		     = maybe_input_stream.has_value() ? *maybe_input_stream : std::cin;
		std::size_t input_read = 0;
		while (true) {
			input.resize(input_read == 0 ? 1024 : (input.size() * 2));
			const std::size_t requested_read_size = input.size() - input_read;
			input_stream.read(
			     reinterpret_cast<char*>(input.data() + input_read),
			     requested_read_size);
			const std::streamsize last_input_read = input_stream.gcount();
			const bool is_successful_read = static_cast<bool>(input_stream);
			const bool is_specific_istream_read_eof
			     = (input_stream.eof() && input_stream.fail()
			          && (static_cast<std::size_t>(last_input_read)
			               <= requested_read_size));
			if (is_successful_read || is_specific_istream_read_eof) {
				input_read += static_cast<std::size_t>(last_input_read);
				if (is_specific_istream_read_eof) {
					// explicit stop: everything is fine!
					break;
				}
			}
			else {
				if (!silent) {
					std::cerr << "[error] Failure to read data from \""
					          << input_file_name << "\"." << std::endl;
				}
				return { exit_file_read_failure };
			}
		}
		input.resize(input_read);
		return { std::nullopt };
	};
	auto maybe_exit_value = std::visit(input_visitor, any_input);
	return { std::move(input_file_name), std::move(maybe_exit_value) };
}

std::optional<unsigned long long> parse_unsigned_integer(
     utf8string_view value, int base) {
	std::string c_string_value(
	     reinterpret_cast<const char*>(value.data()), value.size());
	try {
		unsigned long long parsed_value
		     = std::stoull(c_string_value, nullptr, base);
		return std::optional<unsigned long long>(parsed_value);
	}
	catch (...) {
	}
	return std::nullopt;
}
