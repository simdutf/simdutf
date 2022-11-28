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

#ifndef ZTD_CUNEICODE_EXAMPLES_CCONV_IO_HPP
#define ZTD_CUNEICODE_EXAMPLES_CCONV_IO_HPP

#include <cconv/def.hpp>

#include <vector>
#include <variant>
#include <optional>

struct stdin_read_tag {
} inline constexpr stdin_read = {};

using input_type = std::variant<stdin_read_tag, utf8string>;

struct input_read {
	utf8string input_file_name;
	std::optional<int> maybe_return_code;
};

input_read read_input_into(std::vector<unsigned char>& data,
     const input_type& some_input, bool verbose, bool silent);
std::optional<unsigned long long> parse_unsigned_integer(
     utf8string_view value, int base = 10);

#endif // ZTD_CUNEICODE_EXAMPLES_CCONV_IO_HPP
