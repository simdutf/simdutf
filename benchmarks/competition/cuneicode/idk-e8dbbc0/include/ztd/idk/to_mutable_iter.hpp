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

#ifndef ZTD_IDK_TO_MUTABLE_ITER_HPP
#define ZTD_IDK_TO_MUTABLE_ITER_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/type_traits.hpp>
#include <ztd/ranges/adl.hpp>
#include <ztd/ranges/iterator.hpp>

#include <iterator>
#include <functional>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {
		enum class __iter_start { __begin, __end };

		template <typename __Iter>
		using __to_mutable_iter_test = decltype(to_mutable_iter(::std::declval<__Iter>()));

		template <typename __Iter>
		using __is_to_mutable_iter_invokable = is_detected<__to_mutable_iter_test, __Iter>;

		template <typename _Type>
		using __has_pair_iterator_insert_test
		     = decltype(::std::declval<_Type&>().insert(::ztd::ranges::cbegin(::std::declval<_Type&>()),
		          ::ztd::ranges::cbegin(::std::declval<_Type&>()), ::ztd::ranges::cend(::std::declval<_Type&>())));
	} // namespace __idk_detail

	template <typename __Iter>
	inline constexpr bool is_to_mutable_iter_invokable_v = __idk_detail::__is_to_mutable_iter_invokable<__Iter>::value;

	namespace __idk_detail {
		template <__iter_start __from>
		struct __tmi_fn : public ::ztd::hijack::token<__tmi_fn<__from>>,
		                  public ::ztd_hijack_global_token<__tmi_fn<__from>> {
			template <typename _FromIt, typename _Range>
			constexpr auto operator()(_FromIt&& __from_it, _Range& __source) const noexcept {
				using _URange = ::ztd::remove_cvref_t<_Range>;
				using _ToIt   = typename _URange::iterator;
				if constexpr (::std::is_same_v<::std::remove_cv_t<::std::remove_reference_t<_ToIt>>, _FromIt>) {
					(void)__source;
					return _ToIt(::std::forward<_FromIt>(__from_it));
				}
				else if constexpr (is_to_mutable_iter_invokable_v<_FromIt>) {
					(void)__source;
					return to_mutable_iter(::std::forward<_FromIt>(__from_it));
				}
				else if constexpr (is_detected_v<__idk_detail::__has_pair_iterator_insert_test, _Range>) {
					// http://eel.is/c++draft/container.requirements#sequence.reqmts-8
					// "The iterator returned from a.insert(p, i, j)
					// points to the copy of the first element inserted into a,
					// or p if i == j."
					// in other words, this is our cheat code to avoid
					// hitting the worst-case-scenario here
					return __source.insert(::std::forward<_FromIt>(__from_it), ::ztd::ranges::cend(__source),
					     ::ztd::ranges::cend(__source));
				}
				else if constexpr (::std::is_invocable_r_v<bool, ::std::not_equal_to<>, _ToIt, _FromIt> // cf
				     && (::ztd::ranges::is_iterator_forward_iterator_v<_FromIt>                         // cf
				          || ::ztd::ranges::is_iterator_bidirectional_iterator_v<_FromIt>)) {
					// we can avoid 2N walk of iterators
					// by just moving up by them if they're
					// comparable to one another
					if constexpr (__from == __iter_start::__begin) {
						auto __begin_it = ::ztd::ranges::begin(__source);
						while (__begin_it != __from_it) {
							++__begin_it;
						}
						return __begin_it;
					}
					else {
						auto __end_it = ::ztd::ranges::end(__source);
						while (__end_it != __from_it) {
							--__end_it;
						}
						return __end_it;
					}
				}
				else {
					if constexpr (__from == __iter_start::__begin) {
						// either this is random access and O(1),
						// or this is some other weird iterator and it's O(2N)
						auto __begin_it = ::ztd::ranges::begin(__source);
						auto __it_dist = ::std::distance(_FromIt(__begin_it), ::std::forward<_FromIt>(__from_it));
						std::advance(__begin_it, __it_dist);
						return __begin_it;
					}
					else {
						// either this is random access and O(1),
						// or this is some other weird iterator and it's O(2N)
						auto __end_it  = ::ztd::ranges::end(__source);
						auto __it_dist = ::std::distance(::std::forward<_FromIt>(__from_it), _FromIt(__end_it));
						std::advance(__end_it, -__it_dist);
						return __end_it;
					}
				}
			}
		};

	} // namespace __idk_detail

	inline namespace __fn {
		//////
		/// @brief Uses various methods to attempt to provide a mutable iterator from a non-mutable iterator.
		inline constexpr const __idk_detail::__tmi_fn<__idk_detail::__iter_start::__begin> to_mutable_iter_from_begin
		     = {};
		//////
		/// @brief Uses various methods to attempt to provide a mutable iterator from a non-mutable iterator.
		inline constexpr const __idk_detail::__tmi_fn<__idk_detail::__iter_start::__end> to_mutable_iter_from_end
		     = {};
		//////
		/// @brief An alias for ztd::to_mutable_iter_from_begin.
		inline constexpr const auto& to_mutable_iter = to_mutable_iter_from_begin;
	} // namespace __fn

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#endif
