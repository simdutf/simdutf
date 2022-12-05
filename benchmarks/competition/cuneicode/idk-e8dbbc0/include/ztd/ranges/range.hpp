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

#ifndef ZTD_RANGES_RANGE_HPP
#define ZTD_RANGES_RANGE_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/ranges/adl.hpp>
#include <ztd/ranges/iterator.hpp>

#include <ztd/idk/type_traits.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
#include <ranges>
#endif

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	template <typename _Range>
	using range_const_iterator_t
		= decltype(::ztd::ranges::cbegin(::std::declval<::std::add_lvalue_reference_t<_Range>>()));

	template <typename _Range>
	using range_const_sentinel_t
		= decltype(::ztd::ranges::cend(::std::declval<::std::add_lvalue_reference_t<_Range>>()));

	template <typename _Range>
	using range_pointer_t = iterator_pointer_t<range_iterator_t<_Range>>;

	template <typename _Range>
	using range_iterator_category_t = iterator_category_t<range_iterator_t<_Range>>;

	template <typename _Range>
	using range_iterator_concept_t = iterator_concept_t<range_iterator_t<_Range>>;

	template <typename _Tag, typename _Range>
	inline constexpr bool is_range_iterator_concept_or_better_v
		= ::std::is_base_of_v<_Tag, range_iterator_concept_t<_Range>>;

	template <typename _Range>
	inline constexpr bool is_range_input_or_output_range_v
		= is_iterator_input_or_output_iterator_v<range_iterator_t<_Range>>;

	template <typename _Range>
	inline constexpr bool is_range_contiguous_range_v = is_iterator_contiguous_iterator_v<range_iterator_t<_Range>>;

	template <typename _Range>
	inline constexpr bool is_range_random_access_range_v
		= is_range_iterator_concept_or_better_v<::std::random_access_iterator_tag, range_iterator_t<_Range>>;

	template <typename _Range>
	inline constexpr bool is_sized_range_v
		= is_sized_sentinel_for_v<range_iterator_t<_Range>, range_sentinel_t<_Range>>;

	//////
	/// @brief A detection decltype for use with ztd::is_detected and similar. Checks if the given type has a .reserve
	/// member function on it that takes the provided size type.
	template <typename _Type, typename _SizeType = ::std::size_t>
	using detect_reserve_with_size = decltype(::std::declval<_Type>().reserve(::std::declval<_SizeType>()));

	template <typename _Range, typename _Element>
	using detect_push_back = decltype(::std::declval<_Range>().push_back(::std::declval<_Element>()));

	template <typename _Range, typename _IterFirst, typename _IterLast = _IterFirst>
	using detect_insert_bulk = decltype(::std::declval<_Range>().insert(
		::ztd::ranges::begin(::std::declval<_Range>()), ::std::declval<_IterFirst>(), ::std::declval<_IterLast>()));

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#include <ztd/epilogue.hpp>

#endif
