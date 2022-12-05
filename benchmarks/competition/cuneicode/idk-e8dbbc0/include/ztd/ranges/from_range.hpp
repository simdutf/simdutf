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

#ifndef ZTD_RANGES_FROM_RANGE_HPP
#define ZTD_RANGES_FROM_RANGE_HPP

#include <ztd/ranges/version.hpp>

#if ZTD_IS_ON(ZTD_STD_LIBRARY_FROM_RANGE_T)
#include <ranges>
#endif

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

#if ZTD_IS_OFF(ZTD_STD_LIBRARY_FROM_RANGE_T)
	namespace __rng_detail {
		struct __from_range_t { };
	} // namespace __rng_detail
#endif


	using from_range_t =
#if ZTD_IS_ON(ZTD_STD_LIBRARY_FROM_RANGE_T)
		::std::from_range_t
#else
		__rng_detail::__from_range_t
#endif
		;

#if ZTD_IS_ON(ZTD_STD_LIBRARY_FROM_RANGE_T)
	inline constexpr ::std::from_range_t& from_range = ::std::from_range;
#else
	inline constexpr from_range_t from_range = {};
#endif

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#include <ztd/epilogue.hpp>

#endif
