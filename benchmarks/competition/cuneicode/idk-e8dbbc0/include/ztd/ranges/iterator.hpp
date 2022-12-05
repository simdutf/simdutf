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

#ifndef ZTD_RANGES_ITERATOR_HPP
#define ZTD_RANGES_ITERATOR_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/ranges/adl.hpp>

#include <ztd/idk/type_traits.hpp>
#include <ztd/idk/to_address.hpp>
#include <ztd/idk/contiguous_iterator_tag.hpp>
#include <ztd/idk/detail/mark_contiguous.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
#include <ranges>
#endif

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __rng_detail {

		template <typename _Type, typename... _Args>
		using __detect_next = decltype(::std::declval<_Type>().next(::std::declval<_Args>()...));

		template <typename _Type, typename... _Args>
		using __detect_prev = decltype(::std::declval<_Type>().next(::std::declval<_Args>()...));

		template <typename _Type>
		using __detect_lvalue_increment = decltype(++::std::declval<_Type&>());

		template <typename _Type>
		using __detect_lvalue_decrement = decltype(--::std::declval<_Type&>());

		template <typename _It, typename _Sen, typename = void>
		struct __is_distance_operable : ::std::false_type { };

		template <typename _It, typename _Sen>
		struct __is_distance_operable<_It, _Sen,
			::std::void_t<decltype(::std::declval<_Sen>() - ::std::declval<_It>())>>
		: ::std::integral_constant<bool,
			  ::std::is_convertible_v<decltype(::std::declval<_Sen>() - ::std::declval<_It>()),
			       iterator_difference_type_t<_It>>> { };

		template <typename _It, typename _Sen>
		inline constexpr bool __is_distance_operable_v = __is_distance_operable<_It, _Sen>::value;

	} // namespace __rng_detail

	template <typename _It>
	using iterator_pointer_t =
		typename __rng_detail::__iterator_pointer_or_fallback<::std::remove_reference_t<_It>>::type;
	template <typename _It>
	inline constexpr bool is_iterator_random_access_iterator_v
		= is_iterator_concept_or_better_v<::std::random_access_iterator_tag, _It>;

	template <typename _It>
	inline constexpr bool is_iterator_contiguous_iterator_v = ::ztd::__idk_detail::__mark_contiguous_v<_It>
		|| (
#if ZTD_IS_ON(ZTD_STD_LIBRARY_CONTIGUOUS_ITERATOR_TAG)
			is_iterator_concept_or_better_v<contiguous_iterator_tag, _It>)
		|| (is_iterator_concept_or_better_v<contiguous_iterator_tag, _It> &&
#else
			::std::is_pointer_v<_It> &&
#endif
			is_to_addressable_v<
				_It> && ::std::is_lvalue_reference_v<iterator_reference_t<::std::remove_reference_t<_It>>>);

	template <typename _It>
	inline constexpr bool is_iterator_input_iterator_v
		= ::std::is_same_v<::std::input_iterator_tag, iterator_concept_t<_It>>;

	template <typename _It>
	inline constexpr bool is_iterator_output_iterator_v
		= ::std::is_same_v<::std::output_iterator_tag, iterator_concept_t<_It>>;

	template <typename _It>
	inline constexpr bool is_iterator_input_or_output_iterator_v
		= is_iterator_input_iterator_v<_It> || is_iterator_output_iterator_v<_It>;

	template <typename _It>
	inline constexpr bool is_iterator_forward_iterator_v
		= ::std::is_same_v<::std::forward_iterator_tag, iterator_concept_t<_It>>;

	template <typename _It>
	inline constexpr bool is_iterator_bidirectional_iterator_v
		= ::std::is_same_v<::std::bidirectional_iterator_tag, iterator_concept_t<_It>>;

	template <typename _It, typename _Sen>
	inline constexpr bool is_sized_sentinel_for_v = __rng_detail::__is_distance_operable_v<_It, _Sen>;

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#include <ztd/epilogue.hpp>

#endif
