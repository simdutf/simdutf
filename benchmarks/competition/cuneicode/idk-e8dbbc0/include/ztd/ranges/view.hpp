// =============================================================================
//
// ztd.idk
// Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.idk licenses may use this file in
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

#ifndef ZTD_RANGES_VIEW_HPP
#define ZTD_RANGES_VIEW_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/ranges/range.hpp>

#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
#include <ranges>
#endif

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __rng_detail {
		template <typename _Ty>
		inline constexpr bool __enable_view
#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
			::std::ranges::enable_view<_Ty>
#else
			= false
#endif
			;
	} // namespace __rng_detail

	template <typename _Ty>
	struct is_view
	: ::std::integral_constant<bool,
		  (__rng_detail::__enable_view<_Ty> && is_range_v<_Ty> // cf
		       && ::std::is_move_constructible_v<_Ty> && ::std::is_move_assignable_v<_Ty>)
		       || (is_range_v<_Ty> && !::std::is_const_v<_Ty> && ::std::is_lvalue_reference_v<_Ty>) // clang-format
		                                                                                            // hack
		  > { };

	template <typename _Ty>
	inline constexpr bool is_view_v = is_view<_Ty>::value;

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#include <ztd/epilogue.hpp>

#endif
