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

#pragma once

#ifndef ZTD_CUNEICODE_DETAIL_INDEX_HPP
#define ZTD_CUNEICODE_DETAIL_INDEX_HPP

#include <ztd/cuneicode/version.h>

#include <ztd/idk/span.hpp>

#include <cstddef>
#include <optional>
#include <utility>

namespace cnc {
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __cnc_detail {

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<char32_t> __general_index_to_code_point(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     ::std::size_t __lookup_index_pointer) noexcept;

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<::std::size_t> __general_code_point_to_index(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     char32_t __code) noexcept;

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<char32_t>
		__gb18030_ranges_index_to_code_point(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     ::std::size_t __lookup_index_pointer) noexcept;

		ZTD_CUNEICODE_API_LINKAGE_I_ ::std::optional<::std::size_t>
		__gb18030_ranges_code_point_to_index(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     char32_t __code) noexcept;

		ZTD_CUNEICODE_API_LINKAGE_I_
		::std::optional<::std::size_t> __shift_jis_code_point_to_index(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     char32_t __code) noexcept;

		ZTD_CUNEICODE_API_LINKAGE_I_
		::std::optional<::std::size_t> __big5_hkscs_code_point_to_index(
		     ::ztd::span<const ::std::pair<::std::uint_least32_t, ::std::uint_least32_t>>
		          __index_code_point_map,
		     char32_t __code) noexcept;

	} // namespace __cnc_detail

	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace cnc

#endif // ZTD_CUNEICODE_DETAIL_INDEX_HPP
