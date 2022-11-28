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

#ifndef ZTD_CUNEICODE_EXAMPLES_CCONV_OPTIONS_HPP
#define ZTD_CUNEICODE_EXAMPLES_CCONV_OPTIONS_HPP

#include <cconv/handler.hpp>
#include <cconv/def.hpp>
#include <cconv/io.hpp>

#include <variant>
#include <optional>
#include <vector>

struct options {
	using some_handler
	     = std::variant<discard_handler, fail_handler, byte_substitution_handler>;
	bool list_encodings                              = false;
	bool show_version                                = false;
	bool verbose                                     = false;
	bool show_help                                   = false;
	std::optional<utf8string> maybe_output_file_name = std::nullopt;
	bool silent                                      = false;
	utf8string from_code = reinterpret_cast<const ztd_char8_t*>(+u8"execution");
	utf8string to_code   = reinterpret_cast<const ztd_char8_t*>(+"utf-8");
	some_handler error_handler                   = discard_handler();
	std::vector<input_type> input_files          = {};
	std::optional<std::size_t> maybe_buffer_size = std::nullopt;
};

std::optional<std::string> parse_options(options& opt, int argc, char* argv[]);

#endif // ZTD_CUNEICODE_EXAMPLES_CCONV_OPTIONS_HPP
