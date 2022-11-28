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

#ifndef ZTD_CUNEICODE_BENCHMARKS_SOURCE_UNICODE_RANGE_HPP
#define ZTD_CUNEICODE_BENCHMARKS_SOURCE_UNICODE_RANGE_HPP

#pragma once

#include <ztd/cuneicode.h>

#include <ztd/tests/basic_unicode_strings.hpp>
#include <ztd/idk/detail/unicode.hpp>

#include <vector>

inline namespace ztd_cnc_shared {
	inline std::vector<ztd_char32_t> make_full_unicode_range() noexcept {
		std::vector<ztd_char32_t> data;
		for (ztd_char32_t expected_c = 0; expected_c < __ztd_idk_detail_last_unicode_code_point;
		     ++expected_c) {
			if (__ztd_idk_detail_is_surrogate(expected_c)) {
				continue;
			}
			data.push_back(expected_c);
		}
		return data;
	}

	inline const std::vector<ztd_char32_t>& full_unicode_range() noexcept {
		static const auto range = make_full_unicode_range();
		return range;
	}

	inline std::vector<ztd_char32_t> make_basic_source_range() noexcept {
		std::vector<ztd_char32_t> data(ztd::tests::u32_basic_source_character_set.begin(),
		     ztd::tests::u32_basic_source_character_set.end());
		return data;
	}

	inline const std::vector<ztd_char32_t>& basic_source_range() noexcept {
		static const auto range = make_basic_source_range();
		return range;
	}
} // namespace ztd_cnc_shared

#endif // ZTD_CUNEICODE_BENCHMARKS_SOURCE_UNICODE_RANGE_HPP
