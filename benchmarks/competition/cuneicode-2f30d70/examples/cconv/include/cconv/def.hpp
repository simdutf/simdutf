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

#ifndef ZTD_CUNEICODE_EXAMPLES_CCONV_DEF_HPP
#define ZTD_CUNEICODE_EXAMPLES_CCONV_DEF_HPP

#include <ztd/cuneicode.h>

#include <ztd/idk/char_traits.hpp>

#include <cstddef>
#include <string_view>
#include <string>
#include <iosfwd>

using utf8string      = std::basic_string<ztd_char8_t>;
using utf8string_view = std::basic_string_view<ztd_char8_t>;

std::ostream& operator<<(std::ostream& stream, const utf8string_view& str);
std::ostream& operator<<(std::ostream& stream, const utf8string& str);

constexpr std::size_t minimum_buffer_size = 64;

constexpr int exit_success               = 0;
constexpr int exit_file_write_failure    = 1;
constexpr int exit_file_read_failure     = 2;
constexpr int exit_conversion_failure    = 3;
constexpr int exit_open_failure          = 4;
constexpr int exit_locale_failure        = 5;
constexpr int exit_file_open_failure     = 6;
constexpr int exit_registry_open_failure = 7;
constexpr int exit_option_failure        = 8;

#endif // ZTD_CUNEICODE_EXAMPLES_CCONV_DEF_HPP
