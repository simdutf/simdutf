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

#include <cconv/handler.hpp>

#include <ztd/idk/endian.hpp>
#include <ztd/idk/assert.hpp>
#include <ztd/idk/encoding_name.hpp>
#include <ztd/idk/detail/unicode.h>

#include <algorithm>

bool byte_substitution_handler::operator()(const cnc_conversion_info& info,
     [[maybe_unused]] cnc_conversion* conversion, size_t* p_output_size,
     unsigned char** p_output_data, size_t* p_input_size,
     const unsigned char** p_input_data) const noexcept {
	const std::string_view from_code(
	     reinterpret_cast<const char*>(info.from_code_data), info.from_code_size);
	if (p_input_size != nullptr && *p_input_size != 0) {
		*p_input_size -= 1;
	}
	if (p_input_data != nullptr && *p_input_data != nullptr) {
		*p_input_data += 1;
	}
	if (ztd::is_encoding_name_equal(from_code, "utf8")) {
		while (p_input_size != nullptr && *p_input_size != 0) {
			if (!__ztd_idk_detail_is_single_or_lead_utf8(**p_input_data)) {
				*p_input_size -= 1;
				*p_input_data += 1;
			}
			else {
				break;
			}
		}
	}
	else if (const bool is_utf16
	         = ztd::is_encoding_name_equal(from_code, "utf16"),
	         is_utf16_le = ztd::is_encoding_name_equal(from_code, "utf16le");
	         is_utf16 || is_utf16_le
	         || ztd::is_encoding_name_equal(from_code, "utf16be")) {
		const bool is_le = is_utf16_le
		     || (is_utf16 && (ztd::endian::native == ztd::endian::little));
		const bool is_be = (!is_utf16_le && !is_utf16)
		     || (is_utf16 && (ztd::endian::native == ztd::endian::big));
		ZTD_ASSERT_MESSAGE(
		     "Must be one of big or little endian to properly read or "
		     "serialize this UTF-16 "
		     "data.",
		     is_le || is_be);
		std::size_t skipped = 0;
		while (p_input_size != nullptr && *p_input_size > 1) {
			ztd_char16_t c = 0;
			if (is_utf16_le) {
				c |= (*(*p_input_data + 0) & 0xFF);
				c |= (*(*p_input_data + 1) & 0xFF) << 8;
			}
			else {
				c |= (*(*p_input_data + 0) & 0xFF) << 8;
				c |= (*(*p_input_data + 1) & 0xFF);
			}
			if (__ztd_idk_detail_is_trail_surrogate(c)) {
				*p_input_size -= 2;
				skipped += 2;
			}
			else {
				break;
			}
		}
		*p_input_data += skipped;
	}
	else {
		if (p_input_size != nullptr && *p_input_size != 0) {
			*p_input_size -= 1;
		}
		if (p_input_data != nullptr && *p_input_data != nullptr) {
			*p_input_data += 1;
		}
	}
	if (this->substitution.empty()) {
		return true;
	}
	if (p_output_size && *p_output_size < this->substitution.size()) {
		return false;
	}
	*p_output_size -= this->substitution.size();
	if (p_output_data == nullptr || *p_output_data == nullptr) {
		return true;
	}
	for (const auto& element : this->substitution) {
		**p_output_data = element;
		*p_output_data += 1;
	}
	return true;
}

bool discard_handler::operator()([[maybe_unused]] const cnc_conversion_info& info,
     [[maybe_unused]] cnc_conversion* conversion,
     [[maybe_unused]] size_t* p_output_size,
     [[maybe_unused]] unsigned char** p_output_data,
     [[maybe_unused]] size_t* p_input_size,
     [[maybe_unused]] const unsigned char** p_input_data) const noexcept {
	if (p_input_size != nullptr && *p_input_size != 0) {
		*p_input_size -= 1;
	}
	if (p_input_data != nullptr && *p_input_data != nullptr) {
		*p_input_data += 1;
	}
	return true;
}

bool fail_handler::operator()([[maybe_unused]] const cnc_conversion_info& info,
     [[maybe_unused]] cnc_conversion* conversion,
     [[maybe_unused]] size_t* p_output_size,
     [[maybe_unused]] unsigned char** p_output_data,
     [[maybe_unused]] size_t* p_input_size,
     [[maybe_unused]] const unsigned char** p_input_data) const noexcept {
	return false;
}
