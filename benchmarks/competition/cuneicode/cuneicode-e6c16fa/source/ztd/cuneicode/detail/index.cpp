// =============================================================================
//
// ztd.cuneicode
// Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.cuneicode licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License") for non-commercial use; you may not use this
// file except in compliance with the License. You may obtain a copy of the
// License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ============================================================================ //

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/detail/index.hpp>

#include <ztd/ranges/adl.hpp>

#include <utility>
#include <algorithm>
#include <cstdint>
#include <optional>

namespace {
	static bool __cnc_less_than_index_target(
	     ::std::pair<::std::uint_least32_t, ::std::uint_least32_t> value,
	     ::std::uint_least32_t target) {
		return value.first < target;
	}

	static bool __cnc_less_than_code_point_target(
	     ::std::pair<::std::uint_least32_t, ::std::uint_least32_t> value, char32_t target) {
		return value.second < static_cast<uint_least32_t>(target);
	}
} // namespace

namespace cnc {
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __cnc_detail {

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<char32_t>
		__gb18030_ranges_index_to_code_point(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     ::std::size_t __lookup_index_pointer) noexcept {
			if ((__lookup_index_pointer > 39419 && __lookup_index_pointer < 189000)
			     || __lookup_index_pointer > 1237575) {
				return ::std::nullopt;
			}
			if (__lookup_index_pointer == 7457) {
				return U'\uE7C7';
			}
			::std::uint_least32_t lookup_index
			     = static_cast<::std::uint_least32_t>(__lookup_index_pointer);
			auto __last = ::ztd::ranges::cend(__index_code_point_map);
			auto __it = ::std::lower_bound(::ztd::ranges::cbegin(__index_code_point_map), __last,
			     lookup_index, &__cnc_less_than_index_target);
			if (__it == __last) {
				return ::std::nullopt;
			}
			const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t> __index_and_codepoint
			     = *__it;
			const ::std::uint_least32_t __offset            = __index_and_codepoint.first;
			const ::std::uint_least32_t __code_point_offset = __index_and_codepoint.second;
			const char32_t __code                           = static_cast<char32_t>(
                    (__code_point_offset + __lookup_index_pointer) - __offset);
			return __code;
		}

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<::std::size_t>
		__gb18030_ranges_code_point_to_index(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     char32_t __code) noexcept {
			if (__code == U'\uE7C7') {
				return 7457;
			}
			auto __last = ::ztd::ranges::cend(__index_code_point_map);
			auto __it = ::std::lower_bound(::ztd::ranges::cbegin(__index_code_point_map), __last,
			     __code, &__cnc_less_than_code_point_target);
			if (__it == __last) {
				return ::std::nullopt;
			}
			const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t> __index_and_codepoint
			     = *__it;
			const ::std::uint_least32_t __offset       = __index_and_codepoint.second;
			const ::std::uint_least32_t __index_offset = __index_and_codepoint.first;
			return static_cast<::std::size_t>((__index_offset + __code) - __offset);
		}

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<char32_t> __general_index_to_code_point(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     ::std::size_t __lookup_index_pointer) noexcept {
			::std::uint_least32_t lookup_index
			     = static_cast<::std::uint_least32_t>(__lookup_index_pointer);
			auto __last = ::ztd::ranges::cend(__index_code_point_map);
			auto __it = ::std::lower_bound(::ztd::ranges::cbegin(__index_code_point_map), __last,
			     lookup_index, &__cnc_less_than_index_target);
			if (__it == __last) {
				return ::std::nullopt;
			}
			const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t> __index_and_codepoint
			     = *__it;
			if (__index_and_codepoint.first != lookup_index) {
				return ::std::nullopt;
			}
			return static_cast<char32_t>(__index_and_codepoint.second);
		}

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<::std::size_t> __general_code_point_to_index(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     char32_t __code) noexcept {
			const auto predicate
			     = [&__code](
			            const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>& value) {
				       return __code == value.second;
			       };
			auto __last = ::ztd::ranges::cend(__index_code_point_map);
			auto __it   = ::std::find_if(
			       ::ztd::ranges::cbegin(__index_code_point_map), __last, predicate);
			if (__it == __last) {
				return ::std::nullopt;
			}
			const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t> __index_and_codepoint
			     = *__it;
			return static_cast<::std::size_t>(__index_and_codepoint.first);
		}

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<::std::size_t>
		__shift_jis_code_point_to_index(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     char32_t __code) noexcept {
			const auto predicate
			     = [&__code](
			            const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>& value) {
				       return __code == value.second
				            && !(value.first > 8272 && value.first < 8835);
			       };
			auto __last = ::ztd::ranges::cend(__index_code_point_map);
			auto __it   = ::std::find_if(
			       ::ztd::ranges::cbegin(__index_code_point_map), __last, predicate);
			if (__it == __last) {
				return ::std::nullopt;
			}
			const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t> __index_and_codepoint
			     = *__it;
			return static_cast<::std::size_t>(__index_and_codepoint.first);
		}

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<::std::size_t>
		__big5_hkscs_code_point_to_index(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     char32_t __code) noexcept {
			const auto predicate
			     = [&__code](
			            const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>& value) {
				       return __code == value.second && !(value.first >= (0xA1 - 0x81) * 157);
			       };
			if (__code == 0x2550 || __code == 0x255E || __code == 0x256A || __code == 0x5341) {
				// must index backwards, to find the last matching code point in the range
				auto __last = ::ztd::ranges::crend(__index_code_point_map);
				auto __it   = ::std::find_if(
				       ::ztd::ranges::crbegin(__index_code_point_map), __last, predicate);
				if (__it == __last) {
					return ::std::nullopt;
				}
				const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>
				     __index_and_codepoint = *__it;
				return static_cast<::std::size_t>(__index_and_codepoint.first);
			}
			else {
				auto __last = ::ztd::ranges::cend(__index_code_point_map);
				auto __it   = ::std::find_if(
				       ::ztd::ranges::cbegin(__index_code_point_map), __last, predicate);
				if (__it == __last) {
					return ::std::nullopt;
				}
				const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>
				     __index_and_codepoint = *__it;
				return static_cast<::std::size_t>(__index_and_codepoint.first);
			}
		}
	} // namespace __cnc_detail

	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace cnc
