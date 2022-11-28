

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

#include <ztd/cuneicode/shared/stream_helpers.hpp>

#include <set>
#include <iostream>

std::ostream& operator<<(std::ostream& stream, const utf8string_view& str) {
	stream.write(reinterpret_cast<const char*>(str.data()), str.size());
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const utf8string& str) {
	utf8string_view str_view = str;
	return stream << str_view;
}

struct encoding_data {
	utf8string_view name;
	bool from = false;
	bool to   = false;

	friend constexpr bool operator==(
	     const encoding_data& left, const encoding_data& right) noexcept {
		return left.name == right.name;
	}

	friend constexpr bool operator<(
	     const encoding_data& left, const encoding_data& right) noexcept {
		return left.name < right.name;
	}
};

void print_encoding_list(std::ostream& out, cnc_conversion_registry* registry) {
	using user_data_t = std::set<encoding_data>;
	user_data_t existing_conversions {};
	cnc_conversion_registry_pair_c8_function* on_pairing
	     = [](size_t from_size, const ztd_char8_t* from, size_t to_size, const ztd_char8_t* to,
	            void* user_data) {
		       user_data_t& existing_conversions = *static_cast<user_data_t*>(user_data);
		       const encoding_data& from_it
		            = *existing_conversions.insert({ utf8string_view(from, from_size) }).first;
		       const encoding_data& to_it
		            = *existing_conversions.insert({ utf8string_view(to, to_size) }).first;
		       const_cast<encoding_data&>(from_it).from = true;
		       const_cast<encoding_data&>(to_it).to     = true;
	       };
	out << "Available encodings:" << std::endl;
	cnc_pairs_c8_list(registry, on_pairing, &existing_conversions);
	if (existing_conversions.empty()) {
		out << "\t (None)" << std::endl;
		return;
	}
	for (const auto& data : existing_conversions) {
		out << "\t" << data.name;
		if (data.from != data.to) {
			if (data.from) {
				out << " [from only]";
			}
			else if (data.to) {
				out << " [to only]";
			}
		}
		out << std::endl;
	}
}

void print_conversion_info(std::ostream& out, cnc_conversion_info info) {
	out << "[info] Opened a conversion from \""
	    << utf8string_view(info.from_code_data, info.from_code_size) << "\" to \""
	    << utf8string_view(info.to_code_data, info.to_code_size) << "\"";
	if (info.is_indirect) {
		out << " (converting indirectly from \""
		    << utf8string_view(info.from_code_data, info.from_code_size) << "\" to \""
		    << utf8string_view(info.indirect_code_data, info.indirect_code_size) << "\" to \""
		    << utf8string_view(info.to_code_data, info.to_code_size) << "\").";
	}
	else {
		out << " (converting directly from \""
		    << utf8string_view(info.from_code_data, info.from_code_size) << "\" to \""
		    << utf8string_view(info.to_code_data, info.to_code_size) << "\").";
	}
	out << std::endl;
}
